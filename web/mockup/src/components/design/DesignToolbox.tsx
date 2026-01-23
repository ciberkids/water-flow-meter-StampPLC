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
  selectedElementId: string | null;
  onSelectElement: (elementId: string) => void;
  onClampElement: (elementId: string) => void;
  overflowElementIds: Set<string>;
  maxCoordinateX: number;
  maxCoordinateY: number;
  maxWidth: number;
  maxHeight: number;
  maxInputLength: number;
}

const SIZE_ELEMENT_KINDS: ElementKind[] = ["box", "icon", "animation", "scrollbar"];

const sanitizeNumericInput = (raw: string, maxDigits: number, max: number): number => {
  const numeric = raw.replace(/[^\d]/g, "").slice(0, maxDigits);
  if (numeric.length === 0) {
    return 0;
  }
  const parsed = Number(numeric);
  if (Number.isNaN(parsed)) {
    return 0;
  }
  return Math.min(max, parsed);
};

export function DesignToolbox({
  screen,
  onAddElement,
  onRemoveElement,
  onUpdateElement,
  selectedElementId,
  onSelectElement,
  onClampElement,
  overflowElementIds,
  maxCoordinateX,
  maxCoordinateY,
  maxWidth,
  maxHeight,
  maxInputLength
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
                <div
                  className={`element-row${selectedElementId === element.id ? " element-row--selected" : ""}`}
                  data-element-id={element.id}
                >
                  <div className="element-row__meta">
                    <input
                      type="radio"
                      name="element-selection"
                      data-testid={`element-select-${element.id}`}
                      checked={selectedElementId === element.id}
                      onChange={() => onSelectElement(element.id)}
                    />
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
                      inputMode="numeric"
                      maxLength={maxInputLength}
                      data-element-id={element.id}
                      data-field="x"
                      value={element.x}
                      onChange={(event) =>
                        onUpdateElement(element.id, {
                          x: sanitizeNumericInput(event.target.value, maxInputLength, maxCoordinateX)
                        })
                      }
                    />
                  </label>
                  <label>
                    Y
                    <input
                      type="number"
                      inputMode="numeric"
                      maxLength={maxInputLength}
                      data-element-id={element.id}
                      data-field="y"
                      value={element.y}
                      onChange={(event) =>
                        onUpdateElement(element.id, {
                          y: sanitizeNumericInput(event.target.value, maxInputLength, maxCoordinateY)
                        })
                      }
                    />
                  </label>
                  {SIZE_ELEMENT_KINDS.includes(element.kind) ? (
                    <>
                      <label>
                        Width
                          <input
                            type="number"
                            inputMode="numeric"
                            maxLength={maxInputLength}
                            data-element-id={element.id}
                            data-field="width"
                            value={element.width ?? 0}
                            onChange={(event) =>
                              onUpdateElement(element.id, {
                                width: sanitizeNumericInput(event.target.value, maxInputLength, maxWidth)
                              })
                            }
                          />
                        </label>
                        <label>
                        Height
                        <input
                          type="number"
                            inputMode="numeric"
                            maxLength={maxInputLength}
                            data-element-id={element.id}
                            data-field="height"
                            value={element.height ?? 0}
                            onChange={(event) =>
                              onUpdateElement(element.id, {
                                height: sanitizeNumericInput(event.target.value, maxInputLength, maxHeight)
                              })
                            }
                          />
                        </label>
                    </>
                  ) : null}
                  <div className="element-row__actions">
                    {overflowElementIds.has(element.id) ? (
                      <button
                        type="button"
                        className="tool-button tool-button--secondary element-row__clamp"
                        data-testid={`element-clamp-${element.id}`}
                        onClick={() => onClampElement(element.id)}
                      >
                        Clamp to display
                      </button>
                    ) : null}
                    <button
                      type="button"
                      className="tool-button tool-button--danger"
                      onClick={() => onRemoveElement(element.id)}
                    >
                      Delete
                    </button>
                  </div>
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
