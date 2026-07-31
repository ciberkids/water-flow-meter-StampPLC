#pragma once

#include <cstddef>
#include <cstdint>

/**
 * The info pages reachable by cycling UP/DOWN at the root of the navigation tree
 * (Display_UI_Requirements §5).
 *
 * The enum and the screen ids it maps to live in the same file on purpose. They were
 * previously in separate translation units tied together by a comment, and they drifted:
 * the router defaulted to screen ids the generated table did not contain, so the display
 * came up blank with nothing reporting a fault.
 */
enum class UiPage : uint8_t {
  GlobalStatus = 0,
  InstantFlow,
  CumulativeLiters,
  CumulativeCubicMeters,
  SessionLiters,
  SessionCubicMeters,
  MaxFlow,
  EnterConfiguration,
  FactoryReset,
  Count
};

/** A screen id the firmware looks up by name, with the role it fills. */
struct ScreenRole {
  const char* id;
  const char* role;
};

inline constexpr const char* kInfoScreenIds[] = {
    "info-p0-global-status",       // UiPage::GlobalStatus
    "info-p1-instant-flow",        // UiPage::InstantFlow
    "info-p2-cumulative-liters",   // UiPage::CumulativeLiters
    "info-p3-cumulative-m3",       // UiPage::CumulativeCubicMeters
    "info-p4-session-liters",      // UiPage::SessionLiters
    "info-p5-session-m3",          // UiPage::SessionCubicMeters
    "info-p6-max-flow",            // UiPage::MaxFlow
    "info-p7-enter-config",        // UiPage::EnterConfiguration
    "info-p8-factory-reset",       // UiPage::FactoryReset
};

static_assert(sizeof(kInfoScreenIds) / sizeof(kInfoScreenIds[0]) ==
                  static_cast<std::size_t>(UiPage::Count),
              "kInfoScreenIds must have one entry per UiPage");

inline constexpr const char* kConfigurationScreenId = "config-c1-modbus-id";
inline constexpr const char* kFactoryResetCountdownScreenId = "confirm-factory-reset";

/** Every screen id the firmware resolves by name, for the generated manifest. */
inline constexpr ScreenRole kRequiredScreens[] = {
    {kInfoScreenIds[0], "info-page-0 (UiPage::GlobalStatus)"},
    {kInfoScreenIds[1], "info-page-1 (UiPage::InstantFlow)"},
    {kInfoScreenIds[2], "info-page-2 (UiPage::CumulativeLiters)"},
    {kInfoScreenIds[3], "info-page-3 (UiPage::CumulativeCubicMeters)"},
    {kInfoScreenIds[4], "info-page-4 (UiPage::SessionLiters)"},
    {kInfoScreenIds[5], "info-page-5 (UiPage::SessionCubicMeters)"},
    {kInfoScreenIds[6], "info-page-6 (UiPage::MaxFlow)"},
    {kInfoScreenIds[7], "info-page-7 (UiPage::EnterConfiguration)"},
    {kConfigurationScreenId, "configuration-entry"},
    {kInfoScreenIds[8], "info-page-8 (UiPage::FactoryReset)"},
    {kFactoryResetCountdownScreenId, "countdown-overlay"}};

inline constexpr std::size_t kRequiredScreenCount =
    sizeof(kRequiredScreens) / sizeof(kRequiredScreens[0]);

