import { SimulatedButton } from "../types/buttonSimulation";

const BUTTON_METADATA: Record<
  SimulatedButton,
  { label: string; hint: string }
> = {
  up: { label: "BtnA • Up", hint: "Cycles to previous page" },
  down: { label: "BtnB • Down", hint: "Cycles to next page" },
  enter: { label: "BtnC • Enter", hint: "Short = Config · Long = Reset countdown" }
};

interface ButtonPanelProps {
  pressed: Record<SimulatedButton, boolean>;
  onPressStart: (button: SimulatedButton) => void;
  onPressEnd: (button: SimulatedButton) => void;
}

export function ButtonPanel({ pressed, onPressStart, onPressEnd }: ButtonPanelProps) {
  return (
    <section className="button-panel">
      <h3>StampPLC Buttons</h3>
      {(Object.keys(BUTTON_METADATA) as SimulatedButton[]).map((button) => {
        const meta = BUTTON_METADATA[button];
        const active = pressed[button];
        return (
          <button
            key={button}
            type="button"
            className={active ? "active" : ""}
            aria-pressed={active}
            onPointerDown={(event) => {
              event.preventDefault();
              onPressStart(button);
            }}
            onPointerUp={(event) => {
              event.preventDefault();
              onPressEnd(button);
            }}
            onPointerLeave={() => {
              if (pressed[button]) {
                onPressEnd(button);
              }
            }}
            onPointerCancel={() => {
              if (pressed[button]) {
                onPressEnd(button);
              }
            }}
            onClick={(event) => event.preventDefault()}
          >
            {meta.label}
            <span>{meta.hint}</span>
          </button>
        );
      })}
    </section>
  );
}
