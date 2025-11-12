import { useMemo } from "react";
import { ScreenDefinition, ScreenElement } from "../types";

interface ValuePlaceholderPanelProps {
  screen?: ScreenDefinition;
  overrides: Record<string, string>;
  onChange: (screenId: string, element: ScreenElement, value: string) => void;
  onRevert: (screenId: string, element: ScreenElement) => void;
  onSave: (screenId: string, element: ScreenElement, value: string) => void;
}

function isValueElement(element: ScreenElement): boolean {
  return element.kind === "value";
}

export function ValuePlaceholderPanel({ screen, overrides, onChange, onRevert, onSave }: ValuePlaceholderPanelProps) {
  const valueElements = useMemo(() => {
    if (!screen) {
      return [];
    }
    return screen.elements.filter(isValueElement);
  }, [screen]);

  if (!screen || valueElements.length === 0) {
    return (
      <section className="value-editor-panel">
        <strong>Value placeholders</strong>
        <p>No value elements on this screen.</p>
      </section>
    );
  }

  const screenId = screen.id;

  return (
    <section className="value-editor-panel">
      <strong>Value placeholders</strong>
      <ul>
        {valueElements.map((element) => {
          const originalValue = element.content ?? "";
          const currentValue = overrides[element.id] ?? originalValue;
          const dirty = currentValue !== originalValue;
          return (
            <li key={element.id} className={dirty ? "dirty" : undefined}>
              <label htmlFor={`value-editor-${element.id}`}>
                <span>{element.id}</span>
                <small>{dirty ? "Modified" : "Default"}</small>
              </label>
              <input
                id={`value-editor-${element.id}`}
                value={currentValue}
                onChange={(event) => onChange(screenId, element, event.target.value)}
              />
              <div className="value-editor-actions">
                <button
                  type="button"
                  className="tool-button tool-button--secondary"
                  disabled={!dirty}
                  onClick={() => onRevert(screenId, element)}
                >
                  Revert
                </button>
                <button
                  type="button"
                  className="tool-button"
                  disabled={!dirty}
                  onClick={() => onSave(screenId, element, currentValue)}
                >
                  Save value
                </button>
              </div>
            </li>
          );
        })}
      </ul>
    </section>
  );
}
