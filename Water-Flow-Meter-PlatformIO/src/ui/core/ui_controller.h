#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "led/led_controller.h"
#include "ui/core/ui_navigator.h"
#include "ui/core/ui_pages.h"
#include "ui/pack/ui_pack_selector.h"
#include "net/net_status.h"
#include "time/device_clock.h"
#include "ui/core/ui_settings.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"

enum class UiMode { Idle, Info, Configuration };

struct SensorSnapshot {
  bool enabled = false;
  bool ready = false;
  float instantFlow = 0.0f;
  double cumulativeLiters = 0.0;
  float sessionLiters = 0.0f;
  float maxFlow = 0.0f;
};

struct UiRenderContext {
  UiMode mode = UiMode::Info;
  UiPage page = UiPage::GlobalStatus;
  uint16_t warningFlags = 0;
  uint16_t connectedBitmap = 0;
  double totalSessionLiters = 0.0;
  double aggregateFlowLpm = 0.0;
  /**
   * Core-0 sampling rate, as published in register 0.
   *
   * Surfaced on screen because the achievable rate depends on the wiring and the load,
   * so it has to be read off the device rather than assumed (open decision G1).
   */
  float pollingRateKhz = 0.0f;
  uint16_t ledVolumeStep = 1;
  uint16_t ledPulsePeriodMs = 500;
  bool countdownActive = false;
  uint32_t countdownSeconds = 0;
  std::string countdownLabel;
  /**
   * Exporter screen ID of the overlay to draw while a countdown runs, or nullptr
   * to fall back to the default. Each guarded action has its own countdown
   * screen (Display_UI_Requirements §4.3, §5.3), so the overlay cannot be a
   * single fixed screen.
   */
  const char* countdownScreenId = nullptr;
  /** Screen the navigator is on. Null only before begin() has run. */
  const ui_exporter::Screen* currentScreen = nullptr;
  /**
   * The firmware-drawn Select Menu page is open (§3.4).
   *
   * When set, the renderer paints the selector itself and ignores the screen table entirely.
   * That is the requirement, not an optimisation: §3.4.1 says the gesture "works even if the
   * active pack draws nothing at all", which is only true if the firmware draws this page.
   */
  bool selectorActive = false;
  /**
   * The live selector, when `selectorActive`. A pointer rather than a flattened copy: the list is
   * already owned by the controller, and duplicating it into the context would be one more place
   * for the two to disagree.
   */
  const ui::PackSelector* selector = nullptr;
  /** Position within the current level's ring, for the scrollbar. 0 = unknown. */
  uint8_t ringIndex = 0;
  uint8_t ringCount = 0;
  /**
   * True while something on screen changes faster than the 1 Hz telemetry cadence: an open
   * value editor stepping its pending value (§5.4), or a running countdown (§3.3).
   *
   * UiRenderer used to infer this from `mode == UiMode::Configuration`, a mode the 0.2 header
   * note records as "never implemented" and which nothing sets — ui_actions.cpp is the only
   * setMode() caller and it passes Info. So the fast cadence was unreachable and editors and
   * countdowns redrew once a second. Publishing the condition as state rather than deriving
   * it from a retired mode keeps §7's 100 ms acknowledgement out of the mode enum entirely.
   *
   * Navigation between screens is deliberately NOT in here: a page change is a one-off event,
   * and the renderer handles it by repainting the instant the resolved screen differs from the
   * one on the panel. Holding a static menu page at 12.5 Hz would buy nothing and cost a
   * full-panel clear per frame.
   */
  bool interactive = false;
  /**
   * A VALUE EDITOR is open — `UiEditorState::active`, published.
   *
   * Its own field rather than a reuse of either neighbour, because neither can answer the question.
   * `mode` cannot: the note above records that nothing ever sets `UiMode::Configuration`, so an editor
   * screen reports `Info` like everything else. `interactive` cannot either: it is deliberately the UNION
   * of an editor and a countdown, because its job is the repaint cadence, and the two need opposite
   * treatment from the warning banner — an editor suppresses it, a countdown's overlay outranks it.
   *
   * The single reader is `bannerActive()` below, which is where the reasoning lives.
   */
  bool editorActive = false;
  /**
   * Network state, copied once per pass.
   *
   * A COPY, not a pointer to WifiManager: the renderer reads this context without locking while the
   * logic task owns the manager, so exposing the live object would be a data race on every string it
   * reads. Copying ~130 bytes per pass is the cheap side of that trade.
   */
  plc::NetStatusSnapshot net{};
  /**
   * The device clock's trust state and the moment the session counters were last cleared.
   *
   * Three facts rather than a formatted string, and a COPY rather than a `const DeviceClock*`, for the
   * same reason `net` is a snapshot: the renderer reads this context without locking while the logic
   * task owns the clock, and `DeviceClock::now()` advances off `millis()`, so a pointer would let the
   * panel sample a moving object from the wrong task. Three scalars copied per pass is the cheap side.
   *
   * All three are published rather than derived at read time because the resolver has no route to the
   * clock at all — it holds `settings_` and `controller_`, and the clock is owned by firmware.cpp.
   *
   * `sessionStartEpoch == 0` is a REAL answer and stays distinguishable from a timestamp; nothing may
   * render it as a date. The two remaining fields exist to say WHY it is zero, which is not one
   * question but two: with no clock at all the operator must set one, and if
   * `sessionStartAwaitingClock` is set doing so will fill the timestamp in retroactively. With a
   * trusted clock and a zero start, no sync will ever help and only a fresh reset produces a time.
   * A panel that rendered those identically would send the operator to the wrong menu.
   */
  bool clockSet = false;
  uint32_t sessionStartEpoch = 0;
  bool sessionStartAwaitingClock = false;
  /**
   * A SAMPLING fault is present: `REG_UNDERSAMPLING_FLAGS` is non-zero.
   *
   * Exactly that and nothing more. It is NOT the warning banner's gate any longer — `bannerActive()`
   * below is — and it was not widened to become one.
   *
   * WHY THE OLD OBJECTION IS RETIRED RATHER THAN REFUTED. This field used to carry a paragraph arguing
   * that an uncalibrated channel must never raise the banner, because the banner painted edge to edge
   * over y=34..52, mid-panel, across the very config rows an operator reads while calibrating: a
   * factory-fresh device would have worn it permanently over the screens that clear it. That was
   * CORRECT at bannerY = 34. Moving the banner to the footer row (§2c, y=116..133) is what defused it —
   * a permanent commissioning banner now costs a gesture reminder rather than a reading, and the one
   * reminder it must not cost is handled by `bannerActive()`'s editor term.
   *
   * The field stays narrow anyway, for a reason the relocation does not touch: widening it would make
   * `hasWarnings` true while `warningCount == 0`, contradicting the first line above, and
   * `ui_bindings.cpp`'s `telemetry.status` reads it as "how many warnings" rather than "is anything
   * wrong".
   */
  bool hasWarnings = false;
  uint8_t warningCount = 0;
  /**
   * How many IN-USE channels have no valid calibration — the `SET?` channels, counted.
   *
   * A COMMISSIONING GAP, kept as its own number rather than added to `warningCount`, because the two
   * facts ask different things of the operator: an under-sampling channel is a reading that is wrong,
   * an uncalibrated one is a channel nobody has finished setting up. "2 warnings" covering one of each
   * tells them neither.
   *
   * IN USE is part of the definition, not a filter bolted on: a channel that is not in the connected
   * bitmap is ABSENT, not uncalibrated, and counting all eight would report eight problems on a device
   * with one sensor wired. `SensorStateEngine::update` already draws that line for the green LED
   * (sensor_state_engine.cpp:73-83, whose comment records a two-sensor install that could never go
   * green), and this is the same predicate counted rather than reduced to a boolean — the engine
   * answers "is everything ready" for the LED, this answers "how many are not, and why" for the panel.
   *
   * Published rather than derived in the resolver only because `warningSummary` is composed here and
   * needs the same count; it is a PROJECTION rebuilt every pass from `SensorSnapshot::ready`, which is
   * itself rebuilt from the configuration every pass. Nothing caches it — the cached readiness bit is
   * exactly what lied across a reboot.
   */
  uint8_t uncalibratedCount = 0;
  std::string warningSummary;
  std::array<SensorSnapshot, plc::kNumSensors> sensors{};

