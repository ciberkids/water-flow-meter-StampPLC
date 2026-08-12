/**
 * The firmware loop's CONTROLS and status — the half that stays beside the display.
 *
 * The value editors used to live here too, in the same middle column as the viewport. They are now a
 * separate panel under the Function trace (see FirmwareValuesPanel), which is why this file no longer
 * owns the tick: the loop advances the simulated DEVICE MEMORY in App.tsx, and both panels are views of
 * that one state. Keeping the tick here would have meant two owners of the same fact.
 *
 * Display status lives in this panel's status block, next to the running indicator, because that is the
 * section describing what the loop is currently doing.
 */

interface FirmwareLoopPanelProps {
    running: boolean;
    intervalMs: number;
    /** Simulated backlight state. UP+DOWN released inside 1 s turns it off (interaction_handler.h:91). */
    displayOn: boolean;
    /** The firmware-drawn Select Menu; while it is open the firmware owns every button. */
    selectorOpen: boolean;
    /** How many sensors are marked connected, for the status line. */
    connectedSensors: number;
    /** Total channels, from sensorConfig's single declaration of it rather than a literal here. */
    sensorCount: number;
    onRunningChange: (running: boolean) => void;
    onIntervalChange: (intervalMs: number) => void;
    onSingleTick: () => void;
    onResetValues: () => void;
    /**
     * How the loop drives each channel's flow.
     *
     * `random` is the original: a fresh `Math.random()` per channel per tick, inside that channel's own
     * q_max. Good for seeing the panel move, useless for checking arithmetic — the aggregate, the peak
     * and the accumulated volume all change every tick, so there is nothing to compare against.
     *
     * `steady` holds every connected, ready channel at one figure, which makes all three verifiable by
     * hand: the total is `channels x flow`, the peak settles at `flow`, and the volume after `t` seconds
     * is `channels x flow x t / 60`.
     */
    flowMode: "random" | "steady";
    steadyFlowLpm: number;
    onFlowModeChange: (mode: "random" | "steady") => void;
    onSteadyFlowChange: (lpm: number) => void;
}

const secondaryButton = {
    padding: "6px 12px",
    borderRadius: 6,
    border: "1px solid rgba(156,174,198,0.3)",
    background: "rgba(156,174,198,0.08)",
    color: "#9caec6",
    fontSize: 12
} as const;

export function FirmwareLoopPanel({
    running,
    intervalMs,
    displayOn,
    selectorOpen,
    connectedSensors,
    sensorCount,
    onRunningChange,
    onIntervalChange,
    onSingleTick,
    onResetValues,
    flowMode,
    steadyFlowLpm,
    onFlowModeChange,
    onSteadyFlowChange
}: FirmwareLoopPanelProps) {
    return (
        <div className="firmware-loop-panel">
            <h3>Firmware Loop Simulator</h3>

            <div className="firmware-loop-panel__controls">
                <button
                    type="button"
                    onClick={() => onRunningChange(!running)}
                    style={{
                        padding: "6px 16px",
                        borderRadius: 6,
                        border: "1px solid",
                        borderColor: running ? "#ef4444" : "#22c55e",
                        background: running ? "rgba(239,68,68,0.15)" : "rgba(34,197,94,0.15)",
                        color: running ? "#ef4444" : "#22c55e",
                        fontWeight: 600,
                        cursor: "pointer",
                        fontSize: 12
                    }}
                >
                    {running ? "⏹ Stop" : "▶ Start Loop"}
                </button>

                <button
                    type="button"
                    onClick={onSingleTick}
                    disabled={running}
                    style={{
                        ...secondaryButton,
                        cursor: running ? "not-allowed" : "pointer",
                        opacity: running ? 0.4 : 1
                    }}
                >
                    ⟳ Single Tick
                </button>

                <button type="button" onClick={onResetValues} style={{ ...secondaryButton, cursor: "pointer" }}>
                    ✕ Reset All
                </button>

                <label style={{ display: "flex", alignItems: "center", gap: 4, fontSize: 11, color: "#9caec6" }}>
                    Interval:
                    <input
                        type="range"
                        min={100}
                        max={5000}
                        step={100}
                        value={intervalMs}
                        onChange={(event) => onIntervalChange(Number(event.target.value))}
                        style={{ width: 80 }}
                    />
                    <span style={{ minWidth: 42, textAlign: "right" }}>{intervalMs} ms</span>
                </label>

                {/* Flow source. Deliberately beside the interval: the two together are what decides
                    whether a run is for watching or for checking. */}
                <label style={{ display: "flex", alignItems: "center", gap: 4, fontSize: 11, color: "#9caec6" }}>
                    Flow:
                    <select
                        value={flowMode}
                        onChange={(event) => onFlowModeChange(event.target.value as "random" | "steady")}
                        style={{ fontSize: 11 }}
                    >
                        <option value="random">random per tick</option>
                        <option value="steady">steady</option>
                    </select>
                </label>

                {flowMode === "steady" ? (
                    <label style={{ display: "flex", alignItems: "center", gap: 4, fontSize: 11, color: "#9caec6" }}>
                        at
                        <input
                            type="number"
                            min={0}
                            step={1}
                            value={steadyFlowLpm}
                            onChange={(event) => onSteadyFlowChange(Number(event.target.value))}
                            style={{ width: 64, fontSize: 11 }}
                        />
                        L/min per channel
                    </label>
                ) : null}
            </div>

            {/* Status: what the loop and the device are doing right now. */}
            <div className="firmware-loop-panel__status" role="status" aria-live="polite">
                <span className={running ? "loop-state loop-state--running" : "loop-state"}>
                    {running ? (
                        <>
                            <span className="loop-dot" aria-hidden="true" />
                            Running — advancing device memory every {intervalMs} ms
                            {flowMode === "steady" ? (
                                <span style={{ opacity: 0.8 }}>
                                    {" "}· steady {steadyFlowLpm} L/min x {connectedSensors} ready ={" "}
                                    {(steadyFlowLpm * connectedSensors).toFixed(2)} L/min total, peak{" "}
                                    {steadyFlowLpm.toFixed(2)}
                                </span>
                            ) : null}
                        </>
                    ) : (
                        "Stopped — memory holds its last values"
                    )}
                </span>
                <span className={displayOn ? "display-state display-state--on" : "display-state display-state--off"}>
                    Display: {displayOn ? "ON" : "OFF"}
                    {displayOn ? "" : " — backlight off, panel cleared"}
                </span>
                <span className="loop-sensors">
                    Sensors connected: {connectedSensors} of {sensorCount}
                </span>
                {selectorOpen ? (
                    <span className="loop-selector">Select Menu open — firmware owns all three buttons</span>
                ) : null}
            </div>

            <p className="firmware-loop-panel__where">
                Value editors are under the <b>Function trace</b>, on the right.
            </p>
        </div>
    );
}
