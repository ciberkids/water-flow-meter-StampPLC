import { useCallback, useEffect, useMemo, useRef, useState } from "react";

/* ---------- types ---------- */

interface ValueBinding {
    id: string;
    type?: string;
    unit?: string;
    description?: string;
}

interface FirmwareLoopPanelProps {
    /** All value bindings from the manifest (or extracted from dataset elements) */
    bindings: ValueBinding[];
    /** Current value overrides keyed by binding id */
    values: Record<string, string>;
    /** Callback to update a value override */
    onValueChange: (bindingId: string, value: string) => void;
    /** Callback to update many values at once (batch) */
    onBatchChange: (updates: Record<string, string>) => void;
}

/* ---------- helpers ---------- */

const GROUPS_ORDER = ["sensor", "config", "diagnostics", "countdown", "legend"] as const;

function groupBindings(bindings: ValueBinding[]): Map<string, ValueBinding[]> {
    const map = new Map<string, ValueBinding[]>();
    for (const b of bindings) {
        const prefix = b.id.split(".")[0] ?? "other";
        if (!map.has(prefix)) map.set(prefix, []);
        map.get(prefix)!.push(b);
    }
    return map;
}

function randomFlow(): string {
    return (Math.random() * 10).toFixed(2);
}

function randomCumulative(prev: string): string {
    const base = parseFloat(prev) || 0;
    return (base + Math.random() * 0.5).toFixed(1);
}

function randomStatus(): string {
    const states = ["ready", "ready", "ready", "offline", "error"];
    return states[Math.floor(Math.random() * states.length)];
}

/* ---------- component ---------- */