  /**
   * THE WARNING BANNER'S GATE, and the one home for it.
   *
   * A predicate over the fields rather than a widening of `hasWarnings`, which keeps meaning exactly
   * "REG_UNDERSAMPLING_FLAGS != 0" for its other readers. And ONE predicate rather than two copies of
   * the condition, because `UiRenderer::drawWarningBanner` and the `legend.warning` row colour print the
   * same string (`warningSummary`) and must agree about whether there is anything to say; two spellings
   * of `hasWarnings || uncalibratedCount > 0` in one file is how they would come to disagree.
   *
   * A COMMISSIONING GAP RAISES IT. `SET?` channels are a problem the panel should announce without
   * being paged to a sensor row, which is what §2c's relocation to the footer row made affordable.
   *
   * EXCEPT WHILE A VALUE EDITOR IS OPEN. Every `config-*-edit` screen carries a footer hint ending in
   * `hold=cancel` at y=124 — the only place the abort gesture is documented anywhere — and the banner's
   * band is exactly that row. There are THIRTEEN of them, counted out of the generated table; the decision
   * note that raised this says eighteen, and it is wrong. On a factory-fresh device `uncalibratedCount` is
   * 8, so without this term the banner would hide `hold=cancel` on every one of them, permanently, while
   * the operator is calibrating: the one activity that clears the condition. A commissioning banner must
   * not cover the abort gesture on the screens that close the commissioning gap.
   *
   * SAMPLING FAULTS ARE EXEMPT FROM THAT SUPPRESSION, on purpose. `hasWarnings` says a reading is WRONG
   * — the number on the panel is not the flow — and that is urgent on every screen, including an editor.
   * The uncalibrated half says a setup is UNFINISHED, which can wait for the operator to look up from
   * the setting they are finishing. The asymmetry is the point: urgency about a value outranks a
   * gesture reminder, a reminder about unfinished setup does not.
   */
  bool bannerActive() const { return hasWarnings || (uncalibratedCount > 0 && !editorActive); }
};

