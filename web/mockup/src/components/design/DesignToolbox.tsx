import type { ElementKind, ScreenDefinition, ScreenElement } from "../../types";

const ELEMENT_LABELS: Array<{ kind: ElementKind; label: string }> = [
  { kind: "text", label: "Text" },
  { kind: "value", label: "Value" },
  { kind: "box", label: "Box" },
  { kind: "badge", label: "Badge" },
  { kind: "icon", label: "SVG Ref" },
  { kind: "animation", label: "Animation box" },
  { kind: "scrollbar", label: "Scroll bar" }
];

interface DesignToolboxProps {
  screen?: ScreenDefinition;
  onAddElement: (kind: ElementKind) => void;
  onRemoveElement: (elementId: string) => void;
  onUpdateElement: (elementId: string, updates: Partial<ScreenElement>) => void;
}

export function DesignToolbox({
  screen,
  onAddElement,
  onRemoveElement,
  onUpdateElement
}: DesignToolboxProps) {
  return (
    <section className="design-toolbox">
      <header>
        <h3>Element toolbox</h3>
        <p>Select an element type to insert into the current screen.</p>
      </header>
      <div className="design-toolbox__buttons">
        {ELEMENT_LABELS.map((item) => (
          <button
            key={item.kind}
            type="button"
            className="tool-button"
            data-testid={`design-add-${item.kind}`}
            onClick={() => onAddElement(item.kind)}
            disabled={!screen}
          >
            {item.label}
          </button>
        ))}
      </div>

      <div className="design-toolbox__list" data-testid="design-element-list">
        <h4>Elements</h4>
        {screen?.elements.length ? (
          <ul>
            {screen.elements.map((element) => (
              <li key={element.id}>
                <div className="element-row">
                  <div>
                    <strong>{element.id}</strong>
                    <span>{element.kind}</span>
                  </div>
                  <label>
                    Content
                    <input
                      type="text"
                      value={element.content ?? ""}
                      onChange={(event) =>
                        onUpdateElement(element.id, { content: event.target.value })
                      }
                    />
                  </label>
                  <label>
                    X
                    <input
                      type="number"
                      value={element.x}
                      onChange={(event) =>
                        onUpdateElement(element.id, { x: Number(event.target.value) })
                      }
                    />
                  </label>
                  <label>
                    Y
                    <input
                      type="number"
                      value={element.y}
                      onChange={(event) =>
                        onUpdateElement(element.id, { y: Number(event.target.value) })
                      }
                    />
                  </label>
                  <button
                    type="button"
                    className="tool-button tool-button--danger"
                    onClick={() => onRemoveElement(element.id)}
                  >
                    Delete
                  </button>
                </div>
              </li>
            ))}
          </ul>
        ) : (
          <p>No elements yet.</p>
        )}
      </div>
    </section>
  );
}
