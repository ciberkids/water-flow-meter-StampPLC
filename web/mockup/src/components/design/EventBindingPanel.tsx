import type { ScreenDefinition, ScreenFlow } from "../../types";
import type { FirmwareActionDefinition } from "../../types/firmwareActions";

interface EventBindingPanelProps {
  screen?: ScreenDefinition;
  actions: FirmwareActionDefinition[];
  screens: ScreenDefinition[];
  onUpdateFlow: (flowId: string, updates: Partial<ScreenFlow>) => void;
  onAddFlow: () => void;
  onDeleteFlow: (flowId: string) => void;
}

export function EventBindingPanel({
  screen,
  actions,
  screens,
  onUpdateFlow,
  onAddFlow,
  onDeleteFlow
}: EventBindingPanelProps) {
  const flows = screen?.flows ?? [];
  return (
    <section className="event-binding-panel" data-testid="event-binding-panel">
      <header>
        <h3>Event bindings</h3>
        <p>
          Map screen flows to firmware actions from the manifest. Changes apply immediately and are
          reflected in exports.
        </p>
        <button type="button" className="tool-button" onClick={onAddFlow} disabled={!screen}>
          Add button event
        </button>
      </header>
      {flows.length === 0 ? (
        <p>No flows defined for this screen.</p>
      ) : (
        <table>
          <thead>
            <tr>
              <th>Label</th>
              <th>Trigger</th>
              <th>Action</th>
              <th>Target screen</th>
              <th />
            </tr>
          </thead>
          <tbody>
            {flows.map((flow) => (
              <tr key={flow.id}>
                <td>
                  <input
                    type="text"
                    value={flow.label}
                    onChange={(event) => onUpdateFlow(flow.id, { label: event.target.value })}
                  />
                </td>
                <td className="trigger-cell">
                  {flow.trigger.type === "button" ? (
                    <>
                      <span>{flow.trigger.button.toUpperCase()}</span>
                      <span>{flow.trigger.gesture ?? "short"}</span>
                    </>
                  ) : (
                    <span>{flow.trigger.type}</span>
                  )}
                </td>
                <td>
                  <select
                    value={flow.actionId ?? ""}
                    data-testid="flow-action-select"
                    onChange={(event) =>
                      onUpdateFlow(flow.id, { actionId: event.target.value || undefined })
                    }
                  >
                    <option value="">Unassigned</option>
                    {actions.map((action) => (
                      <option key={action.id} value={action.id}>
                        {action.label}
                      </option>
                    ))}
                  </select>
                </td>
                <td>
                  <select
                    value={flow.targetScreenId ?? ""}
                    data-testid="flow-target-select"
                    onChange={(event) =>
                      onUpdateFlow(flow.id, { targetScreenId: event.target.value || undefined })
                    }
                  >
                    <option value="">Stay</option>
                    {screens.map((candidate) => (
                      <option key={candidate.id} value={candidate.id}>
                        {candidate.name}
                      </option>
                    ))}
                  </select>
                </td>
                <td>
                  <button
                    type="button"
                    className="tool-button tool-button--danger"
                    onClick={() => onDeleteFlow(flow.id)}
                  >
                    Remove
                  </button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </section>
  );
}