export function FirmwareLoopPanel({
    bindings,
    values,
    onValueChange,
    onBatchChange,
}: FirmwareLoopPanelProps) {
    const [running, setRunning] = useState(false);
    const [intervalMs, setIntervalMs] = useState(1000);
    const [expanded, setExpanded] = useState<Set<string>>(new Set(["sensor"]));
    const tickRef = useRef<ReturnType<typeof setInterval> | null>(null);
    const valuesRef = useRef(values);
    valuesRef.current = values;

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

    /** Generate one tick of simulated firmware values */
    const tick = useCallback(() => {
        const updates: Record<string, string> = {};
        const current = valuesRef.current;

        for (const b of bindings) {
            if (b.id.includes(".instantFlow")) {
                updates[b.id] = randomFlow();
            } else if (b.id.includes(".cumulativeLiters")) {
                updates[b.id] = randomCumulative(current[b.id] ?? "0");
            } else if (b.id.includes(".cumulativeM3")) {
                const litresId = b.id.replace("cumulativeM3", "cumulativeLiters");
                const litres = parseFloat(updates[litresId] ?? current[litresId] ?? "0");
                updates[b.id] = (litres / 1000).toFixed(4);
            } else if (b.id.includes(".sessionLiters")) {
                updates[b.id] = randomCumulative(current[b.id] ?? "0");
            } else if (b.id.includes(".sessionM3")) {
                const litresId = b.id.replace("sessionM3", "sessionLiters");
                const litres = parseFloat(updates[litresId] ?? current[litresId] ?? "0");
                updates[b.id] = (litres / 1000).toFixed(4);
            } else if (b.id.includes(".maxFlowSinceReset")) {
                const current_max = parseFloat(current[b.id] ?? "0");
                const flowId = b.id.replace("maxFlowSinceReset", "instantFlow");
                const flow = parseFloat(updates[flowId] ?? current[flowId] ?? "0");
                updates[b.id] = Math.max(current_max, flow).toFixed(2);
            } else if (b.id.includes(".status")) {
                // Only change status occasionally
                if (Math.random() < 0.05) {
                    updates[b.id] = randomStatus();
                }
            }
        }

        onBatchChange(updates);
    }, [bindings, onBatchChange]);

    // Start/stop the loop
    useEffect(() => {
        if (running) {
            tickRef.current = setInterval(tick, intervalMs);
        } else if (tickRef.current) {
            clearInterval(tickRef.current);
            tickRef.current = null;
        }
        return () => {
            if (tickRef.current) clearInterval(tickRef.current);
        };
    }, [running, intervalMs, tick]);

    const toggleGroup = useCallback((group: string) => {
        setExpanded(prev => {
            const next = new Set(prev);
            if (next.has(group)) next.delete(group);
            else next.add(group);
            return next;
        });
    }, []);

    const resetAll = useCallback(() => {
        const updates: Record<string, string> = {};
        bindings.forEach(b => { updates[b.id] = ""; });
        onBatchChange(updates);
    }, [bindings, onBatchChange]);

    return (
        <div style={{ marginTop: 16 }}>
            <h3 style={{
                textTransform: "uppercase",
                letterSpacing: "0.08em",
                fontSize: 13,
                fontWeight: 600,
                marginBottom: 8
            }}>
                Firmware Loop Simulator
            </h3>

            {/* Controls */}
            <div style={{
                display: "flex",
                alignItems: "center",
                gap: 8,
                marginBottom: 12,
                flexWrap: "wrap"
            }}>
                <button
                    onClick={() => setRunning(r => !r)}
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
                    onClick={() => tick()}
                    disabled={running}
                    style={{
                        padding: "6px 12px",
                        borderRadius: 6,
                        border: "1px solid rgba(156,174,198,0.3)",
                        background: "rgba(156,174,198,0.08)",
                        color: "#9caec6",
                        cursor: running ? "not-allowed" : "pointer",
                        fontSize: 12,
                        opacity: running ? 0.4 : 1
                    }}
                >
                    ⟳ Single Tick
                </button>

                <button
                    onClick={resetAll}
                    style={{
                        padding: "6px 12px",
                        borderRadius: 6,
                        border: "1px solid rgba(156,174,198,0.3)",
                        background: "rgba(156,174,198,0.08)",
                        color: "#9caec6",
                        cursor: "pointer",
                        fontSize: 12
                    }}
                >
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
                        onChange={e => setIntervalMs(Number(e.target.value))}
                        style={{ width: 80 }}
                    />
                    <span style={{ minWidth: 42, textAlign: "right" }}>{intervalMs} ms</span>
                </label>
            </div>

            {/* Status */}
            {running && (
                <div style={{
                    fontSize: 11,
                    color: "#22c55e",
                    marginBottom: 8,
                    display: "flex",
                    alignItems: "center",
                    gap: 6
                }}>
                    <span style={{
                        display: "inline-block",
                        width: 6,
                        height: 6,
                        borderRadius: "50%",
                        background: "#22c55e",
                        animation: "pulse 1s infinite"
                    }} />
                    Running — pushing values every {intervalMs} ms
                </div>
            )}

            {/* Grouped bindings */}
            <div style={{ maxHeight: 400, overflowY: "auto" }}>
                {sortedGroups.map(group => {
                    const items = grouped.get(group) ?? [];
                    const isOpen = expanded.has(group);

                    return (
                        <div key={group} style={{ marginBottom: 4 }}>
                            <button
                                onClick={() => toggleGroup(group)}
                                style={{
                                    width: "100%",
                                    display: "flex",
                                    justifyContent: "space-between",
                                    alignItems: "center",
                                    padding: "6px 8px",
                                    background: "rgba(15,30,51,0.6)",
                                    border: "1px solid rgba(128,168,201,0.2)",
                                    borderRadius: 4,
                                    color: "#85bbe8",
                                    cursor: "pointer",
                                    fontSize: 12,
                                    fontWeight: 600,
                                    textTransform: "capitalize"
                                }}
                            >
                                <span>{group} ({items.length})</span>
                                <span>{isOpen ? "▲" : "▼"}</span>
                            </button>

                            {isOpen && (
                                <div style={{ padding: "4px 0" }}>
                                    {items.map(b => (
                                        <div
                                            key={b.id}
                                            style={{
                                                display: "flex",
                                                alignItems: "center",
                                                gap: 6,
                                                padding: "3px 8px",
                                                fontSize: 11,
                                                borderBottom: "1px solid rgba(128,168,201,0.08)"
                                            }}
                                        >
                                            <span style={{
                                                flex: 1,
                                                color: "#9caec6",
                                                overflow: "hidden",
                                                textOverflow: "ellipsis",
                                                whiteSpace: "nowrap",
                                                maxWidth: 160
                                            }}
                                                title={b.description ?? b.id}
                                            >
                                                {b.id.split(".").slice(1).join(".")}
                                            </span>
                                            <input
                                                type="text"
                                                value={values[b.id] ?? ""}
                                                onChange={e => onValueChange(b.id, e.target.value)}
                                                placeholder="—"
                                                style={{
                                                    width: 70,
                                                    padding: "2px 4px",
                                                    background: "rgba(0,10,23,0.6)",
                                                    border: "1px solid rgba(128,168,201,0.25)",
                                                    borderRadius: 3,
                                                    color: "#56d2ff",
                                                    fontSize: 11,
                                                    textAlign: "right"
                                                }}
                                            />
                                            {b.unit && (
                                                <span style={{ color: "#9caec6", fontSize: 10, minWidth: 24 }}>
                                                    {b.unit}
                                                </span>
                                            )}
                                        </div>
                                    ))}
                                </div>
                            )}
                        </div>
                    );
                })}
            </div>
        </div>
    );
}
