import { useCallback, useMemo, useState } from "react";

import { SimulatedClockState, kSimulatedClockChoices } from "../utils/deviceClock";

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
  /**
   * Bit n of `REG_UNDERSAMPLING_FLAGS`, DERIVED — a readout here, no longer an input.
   *
   * It was the panel's third checkbox, on the grounds that the device computes it from a polling rate the
   * simulator did not model. The simulator models the rate now, so the flag is computed the way
   * `evaluateSensorDiagnostics` computes it and this row reports it. A checkbox for it could raise a
   * warning the configuration did not justify and could not raise the one it did.
   */
  undersampling: boolean;
  /** `overrideActive_ || overridePending_` — the §5.5 confirmation, which IS an input. */
  samplingOverride: boolean;
  /** The channel's ceiling frequency in Hz, or null when the configuration has none to compute. */
  ceilingHz: number | null;
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
  onSensorFieldChange: (
    sensorNumber: number,
    field: "connected" | "ready" | "samplingOverride",
    value: boolean
  ) => void;
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
  /**
   * The simulated device clock's state, and how to change it.
   *
   * Here rather than as a pin on `telemetry.sessionStart`, because `canEdit` correctly refuses that
   * row: it is a derived value device memory owns, so the panel renders it as a readout with no input.
   * The clock is not a value override — it is a device fact with four reachable states, and P3's
   * session-start row says something different in each. Without a control the three "no" states would
   * be unreachable in the running mockup.
   */
  clockState: SimulatedClockState;
  onClockStateChange: (state: SimulatedClockState) => void;
  /**
   * The simulated sampler's achieved rate in kHz, and how to change it.
   *
   * A control for the same reason the clock is one: `diagnostics.pollingRateKhz` is a value memory owns,
   * so `canEdit` correctly refuses to make its row typeable, and without a control here the number every
   * sampling verdict is computed from could be neither seen nor changed. On the device it is MEASURED —
   * `pollingRate_kHz = loopCounter / interval` every second (firmware.cpp:657) — which is precisely why
   * it belongs in the simulator as a dial rather than a constant: what a channel's ceiling means depends
   * entirely on it, and open decision G1 records that no board has ever reported one.
   */
  pollingRateKhz: number;
  onPollingRateChange: (rateKhz: number) => void;
  /** `plc::kSamplingMarginFactor` — how many samples per pulse period the firmware's gate demands. */
  samplingMarginFactor: number;
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
  onSelectSensor,
  clockState,
  onClockStateChange,
  pollingRateKhz,
  onPollingRateChange,
  samplingMarginFactor
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

                  {/* THE §5.5 OVERRIDE — an input, where the flag beside it is a readout.

                      This checkbox used to BE the undersampling flag, and that is the round's whole
                      point: the flag is not an input on the device. `evaluateSensorDiagnostics`
                      recomputes it every pass as
                      `(valid && !meetsNyquistLimit) || overrideActive_ || overridePending_`
                      (modbus_manager.cpp:498-502), so only the two override arms are anybody's choice.
                      Ticking this is the operator who was shown "Sampling too slow" and pressed DOWN to
                      save the figures anyway — a channel deliberately outside budget, which is why the
                      warning survives it.

                      DISABLED ON `connected`, NOT ON `ready`: the firmware skips a channel that is not
                      `inUse` before it reads anything else, so an in-use channel carries the flag
                      independently of its calibration — and it is that independence that makes one
                      uncalibrated plus one undersampling channel possible at all. */}
                  <label
                    className="sensor-row__toggle"
                    title="§5.5: the operator was warned the sampler cannot keep up and chose DOWN = Save anyway. ORed into REG_UNDERSAMPLING_FLAGS by evaluateSensorDiagnostics."
                  >
                    <input
                      type="checkbox"
                      checked={sensor.samplingOverride}
                      disabled={!sensor.connected}
                      onChange={(event) =>
                        onSensorFieldChange(sensorNumber, "samplingOverride", event.target.checked)
                      }
                    />
                    override
                  </label>

                  {/* The DERIVED flag, and the frequency it was derived from — a readout, deliberately
                      not a control. Showing the ceiling beside the verdict is what makes the rule
                      legible: at 3.3 kHz a 450 p/L meter at 100 L/min is 750 Hz and fine, the same meter
                      at 1000 L/min is 7500 Hz and is not, and the number says which by how much. `--` is
                      a configuration with no ceiling to compute — no q_max, or no divisor for its
                      form — which is `SET?` on the panel rather than a sampling question. */}
                  <span
                    className="sensor-row__toggle"
                    title={
                      sensor.ceilingHz === null
                        ? "No ceiling frequency: q_max is 0, or the calibration form's divisor is. The device calls this channel not-calibrated, not undersampling."
                        : `Highest frequency this channel can produce at q_max: ${sensor.ceilingHz.toFixed(1)} Hz. REG_UNDERSAMPLING_FLAGS bit ${sensorNumber - 1}.`
                    }
                    style={{ opacity: sensor.connected ? 1 : 0.45 }}
                  >
                    {sensor.connected && sensor.undersampling ? "! WARN" : "OK"}
                    <span style={{ opacity: 0.7, fontSize: 10 }}>
                      {sensor.ceilingHz === null ? " --" : ` ${Math.round(sensor.ceilingHz)} Hz`}
                    </span>
                  </span>

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

        {/* ── The device clock ────────────────────────────────────────────────────────
             Four states, because DeviceClock has exactly four reachable ones and P3's session-start
             row says something different in each. A select rather than a text box for the same reason
             an enum setting gets one: the states are a fixed ring, and a free box for one is a
             guessing game. Not a pin on telemetry.sessionStart — see the prop's comment. */}
        <button
          type="button"
          className="firmware-values-panel__group"
          onClick={() => toggleGroup("clock")}
          aria-expanded={expanded.has("clock")}
        >
          <span>Clock</span>
          <span aria-hidden="true">{expanded.has("clock") ? "▲" : "▼"}</span>
        </button>
        {expanded.has("clock") ? (
          <div style={{ padding: "4px 0" }}>
            <div className="firmware-values-panel__row">
              <label htmlFor="simulated-clock-state" title="The simulated RTC's trust state and whether a session reset has been dated">
                trust
              </label>
              <select
                id="simulated-clock-state"
                value={clockState}
                onChange={(event) => onClockStateChange(event.target.value as SimulatedClockState)}
              >
                {kSimulatedClockChoices.map((choice) => (
                  <option key={choice.state} value={choice.state} title={choice.hint}>
                    {choice.label}
                  </option>
                ))}
              </select>
              <span className="firmware-values-panel__unit" />
            </div>
            <p className="firmware-values-panel__hint" style={{ padding: "4px 8px" }}>
              {kSimulatedClockChoices.find((choice) => choice.state === clockState)?.hint}. A session or
              measured reset dates the clock, exactly as the two Modbus reset registers do; a peak reset
              does not.
            </p>
          </div>
        ) : null}

        {/* ── The sampler ─────────────────────────────────────────────────────────────
             The rate every undersampling verdict on this panel is computed from. A number box rather
             than a select, because unlike the clock's four states this is a continuous measurement on
             the device and the interesting values are the ones either side of a channel's ceiling.
             0 is deliberately reachable: it is the value `pollingRate_kHz` holds from boot until the
             first one-second window closes, and at 0 every calibrated channel flags. */}
        <button
          type="button"
          className="firmware-values-panel__group"
          onClick={() => toggleGroup("sampler")}
          aria-expanded={expanded.has("sampler")}
        >
          <span>Sampler</span>
          <span aria-hidden="true">{expanded.has("sampler") ? "▲" : "▼"}</span>
        </button>
        {expanded.has("sampler") ? (
          <div style={{ padding: "4px 0" }}>
            <div className="firmware-values-panel__row">
              <label
                htmlFor="simulated-polling-rate"
                title="REG_POLLING_RATE_KHZ / diagnostics.pollingRateKhz. Measured on the device; a dial here."
              >
                polling rate
              </label>
              <input
                id="simulated-polling-rate"
                type="number"
                min={0}
                step={0.1}
                value={pollingRateKhz}
                onChange={(event) => onPollingRateChange(Number(event.target.value))}
                style={{ width: 72, textAlign: "right" }}
                aria-label="Simulated sampler polling rate, kilohertz"
              />
              <span className="firmware-values-panel__unit">kHz</span>
            </div>
            <p className="firmware-values-panel__hint" style={{ padding: "4px 8px" }}>
              A channel flags when {samplingMarginFactor} x its ceiling frequency exceeds this rate —
              `plc::kSamplingMarginFactor` in modbus_manager.h, read by the unit test rather than copied.
              The ceiling is <code>K x q_max / 60</code> for a pulses-per-litre meter and{" "}
              <code>multiplier x q_max + adjust</code> for a formula one, which are the state engine's own
              inversions evaluated at full flow.
            </p>
            <p className="firmware-values-panel__hint" style={{ padding: "0 8px 4px" }}>
              <strong>Unmeasured (G1).</strong> {pollingRateKhz.toFixed(1)} kHz is an assumption — no
              board has ever reported a rate. This shows what the firmware's rule does at a rate, not
              what any hardware achieves.
            </p>
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
