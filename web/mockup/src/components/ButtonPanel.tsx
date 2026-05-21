import { SimulatedButton } from "../types/buttonSimulation";

/** Physical buttons on the device — excludes virtual combo "up+down" */
type PhysicalButton = Exclude<SimulatedButton, "up+down">;

const BUTTON_METADATA: Record<
  PhysicalButton,
  { label: string; hint: string }
> = {
  up: { label: "BtnA • Up", hint: "Cycles to previous page" },
  down: { label: "BtnB • Down", hint: "Cycles to next page" },
  enter: { label: "BtnC • Enter", hint: "Short = Config · Long = Reset countdown" }
};

interface ButtonPanelProps {
  pressed: Record<PhysicalButton, boolean>;
  comboActive?: boolean;
  onPressStart: (button: PhysicalButton) => void;
  onPressEnd: (button: PhysicalButton) => void;
}

export function ButtonPanel({ pressed, comboActive, onPressStart, onPressEnd }: ButtonPanelProps) {
  return (
    <section className="button-panel">
      <h3>StampPLC Buttons</h3>
      {(Object.keys(BUTTON_METADATA) as PhysicalButton[]).map((button) => {
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
      {comboActive && (
        <div style={{
          marginTop: 8,
          padding: "6px 10px",
          background: "rgba(239, 68, 68, 0.15)",
          border: "1px solid #ef4444",
          borderRadius: 6,
          color: "#ef4444",
          fontSize: 11,
          fontWeight: 600,
          textAlign: "center"
        }}>
          ⚠ UP+DOWN combo active — hold for factory reset
        </div>
      )}
    </section>
  );
}
