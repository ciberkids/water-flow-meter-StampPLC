#include "ui/core/ui_controller.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

void UiController::begin(uint32_t nowMs) {
  mode_ = UiMode::Info;
  page_ = UiPage::GlobalStatus;
  lastInteractionMs_ = nowMs;
  context_ = UiRenderContext{};
  context_.mode = mode_;
  context_.page = page_;
}

void UiController::syncPageFromScreen(const ui_exporter::Screen* screen, uint32_t nowMs) {
  if (!screen || !screen->id) {
    return;
  }
  // Only info-level screens map onto a UiPage; deeper levels leave it untouched so
  // page.title keeps naming the info page the operator came from.
  //
  // `kInfoScreenIds` from ui/core/ui_pages.h, NOT a local copy. This function used to carry its own
  // identical array — a second home for the very mapping that header exists to keep in one place,
  // and its comment already records that the enum and the ids drifted once when they were apart.
  // The ring renumbering of §5.1 found it: the router had been updated and this had not, so an
  // operator paging to P2 would have had `page.title` name a screen the ring no longer contains.
  for (std::size_t i = 0; i < static_cast<std::size_t>(UiPage::Count); ++i) {
    if (std::strcmp(screen->id, kInfoScreenIds[i]) == 0) {
      setPage(static_cast<UiPage>(i), nowMs);
      return;
    }
  }
  notifyInteraction(nowMs);
}

void UiController::beginEdit(const ui::SettingDescriptor* setting,
                            uint8_t sensorIndex,
                            int32_t current) {
  editor_ = UiEditorState{};
  if (!setting) {
    return;
  }
  editor_.active = true;
  editor_.setting = setting;
  editor_.sensorIndex = sensorIndex;
  editor_.pending = current;
  editor_.saved = current;
}

void UiController::endEdit() { editor_ = UiEditorState{}; }

void UiController::adjustEdit(int32_t delta, uint32_t nowMs) {
  if (!editor_.active || !editor_.setting) {
    return;
  }
  editor_.pending = ui::adjustSetting(*editor_.setting, editor_.pending, delta);
  editor_.lastStepMs = nowMs;
  // Changing the value invalidates any prompt about the previous one.
  editor_.nyquistPrompt = false;
  editor_.commitFailed = false;
  notifyInteraction(nowMs);
}

void UiController::notifyInteraction(uint32_t nowMs) {
  lastInteractionMs_ = nowMs;
  if (mode_ == UiMode::Idle) {
    mode_ = UiMode::Info;
  }
}

void UiController::setMode(UiMode mode, uint32_t nowMs) {
  mode_ = mode;
  if (mode_ == UiMode::Idle) {
    lastInteractionMs_ = nowMs;
  } else {
    notifyInteraction(nowMs);
  }
}

void UiController::enterIdle(uint32_t nowMs) {
  // Going idle must leave the device in the state it will WAKE in, because the rendered
  // screen comes from the navigator (see update(), which publishes
  // context_.currentScreen = navigator_.current()) and not from page_.
  //
  // Previously this set only mode_ and the timestamp. Display_UI_Requirements §3 says the
  // UP+DOWN gesture resets navigation to P0 "from any screen at any depth", and the comment
  // above handleDisplayOffCombo says it "clears the navigation stack" — neither was true.
  // The display woke on whichever screen the operator had left it on, with any open editor
  // still live, so the first UP/DOWN hold resumed the acceleration ramp on an invisible
  // setting and the first ENTER could commit a config write nobody confirmed.
  endEdit();
  navigator_.escape();
  page_ = UiPage::GlobalStatus;
  mode_ = UiMode::Idle;
  lastInteractionMs_ = nowMs;
}

void UiController::nextPage(uint32_t nowMs) {
  notifyInteraction(nowMs);
  int pageIndex = static_cast<int>(page_);
  pageIndex = (pageIndex + 1) % static_cast<int>(UiPage::Count);
  page_ = static_cast<UiPage>(pageIndex);
}

void UiController::previousPage(uint32_t nowMs) {
  notifyInteraction(nowMs);
  int pageIndex = static_cast<int>(page_);
  pageIndex = (pageIndex - 1 + static_cast<int>(UiPage::Count)) % static_cast<int>(UiPage::Count);
  page_ = static_cast<UiPage>(pageIndex);
}

