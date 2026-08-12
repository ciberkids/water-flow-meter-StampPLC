import { useCallback, useMemo, useState } from "react";

/**
 * The firmware loop's VALUE EDITORS — a view of the simulated device memory, under the Function trace.
 *
 * Two things changed when this split out of FirmwareLoopPanel:
 *
 * 1. Every row here reads and writes DEVICE MEMORY. It used to be a flat map of binding id → string that
 *    was fanned out into per-element overrides, so the same fact had two homes and the override won:
 *    switching the selected sensor left the previous sensor's text pinned on screen.
 * 2. The eight sensors are individually addressable. `config.sensor.*` is ONE binding id in the firmware
 *    catalogue — deliberately, because a per-sensor setting has no single register and the sensor is the
 *    navigation level you came from — so the index lives in state here, exactly as `UiNavigator`
 *    caches it on the device, and never in a made-up `config.sensor.3.multiplier` binding id.
 */

interface ValueBinding {
  id: string;
  type?: string;
  unit?: string;
  description?: string;
  category?: string;
  perSensor?: boolean;
  /** The descriptor's fixed set, where it has one — rendered as a dropdown rather than a text box. */
  options?: { label: string; value: number }[];
}

/**
 * Only what this panel needs of a sensor row — declared structurally so the richer memory type in
 * sensorConfig.ts is assignable without this component depending on its shape.
 */
interface SensorRow {
  /** 1-based, the number the device prints. Rows are addressed by THIS, never by array position. */
  number: number;
  connected: boolean;
  ready: boolean;
  /** L/min. Editable here, unlike every other reading — see `onSensorFlowChange`. */
  instantFlowLpm: number;
}

interface FirmwareValuesPanelProps {
  bindings: ValueBinding[];
  /** Resolved values, keyed by binding id: what the device would render right now. */
  values: Record<string, string>;
  onValueChange: (bindingId: string, value: string) => void;
  /**
   * Whether a binding may be typed into.
   *
   * The panel used to render all 104 as text inputs, including the 56 per-sensor READINGS — which are
   * read-only on the device, and whose values memory computes. Typing in one created a pin that
   * outranked memory permanently, so the badge could flip to `--` while the row beside it kept the typed
   * number.
   */
  canEdit: (bindingId: string) => boolean;
  sensors: readonly SensorRow[];
  /** 1-based, 0 when no sensor is implied by the current level — the navigator's contract. */
  selectedSensor: number;
  /**
   * True when NAVIGATION fixed the selection, in which case clicking a row cannot change it.
   *
   * Without this the row button was a silent no-op whenever you were inside a sensor's sub-tree: it set
   * the manual pick, navigation kept winning, and the tooltip promised an edit that never happened.
   */
  selectionFromNavigation: boolean;
  /** What the device would draw for that sensor's flow row, so the effect of a toggle is visible. */
  sensorPreview: (sensorNumber: number) => string;
  onSensorFieldChange: (sensorNumber: number, field: "connected" | "ready", value: boolean) => void;
  /**
   * Sets one channel's instantaneous flow directly, in L/min.
   *
   * On the DEVICE this is a reading, derived from pulses, and everything in this panel that memory owns
   * is read-only for exactly that reason. This is the deliberate exception: in a simulator the flow is
   * the INPUT, and the loop's job is to derive the rest from it — the aggregate, the peak, the volume.
   * Making it editable here removes the need for a separate "steady flow" figure in the loop panel,
   * which was a second place to say the same thing and could only say it for all eight channels at once.
   */
  onSensorFlowChange: (sensorNumber: number, flowLpm: number) => void;
  onSelectSensor: (sensorNumber: number) => void;
}

const GROUPS_ORDER = ["sensor", "config", "net", "telemetry", "diagnostics", "countdown", "legend"] as const;

/** `config.sensor.*` rows are shown in the sensor section, not twice. */
function isPerSensorBinding(binding: ValueBinding): boolean {
  return binding.id.startsWith("config.sensor.");
}

function groupBindings(bindings: ValueBinding[]): Map<string, ValueBinding[]> {
  const map = new Map<string, ValueBinding[]>();
  for (const binding of bindings) {
    if (isPerSensorBinding(binding)) {
      continue;
    }
    const prefix = binding.id.split(".")[0] ?? "other";
    if (!map.has(prefix)) {
      map.set(prefix, []);
    }
    map.get(prefix)!.push(binding);
  }
  return map;
}

/**
 * A field being typed into holds the RAW text until it loses focus.
 *
 * Without this an editable row is not editable: its value comes from memory already formatted — maxFlow
 * resolves to "150 L/min" — so the controlled input contained the unit, `parseInt` read the same 150 back
 * out, and the field snapped to "150 L/min" after every keystroke. Backspace was impossible, and the
 * boolean row turned Off on any keystroke that was not `on`/`1`/`true`/`yes`.
 *
 * The draft is per-field and dies on blur, so what you read when you are NOT typing is always memory.
 */
interface Draft {
  id: string;
  text: string;
}

