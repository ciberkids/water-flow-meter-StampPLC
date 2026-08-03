#pragma once

#include <cstddef>
#include <cstdint>

namespace plc {

/**
 * Arbitrates the one SPI bus the display and the SD card share.
 *
 * The StampPLC wires MOSI 8, MISO 9 and SCLK 7 to both, with separate chip selects (LCD 12,
 * SD 10) — see `docs/hardware docs/StampPLC specifications.md` §1.1. Two masters on one bus is
 * ordinary; the hard requirement here is stronger than "do not corrupt each other", it is
 * **no visible artifacts**.
 *
 * That distinction drives the whole design. A mutex around each SPI transaction would prevent
 * corruption and still permit a torn frame: the renderer draws a screen as a sequence of
 * transactions, so releasing the bus partway leaves half a frame on the panel. Interleaving at
 * transaction granularity cannot be made clean.
 *
 * So the bus is handed over at FRAME granularity instead, and the handover is a state machine
 * rather than a lock:
 *
 *   DisplayOwns ──requestCard──► CardRequested ──frame closes──► CardOwns
 *        ▲                                                          │
 *        └──── repaint done ──── DisplayResuming ◄──releaseCard─────┘
 *
 * While the card owns the bus the renderer is told it may not begin a frame, so it draws
 * nothing at all rather than something partial. When the card is finished the display is
 * required to repaint IN FULL, because whatever was on the panel is now stale by an unknown
 * amount. Nothing is ever half-drawn, at any point, which is what "clean" has to mean.
 *
 * Priority is **display by default, card by handover, never by preemption.** The display is the
 * operator's feedback channel and it does not get interrupted. The card asks, and waits for a
 * frame boundary. The one exception is boot, where no frame has ever been opened and the grant
 * is therefore immediate — which is why §4.5's "card access at boot only" is the cheapest
 * possible arrangement and remains the preferred one.
 *
 * While the card holds the bus the display cannot say so, so the LEDs do: see
 * `led_patterns.h::cardBusyColor`. That is the only status channel available during a handover,
 * and an operator who sees nothing at all is an operator who pulls the card.
 *
 * Arduino-free and allocation-free, so the whole protocol is host-testable. The interleavings
 * that matter — a request arriving mid-frame, a release arriving before a repaint, a renderer
 * that never closes its frame — are the ones no bench test would reliably reproduce.
 *
 * ── HOW OFTEN THIS ACTUALLY MATTERS ───────────────────────────────────────────────────────
 *
 * Rarely, and that is deliberate. The complete set of card accesses in the design is:
 *
 *   | When                        | What                                  | Display live? |
 *   | Boot                        | read /ui/active, read the pack        | No            |
 *   | Opening the Select Menu page| enumerate /ui/, read the pointer      | YES           |
 *   | Selecting an entry          | write or delete /ui/active, then boot | Yes, briefly  |
 *
 * Nothing else touches the card. In particular nothing is logged to it, and there is no
 * background reader.
 *
 * Two consequences that are correctness statements, not observations:
 *
 *  1. **Every runtime access originates from the UI thread at a known point — a button press.**
 *     There is no task racing the renderer, so this class is NOT thread-safe and does not need
 *     to be. Anyone adding a background card reader must add locking first; the cheap version
 *     here is only sound because of the table above.
 *
 *  2. **The selector's directory listing is the only case that genuinely needs arbitration.**
 *     Boot has no display to contend with, and the write is followed immediately by a reboot —
 *     so it need not hand the bus back at all, and `releaseCard` before a reboot would only buy
 *     a repaint of a screen about to vanish.
 *
 * The 500 ms timeout is therefore a safety net rather than a working path: it exists for a
 * wedged renderer, which is defensive, not expected. Said plainly so nobody reads its presence
 * as evidence that contention is routine.
 */

enum class BusState : uint8_t {
  DisplayOwns = 0,   /**< Normal running state. */
  CardRequested,     /**< The card is waiting for the current frame to close. */
  CardOwns,          /**< The card has the bus; the renderer must not draw. */
  DisplayResuming    /**< The card is done; a full repaint is owed. */
};

const char* busStateText(BusState state);

class SpiArbiter {
 public:
  /**
   * How long a request waits for a frame to close before taking the bus anyway.
   *
   * A renderer that has not closed a frame within this is not drawing — it is stuck — and a
   * stuck renderer must not lock the card out forever. Taking the bus then cannot tear anything,
   * because nothing is progressing. Generous relative to the ~80 ms interactive repaint cadence,
   * so a slow but healthy frame is never interrupted.
   */
  static constexpr uint32_t kFrameWaitTimeoutMs = 500;

  BusState state() const { return state_; }

  /** True while the card holds or is about to hold the bus — what the LED status keys off. */
  bool cardBusy() const { return state_ == BusState::CardOwns; }

  // ── Renderer side ────────────────────────────────────────────────────────────

  /**
   * Asked before `startWrite()`. False means draw nothing this pass.
   *
   * Deliberately checked BEFORE a frame opens and never during one: a mid-frame check would
   * reintroduce exactly the torn frame this class exists to prevent.
   */
  bool mayBeginFrame() const {
    return state_ == BusState::DisplayOwns || state_ == BusState::DisplayResuming;
  }

  void noteFrameBegan(uint32_t nowMs);

  /** Closes the frame, and completes a pending handover if one was waiting on it. */
  void noteFrameEnded(uint32_t nowMs);

  /**
   * True once after the card releases the bus: the next frame must be a full repaint.
   *
   * Consumed by the call, because the obligation is discharged by drawing. An incremental
   * update after a handover would leave whatever the previous frame had drawn, which by then
   * describes state that has moved on.
   */
  bool consumeFullRepaintRequest();

  // ── Card side ────────────────────────────────────────────────────────────────

  /**
   * Requests the bus. Idempotent; safe to call while already requested or granted.
   *
   * Returns true when the bus is available immediately — which at boot it always is, because no
   * frame has been opened yet.
   */
  bool requestCard(uint32_t nowMs);

  /** True when the card may talk to the bus. The card must poll this, not assume it. */
  bool cardGranted() const { return state_ == BusState::CardOwns; }

  /** Hands the bus back and obliges the display to repaint in full. */
  void releaseCard(uint32_t nowMs);

  /** Drives the timeout. Call every pass of the logic loop. */
  void update(uint32_t nowMs);

  /** Diagnostics: how many handovers have completed, and how many timed out waiting. */
  uint32_t handovers() const { return handovers_; }
  uint32_t timeouts() const { return timeouts_; }

 private:
  void grantCard();

  BusState state_ = BusState::DisplayOwns;
  bool frameOpen_ = false;
  bool fullRepaintOwed_ = false;
  uint32_t requestedAtMs_ = 0;
  uint32_t handovers_ = 0;
  uint32_t timeouts_ = 0;
};

}  // namespace plc
