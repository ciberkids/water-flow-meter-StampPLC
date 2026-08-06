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
}

/**
 * Only what this panel needs of a sensor row — declared structurally so the richer memory type in
 * sensorConfig.ts is assignable without this component depending on its shape.
 */
interface SensorRow {
  connected: boolean;
  ready: boolean;
}

interface FirmwareValuesPanelProps {
  bindings: ValueBinding[];
  /** Resolved values, keyed by binding id: what the device would render right now. */
  values: Record<string, string>;
  onValueChange: (bindingId: string, value: string) => void;
  sensors: readonly SensorRow[];
  /** 1-based, 0 when no sensor is implied by the current level — the navigator's contract. */
  selectedSensor: number;
  /** What the device would draw for that sensor's flow row, so the effect of a toggle is visible. */
  sensorPreview: (sensorNumber: number) => string;
  onSensorFieldChange: (sensorNumber: number, field: "connected" | "ready", value: boolean) => void;
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

export function FirmwareValuesPanel({
  bindings,
  values,
  onValueChange,
  sensors,
  selectedSensor,
  sensorPreview,
  onSensorFieldChange,
  onSelectSensor
}: FirmwareValuesPanelProps) {
  const [expanded, setExpanded] = useState<Set<string>>(new Set(["sensors"]));

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
            {sensors.map((sensor, index) => {
              const sensorNumber = index + 1;
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
                    title={`Edit sensor ${sensorNumber}'s settings below`}
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

                  <span className="sensor-row__render" title="What the device draws for this sensor">
                    {sensorPreview(sensorNumber)}
                  </span>
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
                {perSensorSettings.map((binding) => (
                  <div className="firmware-values-panel__row" key={binding.id}>
                    <label htmlFor={`value-${binding.id}`} title={binding.description ?? binding.id}>
                      {shortLabel(binding.id)}
                    </label>
                    <input
                      id={`value-${binding.id}`}
                      type="text"
                      value={values[binding.id] ?? ""}
                      placeholder="—"
                      disabled={selectedSensor === 0}
                      onChange={(event) => onValueChange(binding.id, event.target.value)}
                    />
                    <span className="firmware-values-panel__unit">{binding.unit ?? ""}</span>
                  </div>
                ))}
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
                  {items.map((binding) => (
                    <div className="firmware-values-panel__row" key={binding.id}>
                      <label htmlFor={`value-${binding.id}`} title={binding.description ?? binding.id}>
                        {shortLabel(binding.id)}
                      </label>
                      <input
                        id={`value-${binding.id}`}
                        type="text"
                        value={values[binding.id] ?? ""}
                        placeholder="—"
                        onChange={(event) => onValueChange(binding.id, event.target.value)}
                      />
                      <span className="firmware-values-panel__unit">{binding.unit ?? ""}</span>
                    </div>
                  ))}
                </div>
              ) : null}
            </div>
          );
        })}
      </div>
    </div>
  );
}