export function FirmwareValuesPanel({
  bindings,
  values,
  onValueChange,
  canEdit,
  sensors,
  selectedSensor,
  selectionFromNavigation,
  sensorPreview,
  onSensorFieldChange,
  onSensorFlowChange,
  onSelectSensor
}: FirmwareValuesPanelProps) {
  const [expanded, setExpanded] = useState<Set<string>>(new Set(["sensors"]));
  const [draft, setDraft] = useState<Draft | null>(null);

  const grouped = useMemo(() => groupBindings(bindings), [bindings]);
  const sortedGroups = useMemo(() => {
    const keys = Array.from(grouped.keys());
    keys.sort((a, b) => {
      const ia = GROUPS_ORDER.indexOf(a as (typeof GROUPS_ORDER)[number]);
      const ib = GROUPS_ORDER.indexOf(b as (typeof GROUPS_ORDER)[number]);
      return (ia === -1 ? 99 : ia) - (ib === -1 ? 99 : ib);
    });
    return keys;
  }, [grouped]);

  const perSensorSettings = useMemo(() => bindings.filter(isPerSensorBinding), [bindings]);

  const toggleGroup = useCallback((group: string) => {
    setExpanded((previous) => {
      const next = new Set(previous);
      if (next.has(group)) {
        next.delete(group);
      } else {
        next.add(group);
      }
      return next;
    });
  }, []);

  const shortLabel = (id: string) => id.split(".").slice(1).join(".") || id;

  /**
   * One row: label, then either an input or the resolved text, then the unit.
   *
   * The unit column is suppressed when the VALUE already ends with it, which is the general form of two
   * separate duplications: the device's own formats carry units (`%u: %6.2f L/s`), and `formatSetting`
   * appends the descriptor's unit — so both `2: 2.34 L/s` and `150 L/min` had the unit printed twice,
   * once inside the value and once in the column beside it. Testing the rendered string fixes both and
   * cannot drift, where a per-section flag had to be remembered at every call site.
   */
  const renderRow = (binding: ValueBinding) => {
    const editable = canEdit(binding.id);
    // Every value whose domain is a fixed ring: the enums, and the booleans, which carry Off/On.
    const options = binding.options ?? [];
    const resolved = values[binding.id] ?? "";
    const editing = draft?.id === binding.id;
    const shown = editing ? draft.text : resolved;
    // A read-only row is memory's own complete string — units included, and deliberately ABSENT from a
    // withheld reading, which the device prints as bare `1: --`. Appending the column's unit there
    // produced `1: -- L/s`, a unit for a reading that does not exist. Editable rows keep the column,
    // because their input holds a bare number.
    const unitInValue =
      !editable || (Boolean(binding.unit) && resolved.trimEnd().endsWith(binding.unit as string));
    return (
      <div
        className={editable ? "firmware-values-panel__row" : "firmware-values-panel__row firmware-values-panel__row--readonly"}
        key={binding.id}
      >
        <label htmlFor={`value-${binding.id}`} title={binding.description ?? binding.id}>
          {shortLabel(binding.id)}
        </label>
        {editable && options.length > 0 ? (
          /**
           * A SELECT wherever the value has a fixed set, because a free text box for one is a
           * guessing game. `Pulses/L` has to be typed exactly — including the slash and the case —
           * and typing it wrong looked identical to typing it right: the box kept the text and the
           * device ignored it. The list is the descriptor's own options, so it cannot drift from
           * what the device will accept, and the labels are the ones the panel already displays.
           */
          <select
            id={`value-${binding.id}`}
            value={options.find((option) => resolved.startsWith(option.label))?.label ?? ""}
            onChange={(event) => onValueChange(binding.id, event.target.value)}
          >
            {options.map((option) => (
              <option key={option.value} value={option.label}>
                {option.label}
              </option>
            ))}
          </select>
        ) : editable ? (
          <input
            id={`value-${binding.id}`}
            type="text"
            value={shown}
            placeholder="—"
            onFocus={() => setDraft({ id: binding.id, text: resolved })}
            onBlur={() => setDraft((current) => (current?.id === binding.id ? null : current))}
            onChange={(event) => {
              setDraft({ id: binding.id, text: event.target.value });
              onValueChange(binding.id, event.target.value);
            }}
          />
        ) : (
          <span className="firmware-values-panel__readout" title="Computed from device memory">
            {shown || "—"}
          </span>
        )}
        <span className="firmware-values-panel__unit">{unitInValue ? "" : binding.unit ?? ""}</span>
      </div>
    );
  };

  return (
    <div className="firmware-values-panel">
      <header>
        <strong>Device memory</strong>
        <p className="firmware-values-panel__hint">
          Every row is the simulated device's state. Edits go to memory; the loop advances memory.
        </p>
      </header>

      <div className="firmware-values-panel__scroll">
        {/* ── The eight sensors ───────────────────────────────────────────────────── */}
        <button
          type="button"
          className="firmware-values-panel__group"
          onClick={() => toggleGroup("sensors")}
          aria-expanded={expanded.has("sensors")}
        >
          <span>Sensors ({sensors.length})</span>
          <span aria-hidden="true">{expanded.has("sensors") ? "▲" : "▼"}</span>
        </button>

        {expanded.has("sensors") ? (
          <div className="sensor-table">
            {sensors.map((sensor) => {
              // The row's OWN number, not its position. Reading state from position while writing by
              // field is the split that sensorAt() was just fixed to remove; repeating it here would
              // have let a checkbox toggle one sensor while displaying another.
              const sensorNumber = sensor.number;
              const selected = sensorNumber === selectedSensor;
              return (
                <div
                  key={sensorNumber}
                  className={[
                    "sensor-row",
                    selected ? "sensor-row--selected" : "",
                    sensor.connected ? "" : "sensor-row--off"
                  ]
                    .filter(Boolean)
                    .join(" ")}
                >
                  <button
                    type="button"
                    className="sensor-row__name"
                    onClick={() => onSelectSensor(sensorNumber)}
                    disabled={selectionFromNavigation}
                    title={
                      selectionFromNavigation
                        ? `Navigation selects the sensor here (S${selectedSensor}), exactly as UiNavigator does on the device`
                        : `Edit sensor ${sensorNumber}'s settings below`
                    }
                    style={{
                      background: "none",
                      border: "none",
                      padding: 0,
                      cursor: "pointer",
                      textAlign: "left",
                      font: "inherit",
                      color: "inherit"
                    }}
                  >
                    S{sensorNumber}
                  </button>

                  <label className="sensor-row__toggle">
                    <input
                      type="checkbox"
                      checked={sensor.connected}
                      onChange={(event) =>
                        onSensorFieldChange(sensorNumber, "connected", event.target.checked)
                      }
                    />
                    connected
                  </label>

                  {/* Ready is a separate bit because the firmware distinguishes it: every sensor is
                      enabled-but-not-ready at boot, and that renders as WAIT, not as a value. */}
                  <label className="sensor-row__toggle">
                    <input
                      type="checkbox"
                      checked={sensor.ready}
                      disabled={!sensor.connected}
                      onChange={(event) => onSensorFieldChange(sensorNumber, "ready", event.target.checked)}
                    />
                    ready
                  </label>

                  {/* The flow is an INPUT here, not a readout — see `onSensorFlowChange`. Disabled when
                      the channel is not flowing, because `normalizeSensor` forces such a channel to 0
                      and an editable box that silently discards what you type is worse than none. The
                      device's own rendering stays available as the tooltip, padding and all. */}
                  <label
                    className="sensor-row__render"
                    title={`The device draws: ${sensorPreview(sensorNumber)}`}
                  >
                    <input
                      type="number"
                      min={0}
                      step={1}
                      value={Number(sensor.instantFlowLpm.toFixed(2))}
                      disabled={!sensor.connected || !sensor.ready}
                      onChange={(event) => onSensorFlowChange(sensorNumber, Number(event.target.value))}
                      style={{ width: 68, fontSize: 11, textAlign: "right" }}
                      aria-label={`Sensor ${sensorNumber} instantaneous flow, litres per minute`}
                    />
                    <span style={{ opacity: 0.7, fontSize: 10 }}>L/m</span>
                  </label>
                </div>
              );
            })}
          </div>
        ) : null}

        {/* ── The selected sensor's settings, one shared set exactly as on the device ── */}
        {perSensorSettings.length > 0 ? (
          <>
            <button
              type="button"
              className="firmware-values-panel__group"
              onClick={() => toggleGroup("sensor-settings")}
              aria-expanded={expanded.has("sensor-settings")}
            >
              <span>
                Sensor settings ({selectedSensor === 0 ? "no sensor selected" : `S${selectedSensor}`})
              </span>
              <span aria-hidden="true">{expanded.has("sensor-settings") ? "▲" : "▼"}</span>
            </button>
            {expanded.has("sensor-settings") ? (
              <div style={{ padding: "4px 0" }}>
                {selectedSensor === 0 ? (
                  <p className="firmware-values-panel__hint" style={{ padding: "4px 8px" }}>
                    These bindings resolve against the sensor you descended from. Pick one above, or
                    navigate into a SEN page.
                  </p>
                ) : null}
                {perSensorSettings.map((binding) => renderRow(binding))}
              </div>
            ) : null}
          </>
        ) : null}

        {/* ── Everything else, grouped by id prefix ──────────────────────────────── */}
        {sortedGroups.map((group) => {
          const items = grouped.get(group) ?? [];
          const isOpen = expanded.has(group);
          return (
            <div key={group}>
              <button
                type="button"
                className="firmware-values-panel__group"
                onClick={() => toggleGroup(group)}
                aria-expanded={isOpen}
              >
                <span>
                  {group} ({items.length})
                </span>
                <span aria-hidden="true">{isOpen ? "▲" : "▼"}</span>
              </button>
              {isOpen ? (
                <div style={{ padding: "4px 0" }}>
                  {items.map((binding) => renderRow(binding))}
                </div>
              ) : null}
            </div>
          );
        })}
      </div>
    </div>
  );
}