void UiController::setPage(UiPage page, uint32_t nowMs) {
  notifyInteraction(nowMs);
  page_ = page;
}

void UiController::update(uint32_t nowMs,
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
                          const plc::DeviceClock& clock) {
  updateIdleState(nowMs);

  context_.mode = mode_;
  context_.page = page_;
  context_.warningFlags = warningFlags;
  context_.connectedBitmap = connectedBitmap;
  context_.totalSessionLiters = totalSessionLiters;
  context_.aggregateFlowLpm = aggregateFlowLpm;
  context_.pollingRateKhz = pollingRateKhz;
  context_.ledVolumeStep = ledController.volumeStepLiters();
  context_.ledPulsePeriodMs = ledController.pulsePeriodMs();
  context_.countdownActive = countdown.active;
  context_.countdownSeconds = countdown.secondsRemaining;
  context_.countdownLabel = countdown.label;
  context_.countdownScreenId = countdown.screenId;
  context_.net = netStatus;
  // Read out as three scalars rather than held as a reference, so the renderer never samples a clock
  // that is advancing on another task. `sessionStartEpoch()` is a stored moment and does not move;
  // `now()` would, which is why nothing here publishes it.
  context_.clockSet = clock.isSet();
  context_.sessionStartEpoch = clock.sessionStartEpoch();
  context_.sessionStartAwaitingClock = clock.sessionStartAwaitingClock();
  // The two things that change between telemetry ticks, and so the two things that decide
  // the repaint cadence. See UiRenderContext::interactive.
  context_.interactive = editor_.active || countdown.active;
  context_.currentScreen = navigator_.current();
  context_.selectorActive = selectorActive_;
  context_.selector = selectorActive_ ? &packSelector_ : nullptr;
  uint8_t ringIndex = 0;
  uint8_t ringCount = 0;
  if (navigator_.ringPosition(&ringIndex, &ringCount)) {
    context_.ringIndex = ringIndex;
    context_.ringCount = ringCount;
  } else {
    context_.ringIndex = 0;
    context_.ringCount = 0;
  }
  context_.hasWarnings = warningFlags != 0;
  context_.warningCount = 0;
  context_.uncalibratedCount = 0;

  // The P0 flow indicator is driven straight from aggregateFlowLpm by
  // UiRenderer::drawFlowDots(); no frame counter is kept here.

  /**
   * The flagged channels as a list, built into a fixed buffer rather than appended straight onto
   * `warningSummary`.
   *
   * Two reasons. The summary now has to choose between phrasings AFTER both counts are known — a list
   * appended as the loop went would have to be unpicked when an uncalibrated channel outranks it — and
   * the buffer replaces the per-channel `std::to_string` this loop used to run on every pass of the
   * logic loop, so the summary costs one assignment into a string that keeps its capacity.
   *
   * 32 bytes holds "1, 2, 3, 4, 5, 6, 7, 8" (22) with room to spare, and the guard below stops short of
   * the next entry rather than truncating mid-number. It leaves headroom for a two-digit channel too,
   * which `-Werror=format-truncation` then proves fits inside `summary` below — the first size chosen
   * here was 40, and the compiler pointed out that 28 characters of prose plus 39 of list does not.
   */
  char samplingList[32] = {};
  std::size_t listUsed = 0;

  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    auto& dst = context_.sensors[i];
    const auto& src = sensors[i];
    dst.enabled = (connectedBitmap >> i) & 0x01;
    // Derived per frame from the configuration, which this function already receives — the parameter was
    // threaded through and marked unused. The snapshot is rebuilt every frame, so this is a projection
    // rather than a cache, and it cannot go stale the way SensorData::isReady did across a reboot.
    dst.ready = configIsValid(configs[i]);
    dst.instantFlow = src.instantFlow_L_min;
    dst.cumulativeLiters = src.cumulativeLiters;
    dst.sessionLiters = src.sessionLiters;
    dst.maxFlow = src.maxFlowSinceReset;

    // IN USE and no valid calibration — the state the row already renders as `SET?` (§4.4). Read off
    // the two projections just assigned, so the summary cannot disagree with the rows beside it.
    if (dst.enabled && !dst.ready) {
      context_.uncalibratedCount++;
    }

    if ((warningFlags >> i) & 0x01) {
      if (listUsed + 6 < sizeof(samplingList)) {
        listUsed += static_cast<std::size_t>(std::snprintf(samplingList + listUsed,
                                                           sizeof(samplingList) - listUsed,
                                                           listUsed ? ", %u" : "%u",
                                                           static_cast<unsigned>(i + 1)));
      }
      context_.warningCount++;
    }
  }

  /**
   * One summary line, one precedence rule, and two consumers that therefore cannot disagree: the
   * warning banner prints this string and so does `legend.warning`.
   *
   * UNCALIBRATED OUTRANKS UNDER-SAMPLING. A device nobody has finished commissioning is the more
   * urgent fact — the readings are not merely suspect, there are none — so it takes the line. Before
   * this, the line read "All sensors nominal" on a device whose channels all sat at `SET?`, because
   * the only input was `REG_UNDERSAMPLING_FLAGS`, which an uncalibrated channel cannot set:
   * `evaluateSensorDiagnostics` tests `valid && !meetsNyquistLimit`, so an invalid configuration is
   * skipped by the very check that would have reported it (modbus_manager.cpp).
   *
   * WHEN BOTH ARE PRESENT THE CHANNEL LIST IS TRADED FOR A COUNT, and so is the word "channels".
   * Naming both sets needs more than the 37 characters the banner has at x=16 with 6 px glyphs, and
   * "8 channels not calibrated, 8 undersampling" is 42 — five over, which is why this line is the one
   * place the noun is dropped. `telemetry.status` keeps it in every state, having 40 columns from x=2
   * and no channel list to carry. Channel identity is not lost either way: the flagged rows are drawn
   * in the warning colour from `warningFlags` and an uncalibrated row says `SET?` itself, so the panel
   * still says WHICH — this row says how many and which kind.
   *
   * `No channels in use` is not a cosmetic fifth case: the connected bitmap comes out of NVS with a
   * default of 0 (firmware.cpp), so a factory-fresh device has nothing in use at all, and both counts
   * are legitimately zero. "All sensors nominal" for a device with no sensors is the same vacuous claim
   * this change exists to delete — `telemetry.maxFlowLpm` already refuses it with `Max Flow: --`, and
   * `SensorStateEngine` already refuses it for the green LED (`activeSensors == 0` clears allReady).
   */
  char summary[64] = {};
  if (context_.uncalibratedCount > 0 && context_.warningCount > 0) {
    std::snprintf(summary, sizeof(summary), "%u not calibrated, %u undersampling",
                  static_cast<unsigned>(context_.uncalibratedCount),
                  static_cast<unsigned>(context_.warningCount));
  } else if (context_.uncalibratedCount > 0) {
    std::snprintf(summary, sizeof(summary), "%u channel%s not calibrated",
                  static_cast<unsigned>(context_.uncalibratedCount),
                  context_.uncalibratedCount == 1 ? "" : "s");
  } else if (context_.warningCount > 0) {
    std::snprintf(summary, sizeof(summary), "Sampling warning on sensors %s", samplingList);
  } else if (connectedBitmap == 0) {
    std::snprintf(summary, sizeof(summary), "No channels in use");
  } else {
    std::snprintf(summary, sizeof(summary), "All sensors nominal");
  }
  context_.warningSummary = summary;
}

void UiController::updateIdleState(uint32_t nowMs) {
  if (mode_ != UiMode::Idle) {
    if (nowMs - lastInteractionMs_ >= kIdleTimeoutMs) {
      // Route through enterIdle rather than setting mode_ directly: there is one way to
      // become idle, so the inactivity timeout and the deliberate gesture cannot drift
      // apart. They already had — the timeout skipped every reset the gesture performed.
      enterIdle(nowMs);
    }
  }
}

void UiController::openPackSelector(const char (*names)[ui::PackLoader::kMaxNameBytes],
                                    std::size_t count,
                                    const char* activeName,
                                    uint32_t nowMs) {
  // Rebuilt every time rather than cached: the card may have been changed since the page was last
  // opened, and a stale list would offer a pack that is no longer there.
  packSelector_.begin(names, count, activeName);
  // Any pending edit is discarded. The selector is reachable from inside an editor via §3.4.1's
  // gesture, and committing a half-typed value on the way out would be worse than losing it.
  endEdit();
  selectorActive_ = true;
  notifyInteraction(nowMs);
}

void UiController::closePackSelector(uint32_t nowMs) {
  selectorActive_ = false;
  notifyInteraction(nowMs);
}