/**
 * The value being edited (Display_UI_Requirements §5.4).
 *
 * `pending` is what the operator has dialled up; `saved` is what is in force. Both are
 * shown at once so the operator can see what they are about to commit against what is
 * already there.
 */
struct UiEditorState {
  bool active = false;
  const ui::SettingDescriptor* setting = nullptr;
  uint8_t sensorIndex = 0;
  int32_t pending = 0;
  int32_t saved = 0;
  /** Set when a commit failed its Nyquist check and DOWN can force it (§5.5). */
  bool nyquistPrompt = false;
  /**
   * The commit was refused for a reason that is NOT the Nyquist limit.
   *
   * Kept separate so the operator is told which problem they have. Offering
   * "DOWN = Save anyway" for a failure an override cannot fix would be worse than useless.
   */
  bool commitFailed = false;
  uint32_t lastStepMs = 0;
};

struct UiCountdownState {
  bool active = false;
  uint32_t secondsRemaining = 0;
  /**
   * Millisecond precision alongside the whole seconds shown on screen. The §3.5 LED
   * ramp reaches a 60 ms period, which a value rounded to seconds cannot express.
   */
  uint32_t remainingMs = 0;
  uint32_t totalMs = 0;
  std::string label;
  /** Exporter screen ID of the overlay for this countdown; may be nullptr. */
  const char* screenId = nullptr;
};

class UiController {
 public:
  void begin(uint32_t nowMs);

  void notifyInteraction(uint32_t nowMs);
  void setMode(UiMode mode, uint32_t nowMs);
  void enterIdle(uint32_t nowMs);
  void nextPage(uint32_t nowMs);
  void previousPage(uint32_t nowMs);
  void setPage(UiPage page, uint32_t nowMs);

