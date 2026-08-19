import { useState } from "react";
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
  /**
   * The hints said "held, repeats every 250 ms". Nothing about that was true, and it is worth being exact
   * about why, because two commits have now been needed to get this line right.
   *
   * `button_input.cpp` does emit repeats — from 1500 ms, every 250 ms — but `mapGesture` maps them to
   * `FlowGesture::Hold`, `matchFlow` demands an exact gesture match, and no screen in the dataset declares
   * a single hold flow. So every repeat is popped, matched against nothing, and dropped: a held UP/DOWN
   * navigates NOWHERE on the device, at any depth — and as of 2026-08-17 that is specified rather than
   * merely observed (`Display_UI_Requirements` §3.1.1).
   *
   * Two paths bypass the flow table and do respond to a held button. §5.4's editor ramp, which is not a
   * repeat at all but a read of the button LEVELS. And the firmware-drawn Select Menu, whose cursor moves
   * on any event kind, because `handlePackSelector` drains the queue and switches on the button alone.
   * Neither is reachable from these hints, which describe the ordinary screens.
   *
   * Someone holding UP on a setting page to dial a number faster was reading this line and believing it.
   */
  up: {
    key: "↑",
    name: "BtnA",
    role: "Up",
    hint: "Tap for the previous page · holding acts only in a value editor, where it ramps ×1 → ×5 → ×25"
  },
  down: {
    key: "↓",
    name: "BtnB",
    role: "Down",
    hint: "Tap for the next page · holding acts only in a value editor, where it ramps ×1 → ×5 → ×25"
  },
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
/**
 * How long the long-ENTER control holds for.
 *
 * `useSimulatedButtons` fires at LONG_PRESS_MS = 1500 on a real timer; the extra 150 ms makes sure
 * that timer has run before the release, so the control cannot intermittently produce a short press.
 */
const LONG_PRESS_HOLD_MS = 1650;

/**
 * The confirm-screen holds. `confirm-reset-totals` and `confirm-reset-portal-login` ask for 3 s,
 * `confirm-reset-session` for 1.5 s — so the 1.5 s control cannot complete the 3 s ones, and holding
 * a pad by hand for three seconds while watching the counter is exactly the thing worth automating.
 *
 * `confirm-factory-reset` wants 30 s and deliberately has NO control: a button that performs a
 * factory reset in one click is not a convenience.
 */
const CONFIRM_HOLD_MS = 3200;

/**
 * The two multi-button gestures, each with the hold its own machine requires.
 *
 * These are CLICK-TO-PERFORM, like the two ENTER holds above and for the same reason: a control that
 * demands three seconds of physically held mouse button is one people release early, and releasing the
 * selector gesture early produces nothing at all — so the control looked broken every time. Reported as
 * exactly that: "I click it and as soon as I release it releases."
 *
 * `ms` is the scripted duration, not the threshold. Display-off must be released INSIDE 1 s to count as a
 * tap, so it gets a short press; the selector fires at 3000 ms while still held, so it gets a margin over
 * that. Both then release, which is what the machine needs to see to settle.
 */
const COMBOS: { id: string; label: string; hint: string; buttons: PhysicalButton[]; ms: number }[] = [
  {
    id: "display-off",
    label: "BtnA + BtnB — tap",
    hint: "Display off and navigation reset to P0 (§3.1). Fires on RELEASE, within 1 s.",
    buttons: ["up", "down"],
    ms: 200
  },
  {
    id: "selector",
    label: "BtnA + BtnB + BtnC — hold 3 s",
    hint: "Select Menu — the only way to switch UI pack (Loadable_UI_Menu_Packs §3.4.1). Fires WHILE held at 3 s; the panel draws it, because the firmware does.",
    buttons: ["up", "down", "enter"],
    ms: CONFIRM_HOLD_MS
  }
];

