/**
 * The simulator's half of `telemetry.sessionStart` — a deliberate mirror of the firmware.
 *
 * The device resolves this in `resolveTelemetryBinding` (ui_bindings.cpp) from three facts published on
 * UiRenderContext: whether the clock is trusted, the epoch the session counters were last cleared at,
 * and whether a reset is still waiting for a clock to date it. This file reproduces exactly that
 * decision from exactly those three facts, so the mockup cannot show a string the panel will not.
 *
 * Kept OUT of App.tsx on purpose: it is the one part of the feature with four distinguishable outputs,
 * and a pure function is the only shape a unit test can pin all four of down. Everything App.tsx adds
 * is the question of where the three facts come from in a browser, which is a separate decision and is
 * commented where it is made.
 */

/** The three facts the device publishes, and nothing else. Mirrors UiRenderContext's new fields. */
export interface SimulatedClock {
  /** `DeviceClock::isSet()` — there is a time worth showing. */
  set: boolean;
  /** `DeviceClock::sessionStartEpoch()` — Unix seconds, or 0 when the start cannot be stated. */
  sessionStartEpoch: number;
  /** `DeviceClock::sessionStartAwaitingClock()` — a reset happened before there was a clock. */
  awaitingClock: boolean;
}

/**
 * `YYYY-MM-DD HH:MM UTC` from Unix seconds.
 *
 * `Date.getUTC*` rather than `toISOString().slice(...)`: the slice would be shorter but silently
 * depends on the exact ISO shape, and rather than the local getters it is the UTC ones that matter —
 * the device has no timezone setting at all, so a browser in Rome must render the same characters the
 * panel does. Built by hand for the same reason `civilFromEpoch` is written out in device_clock.cpp.
 */
function utcMinuteStamp(epochSeconds: number): string {
  const at = new Date(epochSeconds * 1000);
  const pad = (value: number, width = 2) => String(value).padStart(width, "0");
  return (
    `${pad(at.getUTCFullYear(), 4)}-${pad(at.getUTCMonth() + 1)}-${pad(at.getUTCDate())} ` +
    `${pad(at.getUTCHours())}:${pad(at.getUTCMinutes())} UTC`
  );
}

/**
 * What P3's session-start row renders. Four outputs, and the three negatives are NOT interchangeable.
 *
 * The ordering matches the firmware arm exactly, including the `set &&` on the first branch: an epoch
 * with no clock behind it is not reachable through DeviceClock, and both implementations refuse to
 * render one rather than trusting that it stays unreachable.
 *
 *   timestamp        the answer.
 *   "AWAITING CLOCK" a reset happened with no clock; setting the clock fills this in retroactively.
 *   "CLOCK UNSET"    no clock and no reset waiting; setting the clock will NOT produce a time here.
 *   "UNKNOWN"        the clock is trusted and nothing recorded a start; only a reset can fix it.
 *
 * Never a zero and never 1970 — the epoch is checked before it reaches the formatter.
 */
export function sessionStartText(clock: SimulatedClock): string {
  if (clock.set && clock.sessionStartEpoch !== 0) {
    return utcMinuteStamp(clock.sessionStartEpoch);
  }
  if (!clock.set) {
    return clock.awaitingClock ? "AWAITING CLOCK" : "CLOCK UNSET";
  }
  return "UNKNOWN";
}

/**
 * Which of DeviceClock's four reachable states the simulated device is in.
 *
 * A browser has no RX8130CE and no VLF flag, so "is the clock trusted" is not a fact the mockup can
 * discover — it has to be an input, and it is a DEVICE FACT rather than a display override. So it is
 * explicit state with its own control in the Device memory panel, NOT a pin on `telemetry.sessionStart`.
 *
 * The pin was the first design and it does not work: `canEditBinding` in App.tsx makes a row editable
 * only when `category === "setting"`, and additionally refuses everything `telemetry.*` because device
 * memory owns it. `telemetry.sessionStart` is a read-only derived value on both counts, so the panel
 * renders it as a readout with no input at all — the three "no" cases would have been unreachable in
 * the running mockup while their unit tests passed. That rule is right and worth keeping; the clock
 * simply is not a value override.
 *
 * Four states, because DeviceClock has exactly four reachable ones and each renders differently. There
 * is no fifth: an epoch with no clock behind it cannot be constructed here, as on the device.
 */
export type SimulatedClockState =
  /** Trusted clock, and a reset recorded when the session began. */
  | "dated"
  /** Trusted clock, but nothing ever recorded a start — only a reset can produce one. */
  | "undated"
  /** No clock at all, and no reset waiting to be dated. */
  | "unset"
  /** No clock, and a reset waiting for one; the next sync will date it. */
  | "awaiting";

/** The four states as the control offers them, with what each one makes P3 say. */
export const kSimulatedClockChoices: ReadonlyArray<{
  state: SimulatedClockState;
  label: string;
  hint: string;
}> = [
  { state: "dated", label: "Set, session dated", hint: "P3 shows the timestamp" },
  { state: "undated", label: "Set, never reset", hint: "P3 shows UNKNOWN" },
  { state: "unset", label: "Unset", hint: "P3 shows CLOCK UNSET" },
  { state: "awaiting", label: "Unset, reset pending", hint: "P3 shows AWAITING CLOCK" }
];

/** The three facts the device would publish, for a given simulated state. */
export function simulatedClock(state: SimulatedClockState, sessionStartEpoch: number): SimulatedClock {
  switch (state) {
    case "dated":
      return { set: true, sessionStartEpoch, awaitingClock: false };
    case "undated":
      return { set: true, sessionStartEpoch: 0, awaitingClock: false };
    case "unset":
      return { set: false, sessionStartEpoch: 0, awaitingClock: false };
    case "awaiting":
      return { set: false, sessionStartEpoch: 0, awaitingClock: true };
  }
}

/**
 * What a session reset does to the clock's state — a mirror of `DeviceClock::noteSessionStart`.
 *
 * With a clock, the reset is dated and the state becomes "dated". WITHOUT one it cannot be, so the
 * reset is remembered as awaiting a clock instead: that is the firmware's `sessionStartUnknown_ = true`
 * branch, and reproducing it here is what makes a reset in the mockup behave like a reset on the
 * device rather than silently jumping to a timestamp the device could not have produced.
 */
export function clockAfterSessionReset(state: SimulatedClockState): SimulatedClockState {
  return state === "unset" || state === "awaiting" ? "awaiting" : "dated";
}
