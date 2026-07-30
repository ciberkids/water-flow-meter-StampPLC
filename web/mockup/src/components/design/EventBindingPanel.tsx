import {
  ScreenDefinition,
  ScreenEvent,
  ScreenDefinition as ScreenDefWithEvents,
  ScreenEvent as ScreenEvWithEvents
} from "../../types";
import { FirmwareActionDefinition } from "../../types/firmwareActions";

interface EventBindingPanelProps {
  screen?: ScreenDefinition;
  actions: FirmwareActionDefinition[];
  screens: ScreenDefinition[];
  onAddEvent: (screenId: string) => void;
  onUpdateEvent: (screenId: string, index: number, updates: Partial<ScreenEvent>) => void;
  onRemoveEvent: (screenId: string, index: number) => void;
}

const TRIGGER_OPTIONS = [
  { value: "btn_a_click", label: "Button A Click" },
  { value: "btn_b_click", label: "Button B Click" },
  { value: "btn_c_click", label: "Button C Click" },
  { value: "encoder_push", label: "Encoder Push" },
  { value: "encoder_cw", label: "Encoder CW" },
  { value: "encoder_ccw", label: "Encoder CCW" }
];

export function EventBindingPanel({
  screen,
  actions,
  screens,
  onAddEvent,
  onUpdateEvent,
  onRemoveEvent
}: EventBindingPanelProps) {
  if (!screen) {
    return (
      <section className="event-binding-panel">
        <header>
          <h3>Event bindings</h3>
          <p>No screen selected.</p>
        </header>
      </section>
    );
  }

  const events = screen.events ?? [];

  return (
    <section className="event-binding-panel">
      <header>
        <div className="toolbox-header-row">
          <h3>Event bindings</h3>
          <button
            type="button"
            className="tool-button tool-button--secondary tool-button--small"
            onClick={() => onAddEvent(screen.id)}
          >
            Add Event
          </button>
        </div>
        <p>Map hardware inputs to firmware actions.</p>
      </header>

      {events.length === 0 ? (
        <p className="empty-state">No events configured.</p>
      ) : (
        <ul className="event-list">
          {events.map((event, index) => (
            <li key={index} className="event-item">
              <div className="event-row">
                <label>
                  <span>Trigger</span>
                  <select
                    value={event.trigger}
                    onChange={(e) => onUpdateEvent(screen.id, index, { trigger: e.target.value })}
                  >
                    <option value="" disabled>Select Trigger</option>
                    {TRIGGER_OPTIONS.map((opt) => (
                      <option key={opt.value} value={opt.value}>
                        {opt.label}
                      </option>
                    ))}
                  </select>
                </label>
                <div className="event-actions">
                  <button
                    type="button"
                    className="icon-button delete-button"
                    onClick={() => onRemoveEvent(screen.id, index)}
                    title="Remove Event"
                  >
                    ×
                  </button>
                </div>
              </div>

              <div className="event-row">
                <label>
                  <span>Action</span>
                  <select
                    value={event.actionId ?? ""}
                    onChange={(e) => onUpdateEvent(screen.id, index, { actionId: e.target.value || undefined })}
                  >
                    <option value="">(None)</option>
                    {actions.map((action) => (
                      <option key={action.id} value={action.id}>
                        {action.label}
                      </option>
                    ))}
                  </select>
                </label>
              </div>

              <div className="event-row">
                <label>
                  <span>Target Screen</span>
                  <select
                    value={event.targetScreenId ?? ""}
                    onChange={(e) => onUpdateEvent(screen.id, index, { targetScreenId: e.target.value || undefined })}
                  >
                    <option value="">(None)</option>
                    {screens.map((s) => (
                      <option key={s.id} value={s.id}>
                        {s.name}
                      </option>
                    ))}
                  </select>
                </label>
              </div>
            </li>
          ))}
        </ul>
      )}
    </section>
  );
}