  void update(uint32_t nowMs,
              const SensorData* sensors,
              const SensorCharacteristics* configs,
              uint16_t warningFlags,
              uint16_t connectedBitmap,
              double totalSessionLiters,
              double aggregateFlowLpm,
              float pollingRateKhz,
              const LedController& ledController,
              const UiCountdownState& countdown,
              const plc::NetStatusSnapshot& netStatus,
              /**
               * The clock, by const reference — the route `ledController` already uses.
               *
               * Not a `DeviceClock*` member set by a `bindClock()` call, which was the alternative:
               * a member would let the controller be constructed, updated and rendered with the
               * clock still null, and "nullable dependency, null everywhere" is the exact defect
               * this round exists to close on ModbusManager. As a required parameter every call site
               * — firmware.cpp and the host harness alike — has to supply one, so the wiring cannot
               * silently go missing.
               *
               * Not a `plc::DeviceClock&` snapshot struct either: `update()` reads three scalars out
               * of it and copies them into the context, exactly as it reads two out of
               * `LedController`, so the reference never escapes this function.
               */
              const plc::DeviceClock& clock);

  const UiRenderContext& context() const { return context_; }
  UiMode mode() const { return mode_; }
  UiPage page() const { return page_; }

  ui::UiNavigator& navigator() { return navigator_; }
  const ui::UiNavigator& navigator() const { return navigator_; }

  /**
   * Keeps UiPage in step with the navigator, one direction only: screen -> page.
   *
   * UiPage remains the source for the `page.title` binding, but the navigator is
   * authoritative for what is drawn. Deriving the page from the screen means the two
   * cannot drift; maintaining both independently is exactly the kind of duplicate
   * bookkeeping that produced the stale router defaults.
   */
  void syncPageFromScreen(const ui_exporter::Screen* screen, uint32_t nowMs);

  const UiEditorState& editor() const { return editor_; }
  void beginEdit(const ui::SettingDescriptor* setting, uint8_t sensorIndex, int32_t current);
  void endEdit();
  void adjustEdit(int32_t delta, uint32_t nowMs);
  void setNyquistPrompt(bool on) { editor_.nyquistPrompt = on; }
  void setCommitFailed(bool on) { editor_.commitFailed = on; }

  // ── The firmware-owned Select Menu page (§3.4) ────────────────────────────────
  //
  // Held here because the interaction handler routes buttons to it and the renderer paints it,
  // and both already hold a UiController. Opening it discards any pending edit: it is reachable
  // from inside an editor, and committing a half-typed value on the way out would be worse than
  // losing it.
  void openPackSelector(const char (*names)[ui::PackLoader::kMaxNameBytes],
                        std::size_t count,
                        const char* activeName,
                        uint32_t nowMs);
  void closePackSelector(uint32_t nowMs);
  /**
   * A root-entry ENTER asked for the Select Menu — read and cleared once (§3.4).
   *
   * A one-shot rather than a call that opens the page: opening needs the card listed, which only
   * firmware.cpp can do. `InteractionHandler::update` transfers this to
   * `InteractionResult::openPackSelector` on the SAME pass it is set, so it can never go stale, and
   * both routes into the page — this and the gesture — end up on one flag with one consumer.
   */
  void requestPackSelector() { packSelectorRequested_ = true; }
  bool consumePackSelectorRequest() {
    const bool requested = packSelectorRequested_;
    packSelectorRequested_ = false;
    return requested;
  }
  bool selectorActive() const { return selectorActive_; }
  ui::PackSelector& packSelector() { return packSelector_; }
  const ui::PackSelector& packSelector() const { return packSelector_; }

 private:
  static constexpr uint32_t kIdleTimeoutMs = 120000;  // 2 minutes

  void updateIdleState(uint32_t nowMs);

  UiMode mode_ = UiMode::Info;
  UiPage page_ = UiPage::GlobalStatus;
  uint32_t lastInteractionMs_ = 0;

  ui::UiNavigator navigator_;
  ui::PackSelector packSelector_{};
  bool selectorActive_ = false;
  bool packSelectorRequested_ = false;
  UiEditorState editor_{};
  UiRenderContext context_;
};
