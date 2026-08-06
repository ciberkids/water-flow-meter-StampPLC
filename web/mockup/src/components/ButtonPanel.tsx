import type { KeyboardEvent as ReactKeyboardEvent, MouseEvent as ReactMouseEvent, PointerEvent as ReactPointerEvent } from "react";
import { SimulatedButton } from "../types/buttonSimulation";
import { ArmedCombo } from "../utils/comboGestures";

/** Physical buttons on the device — excludes the virtual combo members of the union. */
type PhysicalButton = Exclude<SimulatedButton, "up+down" | "up+down+enter">;

/**
 * One pad per hardware button, in A · B · C order.
 *
 * `key` is the keyboard glyph, not decoration: the window-level mapping in App.tsx binds ArrowUp,
 * ArrowDown and Enter, and nothing in the panel said so.
 *
 * The ORDER is role order (A=UP, B=DOWN, C=ENTER), which is what Gesture_Reference documents. The
 * physical left-to-right placement of the three buttons on the StampPLC is not stated anywhere in this
 * repository — only the electrical mapping (KEYA/KEYB/KEYC on the PI4IOE5V6408) — so this panel does
 * not claim to mirror the board's geometry.
 */
const BUTTON_METADATA: Record<PhysicalButton, { key: string; name: string; role: string; hint: string }> = {
  up: { key: "↑", name: "BtnA", role: "Up", hint: "Previous page · held, repeats every 250 ms" },
  down: { key: "↓", name: "BtnB", role: "Down", hint: "Next page · held, repeats every 250 ms" },
  enter: { key: "⏎", name: "BtnC", role: "Enter", hint: "Short descends or commits · held 1.5 s escapes" }
};

interface ButtonPanelProps {
  pressed: Record<PhysicalButton, boolean>;
  /** Which multi-button gesture is currently armed, from the gesture state machine. */
  armedCombo: ArmedCombo;
  /** Whether the simulated display is on — a fired display-off gesture is visible here. */
  displayOn: boolean;
  /** Whether the firmware-drawn Select Menu is open. */
  selectorOpen: boolean;
  onPressStart: (button: PhysicalButton) => void;
  onPressEnd: (button: PhysicalButton) => void;
}

/**
 * The two real gestures that need more than one button at once.
 *
 * They were unreachable with a mouse and nothing said so: `onPointerDown` gives one pointer, so no
 * amount of clicking can hold two buttons together. The keyboard could, but that was undocumented.
 *
 * The second entry used to press only `["up", "down"]` while calling itself a three-button gesture, so
 * both controls were byte-identical in effect and the recovery gesture could not be performed by ANY
 * route. Timings and semantics below come from interaction_handler.h:91 and :93.
 */
const COMBOS: { id: string; label: string; hint: string; buttons: PhysicalButton[] }[] = [
  {
    id: "display-off",
    label: "BtnA + BtnB — tap",
    hint: "Display off and navigation reset to P0 (§3.1). Fires on RELEASE, within 1 s.",
    buttons: ["up", "down"]
  },
  {
    id: "selector",
    label: "BtnA + BtnB + BtnC — hold 3 s",
    hint: "Select Menu recovery page (Loadable_UI_Menu_Packs §3.4.1). Firmware-drawn, so the panel reports it rather than showing it.",
    buttons: ["up", "down", "enter"]
  }
];

/** What the status line says. Rendered unconditionally — see the CSS note on live regions. */
function statusFor(armedCombo: ArmedCombo, displayOn: boolean, selectorOpen: boolean): {
  text: string;
  modifier: string;
} {
  if (armedCombo === "selector") {
    return { text: "BtnA + BtnB + BtnC held — Select Menu opens at 3 s", modifier: "armed" };
  }
  if (armedCombo === "display-off") {
    return { text: "BtnA + BtnB held — release within 1 s for display off", modifier: "armed" };
  }
  if (selectorOpen) {
    return { text: "Select Menu open — firmware draws this page, not the dataset", modifier: "fired" };
  }
  if (!displayOn) {
    return { text: "Display off — any button wakes it", modifier: "fired" };
  }
  return { text: "No gesture held", modifier: "" };
}

