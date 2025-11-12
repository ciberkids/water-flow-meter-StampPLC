#include "GeneratedUi.h"

namespace ui_exporter {

static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewTitle_Text = { "Instant Flow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewValueTotal_Text = { "000.0 L/s", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewBadgeReady_Text = { "All sensors ready", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewSensor1_Text = { "S1 00.0", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewSensor2_Text = { "S2 00.0", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewSensor3_Text = { "S3 00.0", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewSensor4_Text = { "S4 00.0", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewSensor5_Text = { "S5 00.0", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewSensor6_Text = { "S6 00.0", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewSensor7_Text = { "S7 00.0", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewSensor8_Text = { "S8 00.0", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewLegendLed_Text = { "LED status legend", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoOverview_InfoOverviewLegendWarning_Text = { "Warnings", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };

static constexpr ui_exporter::Element kInfoOverviewElements[] = {
    { "title", ui_exporter::ElementType::Text, 4, 4, 0, 0, &kInfoOverview_InfoOverviewTitle_Text, nullptr, "page.title" },
    { "value-total", ui_exporter::ElementType::Value, 4, 20, 0, 0, &kInfoOverview_InfoOverviewValueTotal_Text, nullptr, "telemetry.total" },
    { "badge-ready", ui_exporter::ElementType::Badge, 4, 34, 0, 0, &kInfoOverview_InfoOverviewBadgeReady_Text, nullptr, "telemetry.status" },
    { "box-left", ui_exporter::ElementType::Box, 4, 48, 58, 60, nullptr, nullptr, nullptr },
    { "box-right", ui_exporter::ElementType::Box, 72, 48, 58, 60, nullptr, nullptr, nullptr },
    { "sensor-1", ui_exporter::ElementType::Text, 8, 52, 0, 0, &kInfoOverview_InfoOverviewSensor1_Text, nullptr, "sensor.1" },
    { "sensor-2", ui_exporter::ElementType::Text, 8, 64, 0, 0, &kInfoOverview_InfoOverviewSensor2_Text, nullptr, "sensor.2" },
    { "sensor-3", ui_exporter::ElementType::Text, 8, 76, 0, 0, &kInfoOverview_InfoOverviewSensor3_Text, nullptr, "sensor.3" },
    { "sensor-4", ui_exporter::ElementType::Text, 8, 88, 0, 0, &kInfoOverview_InfoOverviewSensor4_Text, nullptr, "sensor.4" },
    { "sensor-5", ui_exporter::ElementType::Text, 76, 52, 0, 0, &kInfoOverview_InfoOverviewSensor5_Text, nullptr, "sensor.5" },
    { "sensor-6", ui_exporter::ElementType::Text, 76, 64, 0, 0, &kInfoOverview_InfoOverviewSensor6_Text, nullptr, "sensor.6" },
    { "sensor-7", ui_exporter::ElementType::Text, 76, 76, 0, 0, &kInfoOverview_InfoOverviewSensor7_Text, nullptr, "sensor.7" },
    { "sensor-8", ui_exporter::ElementType::Text, 76, 88, 0, 0, &kInfoOverview_InfoOverviewSensor8_Text, nullptr, "sensor.8" },
    { "legend-led", ui_exporter::ElementType::Text, 0, 138, 0, 0, &kInfoOverview_InfoOverviewLegendLed_Text, nullptr, "legend.led" },
    { "legend-warning", ui_exporter::ElementType::Text, 0, 150, 0, 0, &kInfoOverview_InfoOverviewLegendWarning_Text, nullptr, "legend.warning" },
    { "propeller", ui_exporter::ElementType::Icon, 210, 48, 44, 44, nullptr, "propeller", nullptr }
};


static constexpr ui_exporter::Flow kInfoOverviewFlows[] = {
    { "info-nav-prev", "UP switches to previous page", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "info-nav-next", "DOWN switches to next page", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "info-enter-config", "ENTER opens configuration", "configuration", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.mode.configuration", nullptr, 0 },
    { "info-enter-idle", "ENTER long press idles display", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.mode.idle", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfiguration_ConfigurationTitle_Text = { "Configuration", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfiguration_ConfigurationInfo_Text = { "Select an option:", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfiguration_ConfigurationOption1_Text = { "Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfiguration_ConfigurationOption2_Text = { "LED Pulse", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfiguration_ConfigurationOption3_Text = { "Nyquist Warnings", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };

static constexpr ui_exporter::Element kConfigurationElements[] = {
    { "title", ui_exporter::ElementType::Text, 4, 4, 0, 0, &kConfiguration_ConfigurationTitle_Text, nullptr, "config.title" },
    { "info", ui_exporter::ElementType::Text, 4, 16, 0, 0, &kConfiguration_ConfigurationInfo_Text, nullptr, nullptr },
    { "option-1", ui_exporter::ElementType::Badge, 4, 32, 0, 0, &kConfiguration_ConfigurationOption1_Text, nullptr, nullptr },
    { "option-2", ui_exporter::ElementType::Badge, 4, 46, 0, 0, &kConfiguration_ConfigurationOption2_Text, nullptr, nullptr },
    { "option-3", ui_exporter::ElementType::Badge, 4, 60, 0, 0, &kConfiguration_ConfigurationOption3_Text, nullptr, nullptr },
    { "decor", ui_exporter::ElementType::Icon, 115, 16, 0, 0, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfiguration_Flow2ActionParams[] = {
    { "scope", "ui" }
};

static constexpr ui_exporter::Flow kConfigurationFlows[] = {
    { "config-nav-prev", "UP selects previous field", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "config-nav-next", "DOWN selects next field", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "config-save", "ENTER saves configuration", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "core.action.save-config", kConfiguration_Flow2ActionParams, sizeof(kConfiguration_Flow2ActionParams) / sizeof(kConfiguration_Flow2ActionParams[0]) },
    { "config-exit", "ENTER hold returns to info", "info-overview", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.mode.info", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kCountdown_CountdownTitle_Text = { "Factory Reset", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kCountdown_CountdownMessage_Text = { "Hold buttons to continue...", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kCountdown_CountdownTimerValue_Text = { "30 s", ui_exporter::TextAlign::Center, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdown_CountdownHint_Text = { "Release to cancel", ui_exporter::TextAlign::Center, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kCountdownElements[] = {
    { "title", ui_exporter::ElementType::Text, 4, 4, 0, 0, &kCountdown_CountdownTitle_Text, nullptr, "countdown.title" },
    { "message", ui_exporter::ElementType::Text, 4, 20, 0, 0, &kCountdown_CountdownMessage_Text, nullptr, "countdown.message" },
    { "timer-box", ui_exporter::ElementType::Box, 18, 48, 100, 60, nullptr, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 68, 72, 0, 0, &kCountdown_CountdownTimerValue_Text, nullptr, "countdown.value" },
    { "hint", ui_exporter::ElementType::Text, 68, 90, 0, 0, &kCountdown_CountdownHint_Text, nullptr, "countdown.hint" }
};


const ui_exporter::Screen kGeneratedScreens[] = {
    { "info-overview", "Info Overview", kInfoOverviewElements, sizeof(kInfoOverviewElements) / sizeof(kInfoOverviewElements[0]), kInfoOverviewFlows, sizeof(kInfoOverviewFlows) / sizeof(kInfoOverviewFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "configuration", "Configuration Menu", kConfigurationElements, sizeof(kConfigurationElements) / sizeof(kConfigurationElements[0]), kConfigurationFlows, sizeof(kConfigurationFlows) / sizeof(kConfigurationFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "countdown", "Reset Countdown", kCountdownElements, sizeof(kCountdownElements) / sizeof(kCountdownElements[0]), nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0 }
};

static constexpr ui_exporter::ThemeColor kThemeColors[] = {
    { "displayBackground", 0xFF000A17u },
    { "textPrimary", 0xFFF5FAFFu },
    { "textMuted", 0xFF9CAEC6u },
    { "textStrong", 0xFFFFFFFFu },
    { "value", 0xFF56D2FFu },
    { "badgeBackground", 0xFF0F1E33u },
    { "badgeBorder", 0xFF80A8C9u },
    { "icon", 0xFF56D2FFu },
    { "legend", 0xFF85BBE8u },
    { "gridMinor", 0x477CA2CEu },
    { "gridMajor", 0x8C7CA2CEu }
};

const std::size_t kGeneratedScreenCount = sizeof(kGeneratedScreens) / sizeof(kGeneratedScreens[0]);

const ui_exporter::Theme kGeneratedTheme = {
    "StampPLC Default", kThemeColors, sizeof(kThemeColors) / sizeof(kThemeColors[0]), 8, 10, 8, "ease-in-out"
};

const ui_exporter::Metadata kGeneratedMetadata = {
    "2025-11-12T10:11:07.976Z", 3, 27
};

}  // namespace ui_exporter
