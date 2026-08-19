import { SimulationTraceEntry } from "../types/simulationTrace";

interface SimulationTracePanelProps {
  entries: SimulationTraceEntry[];
  filter: string;
  onFilterChange: (value: string) => void;
  onReplay: (entry: SimulationTraceEntry) => void;
  onClear: () => void;
}

function formatTimestamp(value: number): string {
  return new Date(value).toLocaleTimeString();
}

export function SimulationTracePanel({ entries, filter, onFilterChange, onReplay, onClear }: SimulationTracePanelProps) {
  const normalizedFilter = filter.trim().toLowerCase();
  const filteredEntries = entries.filter((entry) => {
    if (!normalizedFilter) {
      return true;
    }
    return (
      entry.id.toLowerCase().includes(normalizedFilter) ||
      entry.trigger.toLowerCase().includes(normalizedFilter) ||
      (entry.screenId?.toLowerCase?.().includes(normalizedFilter) ?? false)
    );
  });

  return (
    <div className="simulation-trace-panel">
      <header>
        <div>
          <strong>Function trace</strong>
          <span className="trace-count">{filteredEntries.length} total</span>
        </div>
        <div className="trace-actions">
          <input
            type="search"
            placeholder="Filter actions"
            // A placeholder is NOT an accessible name: it is announced inconsistently and disappears the
            // moment anything is typed (J6).
            aria-label="Filter actions"
            value={filter}
            onChange={(event) => onFilterChange(event.target.value)}
          />
          <button type="button" className="tool-button tool-button--secondary" onClick={onClear}>
            Clear
          </button>
        </div>
      </header>
      {filteredEntries.length === 0 ? (
        <p className="simulation-trace-empty">No trace entries yet.</p>
      ) : (
        <div className="simulation-trace-scroll">
          <ul className="simulation-trace-list">
            {filteredEntries.map((entry) => (
              <li key={`${entry.timestamp}-${entry.id}`}>
                <div className="trace-row">
                  <div>
                    <strong>{entry.functionName ?? entry.id}</strong>
                    {entry.functionName ? <span className="trace-id">{entry.id}</span> : null}
                    {entry.label ? <span className="trace-label">{entry.label}</span> : null}
                    <div className="trace-meta">
                      <span>{formatTimestamp(entry.timestamp)}</span>
                    <span>{entry.trigger}</span>
                    <span>{entry.screenName ?? entry.screenId}</span>
                    {entry.targetScreenId ? <span>→ {entry.targetScreenId}</span> : null}
                    {entry.notes ? <span className="trace-note">{entry.notes}</span> : null}
                  </div>
                </div>
                <button type="button" onClick={() => onReplay(entry)} className="tool-button tool-button--secondary">
                  Replay
                </button>
              </div>
              {entry.actionParams && Object.keys(entry.actionParams).length > 0 ? (
                <pre className="trace-params">{JSON.stringify(entry.actionParams, null, 2)}</pre>
              ) : null}
            </li>
          ))}
          </ul>
        </div>
      )}
    </div>
  );
}