export function ButtonPanel({
  pressed,
  armedCombo,
  displayOn,
  selectorOpen,
  onPressStart,
  onPressEnd
}: ButtonPanelProps) {
  const status = statusFor(armedCombo, displayOn, selectorOpen);

  /**
   * Shared handler set. `onPointerCancel` and `onLostPointerCapture` matter as much as the up/down pair:
   * without them an interrupted touch leaves a button latched down with no release path.
   *
   * Space, and only Space, activates a focused control: Enter is already mapped window-wide to BtnC, so
   * intercepting it here would either double-press or shadow that mapping.
   */
  const handlers = (buttons: PhysicalButton[], isPressed: boolean) => ({
    onPointerDown: (event: ReactPointerEvent) => {
      event.preventDefault();
      buttons.forEach(onPressStart);
    },
    onPointerUp: (event: ReactPointerEvent) => {
      event.preventDefault();
      buttons.forEach(onPressEnd);
    },
    // Guarded, unlike the old combo controls: an unguarded release fired on every pointer that merely
    // swept across the control, which released a button the KEYBOARD was holding.
    onPointerLeave: () => {
      if (isPressed) {
        buttons.forEach(onPressEnd);
      }
    },
    onPointerCancel: () => {
      if (isPressed) {
        buttons.forEach(onPressEnd);
      }
    },
    onLostPointerCapture: () => {
      if (isPressed) {
        buttons.forEach(onPressEnd);
      }
    },
    onKeyDown: (event: ReactKeyboardEvent) => {
      if (event.key === " " && !event.repeat) {
        event.preventDefault();
        buttons.forEach(onPressStart);
      }
    },
    onKeyUp: (event: ReactKeyboardEvent) => {
      if (event.key === " ") {
        event.preventDefault();
        buttons.forEach(onPressEnd);
      }
    },
    onClick: (event: ReactMouseEvent) => event.preventDefault()
  });

  return (
    <section className="button-panel" aria-label="StampPLC buttons">
      <div className="button-panel__head">
        <h3>StampPLC Buttons</h3>
        <p className="button-panel__keys">Keyboard: ↑ ↓ ⏎ · Space activates a focused pad</p>
      </div>

      <div
        className="button-panel__hardware"
        role="group"
        aria-label="Buttons A, B and C in role order"
      >
        {(Object.keys(BUTTON_METADATA) as PhysicalButton[]).map((button) => {
          const meta = BUTTON_METADATA[button];
          const active = pressed[button];
          return (
            <button
              key={button}
              type="button"
              className={active ? "pad active" : "pad"}
              aria-pressed={active}
              aria-label={`${meta.name} ${meta.role}`}
              {...handlers([button], active)}
            >
              <span className="pad__key" aria-hidden="true">
                {meta.key}
              </span>
              <span className="pad__name">{meta.name}</span>
              <span className="pad__role">{meta.role}</span>
            </button>
          );
        })}
      </div>

      <div className="button-panel__legend">
        {(Object.keys(BUTTON_METADATA) as PhysicalButton[]).map((button) => (
          <p key={button}>
            <b>{BUTTON_METADATA[button].name}</b> {BUTTON_METADATA[button].hint}
          </p>
        ))}
      </div>

      <div className="button-panel__combos">
        <p className="button-panel__combos-title">Two- and three-button gestures</p>
        <div className="button-panel__combo-row">
          {COMBOS.map((combo) => {
            const held = combo.buttons.every((button) => pressed[button]);
            return (
              <button
                key={combo.id}
                type="button"
                className={held ? "combo active" : "combo"}
                aria-pressed={held}
                {...handlers(combo.buttons, held)}
              >
                <span className="combo__label">{combo.label}</span>
                <span className="combo__hint">{combo.hint}</span>
              </button>
            );
          })}
        </div>
        <p className="button-panel__combos-hint">
          One pointer can hold one pad, so these press their buttons together. The keyboard works too —
          hold ↑ and ↓ (add ⏎ for the recovery gesture).
        </p>
      </div>

      <p
        className={`button-panel__status${status.modifier ? ` button-panel__status--${status.modifier}` : ""}`}
        role="status"
        aria-live="polite"
      >
        {status.text}
      </p>
    </section>
  );
}