/** What the status line says. Rendered unconditionally — see the CSS note on live regions. */
function statusFor(armedCombo: ArmedCombo, displayOn: boolean, selectorOpen: boolean): {
  text: string;
  modifier: string;
} {
  /**
   * What HAPPENED outranks what is pending. `armedComboOf` no longer reports a fired selector as armed,
   * so these two cannot both be true any more — but the order is the belt to that braces: the message
   * an operator most needs is the one describing the state they are now in.
   */
  if (selectorOpen) {
    return { text: "Select Menu open — firmware draws this page, not the dataset", modifier: "fired" };
  }
  if (armedCombo === "selector") {
    return { text: "BtnA + BtnB + BtnC held — Select Menu opens at 3 s", modifier: "armed" };
  }
  if (armedCombo === "display-off") {
    return { text: "BtnA + BtnB held — release within 1 s for display off", modifier: "armed" };
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
  const [enterHeld, setEnterHeld] = useState<string | null>(null);
  /** Which multi-button gesture is being performed for us right now, if any. */
  const [comboHeld, setComboHeld] = useState<string | null>(null);
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
      /**
       * CAPTURE the pointer, or a click that drifts a few pixels latches the buttons down for ever.
       *
       * Mouse pointers are not implicitly captured, and `.active` applies `translateY(2px) scale(0.98)`
       * — so the control moves out from under a cursor that has barely moved, `pointerup` is delivered to
       * whatever is underneath instead, and the levels never come back down. `onPointerLeave` is the only
       * thing that saves it, and it is guarded by `isPressed`, which is the value captured at the last
       * RENDER: drift inside the same frame as the press and the guard is still false, so nothing
       * releases. All three pads then sit lit, the status reads "Select Menu opens at 3 s" for ever, and
       * the gesture cannot fire again because it never let go.
       *
       * With capture, every subsequent event for this pointer comes here wherever the cursor wanders,
       * which removes the race rather than narrowing it — and makes `onLostPointerCapture` below the real
       * safety net it was written to be, instead of near-dead code for mouse input.
       */
      // PRESS FIRST, capture second, and never let the capture stop the press. Ordered the other way
      // round this was worse than the bug it fixes: `setPointerCapture` throws on an invalid pointer id,
      // and the throw skipped `onPressStart` entirely, so the control did nothing at all.
      buttons.forEach(onPressStart);
      try {
        event.currentTarget.setPointerCapture?.(event.pointerId);
      } catch {
        // A browser that refuses capture keeps the guarded `onPointerLeave` below as its safety net.
      }
    },
    onPointerUp: (event: ReactPointerEvent) => {
      event.preventDefault();
      buttons.forEach(onPressEnd);
    },
    /**
     * A leave releases only when we are NOT holding the pointer.
     *
     * With capture in place the cursor leaving means nothing — the button is still held and the events
     * still arrive here — so releasing on leave would cancel a deliberate hold the moment a hand drifted.
     * Without capture (a browser that refuses it) this stays the old safety net, still guarded so a
     * pointer merely sweeping across cannot release a button the KEYBOARD is holding.
     */
    onPointerLeave: (event: ReactPointerEvent) => {
      if (event.currentTarget.hasPointerCapture?.(event.pointerId)) {
        return;
      }
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
        <p className="button-panel__combos-title">Held presses</p>
        <div className="button-panel__combo-row">
          {/**
            * BtnC held past the long-press threshold, released for you.
            *
            * Holding a pad with a pointer for a second and a half works, but nothing said so and it
            * is easy to release early and get a SHORT press instead — which on a setting page
            * descends into the editor rather than escaping, so the gesture silently did the opposite
            * of what was wanted. This presses and releases on a real timer, so the machine sees a
            * genuine long press rather than a synthesised event.
            */}
          {[
            {
              id: "hold-long",
              ms: LONG_PRESS_HOLD_MS,
              label: "BtnC — hold 1.5 s",
              hint: "Long ENTER: ascends one level, or discards an open editor."
            },
            {
              id: "hold-confirm",
              ms: CONFIRM_HOLD_MS,
              label: "BtnC — hold 3 s",
              hint: "Completes a 3 s hold-to-confirm. Watch the counter run down on the panel."
            }
          ].map((hold) => (
            <button
              key={hold.id}
              type="button"
              className={enterHeld === hold.id ? "combo active" : "combo"}
              aria-pressed={enterHeld === hold.id}
              onClick={(event) => {
                event.preventDefault();
                if (enterHeld) return;
                setEnterHeld(hold.id);
                onPressStart("enter");
                window.setTimeout(() => {
                  onPressEnd("enter");
                  setEnterHeld(null);
                }, hold.ms);
              }}
            >
              <span className="combo__label">{hold.label}</span>
              <span className="combo__hint">
                {enterHeld === hold.id ? "Holding ENTER…" : hold.hint}
              </span>
            </button>
          ))}
        </div>
        <p className="button-panel__combos-title">Two- and three-button gestures</p>
        <div className="button-panel__combo-row">
          {COMBOS.map((combo) => {
            const held = combo.buttons.every((button) => pressed[button]);
            const running = comboHeld === combo.id;
            return (
              <button
                key={combo.id}
                type="button"
                className={held || running ? "combo active" : "combo"}
                aria-pressed={held || running}
                onClick={(event) => {
                  event.preventDefault();
                  if (comboHeld) return;
                  setComboHeld(combo.id);
                  combo.buttons.forEach(onPressStart);
                  window.setTimeout(() => {
                    combo.buttons.forEach(onPressEnd);
                    setComboHeld(null);
                  }, combo.ms);
                }}
              >
                <span className="combo__label">{combo.label}</span>
                <span className="combo__hint">
                  {running ? `Holding ${combo.buttons.length} buttons…` : combo.hint}
                </span>
              </button>
            );
          })}
        </div>
        <p className="button-panel__combos-hint">
          Click once and the panel performs the whole gesture, holding for as long as the device needs — a
          three-second mouse hold is one people let go of early, and letting go early of the selector
          gesture produces nothing. The keyboard still works by hand: hold ↑ and ↓, adding ⏎ for the
          recovery gesture.
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
