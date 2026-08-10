#include "GeneratedUi.h"

namespace ui_exporter {

static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusHdrTitle_Text = { "System Status", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusTotalFlowLabel_Text = { "Total Current Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusTotalFlowValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusSessionTotalLabel_Text = { "Since reset (m3)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusSessionTotalValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusMaxFlow_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusNetLedStatus_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusFooterHint_Text = { "UP/DN pages   UP+DN off", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP0GlobalStatusElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusHdrTitle_Text, nullptr, nullptr },
    { "total-flow-label", ui_exporter::ElementType::Text, 2, 14, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusTotalFlowLabel_Text, nullptr, nullptr },
    { "total-flow-value", ui_exporter::ElementType::Value, 152, 14, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusTotalFlowValue_Text, nullptr, "telemetry.totalFlowLpm" },
    { "flow-dots", ui_exporter::ElementType::Icon, 60, 30, 120, 40, nullptr, "flow-dots", nullptr },
    { "session-total-label", ui_exporter::ElementType::Text, 2, 80, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusSessionTotalLabel_Text, nullptr, nullptr },
    { "session-total-value", ui_exporter::ElementType::Value, 153, 80, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusSessionTotalValue_Text, nullptr, "telemetry.totalVolumeM3" },
    { "max-flow", ui_exporter::ElementType::Text, 2, 92, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusMaxFlow_Text, nullptr, "telemetry.maxFlowLpm" },
    { "net-led-status", ui_exporter::ElementType::Text, 2, 108, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusNetLedStatus_Text, nullptr, "legend.status" },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP0GlobalStatusFlows[] = {
    { "f-next", "Next page", "info-p1-instant-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous page", "net-mqtt-root", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowHdrTitle_Text = { "Instant Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS4Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS5Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS6Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS7Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS8Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowFooterHint_Text = { "UP/DN pages   UP+DN off", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP1InstantFlowElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowHdrTitle_Text, nullptr, nullptr },
    { "s1-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS1Value_Text, nullptr, "sensor.1.instantFlow" },
    { "s2-value", ui_exporter::ElementType::Value, 2, 44, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS2Value_Text, nullptr, "sensor.2.instantFlow" },
    { "s3-value", ui_exporter::ElementType::Value, 2, 64, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS3Value_Text, nullptr, "sensor.3.instantFlow" },
    { "s4-value", ui_exporter::ElementType::Value, 2, 84, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS4Value_Text, nullptr, "sensor.4.instantFlow" },
    { "s5-value", ui_exporter::ElementType::Value, 114, 24, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS5Value_Text, nullptr, "sensor.5.instantFlow" },
    { "s6-value", ui_exporter::ElementType::Value, 114, 44, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS6Value_Text, nullptr, "sensor.6.instantFlow" },
    { "s7-value", ui_exporter::ElementType::Value, 114, 64, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS7Value_Text, nullptr, "sensor.7.instantFlow" },
    { "s8-value", ui_exporter::ElementType::Value, 114, 84, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS8Value_Text, nullptr, "sensor.8.instantFlow" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP1InstantFlowFlows[] = {
    { "f-next", "Next page", "info-p2-cumulative-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous page", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP2CumulativeM3_InfoP2CumulativeM3HdrTitle_Text = { "Cumulative (m3)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeM3_InfoP2CumulativeM3S1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeM3_InfoP2CumulativeM3S2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeM3_InfoP2CumulativeM3S3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeM3_InfoP2CumulativeM3S4Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeM3_InfoP2CumulativeM3S5Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeM3_InfoP2CumulativeM3S6Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeM3_InfoP2CumulativeM3S7Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeM3_InfoP2CumulativeM3S8Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeM3_InfoP2CumulativeM3FooterHint_Text = { "ENTER reset totals (hold 3s)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP2CumulativeM3Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP2CumulativeM3_InfoP2CumulativeM3HdrTitle_Text, nullptr, nullptr },
    { "s1-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kInfoP2CumulativeM3_InfoP2CumulativeM3S1Value_Text, nullptr, "sensor.1.cumulativeM3" },
    { "s2-value", ui_exporter::ElementType::Value, 2, 44, 0, 0, &kInfoP2CumulativeM3_InfoP2CumulativeM3S2Value_Text, nullptr, "sensor.2.cumulativeM3" },
    { "s3-value", ui_exporter::ElementType::Value, 2, 64, 0, 0, &kInfoP2CumulativeM3_InfoP2CumulativeM3S3Value_Text, nullptr, "sensor.3.cumulativeM3" },
    { "s4-value", ui_exporter::ElementType::Value, 2, 84, 0, 0, &kInfoP2CumulativeM3_InfoP2CumulativeM3S4Value_Text, nullptr, "sensor.4.cumulativeM3" },
    { "s5-value", ui_exporter::ElementType::Value, 114, 24, 0, 0, &kInfoP2CumulativeM3_InfoP2CumulativeM3S5Value_Text, nullptr, "sensor.5.cumulativeM3" },
    { "s6-value", ui_exporter::ElementType::Value, 114, 44, 0, 0, &kInfoP2CumulativeM3_InfoP2CumulativeM3S6Value_Text, nullptr, "sensor.6.cumulativeM3" },
    { "s7-value", ui_exporter::ElementType::Value, 114, 64, 0, 0, &kInfoP2CumulativeM3_InfoP2CumulativeM3S7Value_Text, nullptr, "sensor.7.cumulativeM3" },
    { "s8-value", ui_exporter::ElementType::Value, 114, 84, 0, 0, &kInfoP2CumulativeM3_InfoP2CumulativeM3S8Value_Text, nullptr, "sensor.8.cumulativeM3" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP2CumulativeM3_InfoP2CumulativeM3FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP2CumulativeM3Flows[] = {
    { "f-next", "Next page", "info-p3-session-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous page", "info-p1-instant-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open", "confirm-reset-totals", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP3SessionM3_InfoP3SessionM3HdrTitle_Text = { "Session (m3)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP3SessionM3_InfoP3SessionM3S1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3SessionM3_InfoP3SessionM3S2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3SessionM3_InfoP3SessionM3S3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3SessionM3_InfoP3SessionM3S4Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3SessionM3_InfoP3SessionM3S5Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3SessionM3_InfoP3SessionM3S6Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3SessionM3_InfoP3SessionM3S7Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3SessionM3_InfoP3SessionM3S8Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3SessionM3_InfoP3SessionM3FooterHint_Text = { "ENTER reset session (hold 3s)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP3SessionM3Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP3SessionM3_InfoP3SessionM3HdrTitle_Text, nullptr, nullptr },
    { "s1-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kInfoP3SessionM3_InfoP3SessionM3S1Value_Text, nullptr, "sensor.1.sessionM3" },
    { "s2-value", ui_exporter::ElementType::Value, 2, 44, 0, 0, &kInfoP3SessionM3_InfoP3SessionM3S2Value_Text, nullptr, "sensor.2.sessionM3" },
    { "s3-value", ui_exporter::ElementType::Value, 2, 64, 0, 0, &kInfoP3SessionM3_InfoP3SessionM3S3Value_Text, nullptr, "sensor.3.sessionM3" },
    { "s4-value", ui_exporter::ElementType::Value, 2, 84, 0, 0, &kInfoP3SessionM3_InfoP3SessionM3S4Value_Text, nullptr, "sensor.4.sessionM3" },
    { "s5-value", ui_exporter::ElementType::Value, 114, 24, 0, 0, &kInfoP3SessionM3_InfoP3SessionM3S5Value_Text, nullptr, "sensor.5.sessionM3" },
    { "s6-value", ui_exporter::ElementType::Value, 114, 44, 0, 0, &kInfoP3SessionM3_InfoP3SessionM3S6Value_Text, nullptr, "sensor.6.sessionM3" },
    { "s7-value", ui_exporter::ElementType::Value, 114, 64, 0, 0, &kInfoP3SessionM3_InfoP3SessionM3S7Value_Text, nullptr, "sensor.7.sessionM3" },
    { "s8-value", ui_exporter::ElementType::Value, 114, 84, 0, 0, &kInfoP3SessionM3_InfoP3SessionM3S8Value_Text, nullptr, "sensor.8.sessionM3" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP3SessionM3_InfoP3SessionM3FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP3SessionM3Flows[] = {
    { "f-next", "Next page", "info-p4-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous page", "info-p2-cumulative-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open", "confirm-reset-session", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP4MaxFlow_InfoP4MaxFlowHdrTitle_Text = { "Max Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP4MaxFlow_InfoP4MaxFlowS1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4MaxFlow_InfoP4MaxFlowS2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4MaxFlow_InfoP4MaxFlowS3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4MaxFlow_InfoP4MaxFlowS4Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4MaxFlow_InfoP4MaxFlowS5Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4MaxFlow_InfoP4MaxFlowS6Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4MaxFlow_InfoP4MaxFlowS7Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4MaxFlow_InfoP4MaxFlowS8Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4MaxFlow_InfoP4MaxFlowFooterHint_Text = { "MAX = at sensor ceiling", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP4MaxFlowElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP4MaxFlow_InfoP4MaxFlowHdrTitle_Text, nullptr, nullptr },
    { "s1-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kInfoP4MaxFlow_InfoP4MaxFlowS1Value_Text, nullptr, "sensor.1.maxFlowSinceReset" },
    { "s2-value", ui_exporter::ElementType::Value, 2, 44, 0, 0, &kInfoP4MaxFlow_InfoP4MaxFlowS2Value_Text, nullptr, "sensor.2.maxFlowSinceReset" },
    { "s3-value", ui_exporter::ElementType::Value, 2, 64, 0, 0, &kInfoP4MaxFlow_InfoP4MaxFlowS3Value_Text, nullptr, "sensor.3.maxFlowSinceReset" },
    { "s4-value", ui_exporter::ElementType::Value, 2, 84, 0, 0, &kInfoP4MaxFlow_InfoP4MaxFlowS4Value_Text, nullptr, "sensor.4.maxFlowSinceReset" },
    { "s5-value", ui_exporter::ElementType::Value, 114, 24, 0, 0, &kInfoP4MaxFlow_InfoP4MaxFlowS5Value_Text, nullptr, "sensor.5.maxFlowSinceReset" },
    { "s6-value", ui_exporter::ElementType::Value, 114, 44, 0, 0, &kInfoP4MaxFlow_InfoP4MaxFlowS6Value_Text, nullptr, "sensor.6.maxFlowSinceReset" },
    { "s7-value", ui_exporter::ElementType::Value, 114, 64, 0, 0, &kInfoP4MaxFlow_InfoP4MaxFlowS7Value_Text, nullptr, "sensor.7.maxFlowSinceReset" },
    { "s8-value", ui_exporter::ElementType::Value, 114, 84, 0, 0, &kInfoP4MaxFlow_InfoP4MaxFlowS8Value_Text, nullptr, "sensor.8.maxFlowSinceReset" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP4MaxFlow_InfoP4MaxFlowFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP4MaxFlowFlows[] = {
    { "f-next", "Next page", "info-p5-enter-config", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous page", "info-p3-session-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open", "confirm-reset-session", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP5EnterConfig_InfoP5EnterConfigHdrTitle_Text = { "Configuration", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP5EnterConfig_InfoP5EnterConfigBody1_Text = { "Sensor calibration, Modbus", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP5EnterConfig_InfoP5EnterConfigBody2_Text = { "link, LED pulses, network.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP5EnterConfig_InfoP5EnterConfigFooterHint_Text = { "ENTER opens Configuration", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP5EnterConfigElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP5EnterConfig_InfoP5EnterConfigHdrTitle_Text, nullptr, nullptr },
    { "body-1", ui_exporter::ElementType::Text, 2, 28, 0, 0, &kInfoP5EnterConfig_InfoP5EnterConfigBody1_Text, nullptr, nullptr },
    { "body-2", ui_exporter::ElementType::Text, 2, 40, 0, 0, &kInfoP5EnterConfig_InfoP5EnterConfigBody2_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP5EnterConfig_InfoP5EnterConfigFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP5EnterConfigFlows[] = {
    { "f-next", "Next page", "info-p6-factory-reset", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous page", "info-p4-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningTitle_Text = { "! Sampling too slow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningDetail1_Text = { "f_theoretical exceeds", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningDetail2_Text = { "Nyquist limit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningNyquistValue_Text = { "-", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningOptionUp_Text = { "UP = Edit values", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningOptionDown_Text = { "DOWN = Save anyway", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningFooterHint_Text = { "Choose UP or DOWN", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNyquistWarningElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 240, 135, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 5, 40, 0, 0, &kNyquistWarning_NyquistWarningTitle_Text, nullptr, nullptr },
    { "detail-1", ui_exporter::ElementType::Text, 5, 60, 0, 0, &kNyquistWarning_NyquistWarningDetail1_Text, nullptr, nullptr },
    { "detail-2", ui_exporter::ElementType::Text, 5, 72, 0, 0, &kNyquistWarning_NyquistWarningDetail2_Text, nullptr, nullptr },
    { "nyquist-value", ui_exporter::ElementType::Value, 5, 88, 0, 0, &kNyquistWarning_NyquistWarningNyquistValue_Text, nullptr, "config.sensor.nyquistWarning" },
    { "option-up", ui_exporter::ElementType::Text, 5, 100, 0, 0, &kNyquistWarning_NyquistWarningOptionUp_Text, nullptr, nullptr },
    { "option-down", ui_exporter::ElementType::Text, 5, 112, 0, 0, &kNyquistWarning_NyquistWarningOptionDown_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 5, 124, 0, 0, &kNyquistWarning_NyquistWarningFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNyquistWarningFlows[] = {
    { "f-edit", "Return to edit", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-force-save", "Force save (ignore warning)", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit-override", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kStateIdle_StateIdleIdlePlaceholder_Text = { "- Display off -", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kStateIdleElements[] = {
    { "idle-placeholder", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kStateIdle_StateIdleIdlePlaceholder_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kStateIdleFlows[] = {
    { "f-wake", "Wake to info", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-wake-up", "Wake to info", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-wake-down", "Wake to info", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdHdrTitle_Text = { "Config > Modbus ID", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC1ModbusIdElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdNavPosition_Text, nullptr, "nav.position" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdFieldValue_Text, nullptr, "config.modbusSlaveId" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdRangeHint_Text, nullptr, "config.editor.range" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC1ModbusIdFlows[] = {
    { "f-next", "Next entry", "config-c2-baud-rate", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-root-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c1-modbus-id-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateHdrTitle_Text = { "Config > Baud Rate", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC2BaudRateElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateNavPosition_Text, nullptr, "nav.position" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateFieldValue_Text, nullptr, "config.baudRate" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateRangeHint_Text, nullptr, "config.editor.range" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC2BaudRateFlows[] = {
    { "f-next", "Next entry", "config-c3-parity", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c2-baud-rate-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityHdrTitle_Text = { "Config > Parity", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC3ParityElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC3Parity_ConfigC3ParityHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC3Parity_ConfigC3ParityNavPosition_Text, nullptr, "nav.position" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigC3Parity_ConfigC3ParityFieldValue_Text, nullptr, "config.parity" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC3Parity_ConfigC3ParityRangeHint_Text, nullptr, "config.editor.range" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC3Parity_ConfigC3ParityFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC3ParityFlows[] = {
    { "f-next", "Next entry", "config-c4-stop-bits", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c2-baud-rate", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c3-parity-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsHdrTitle_Text = { "Config > Stop Bits", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC4StopBitsElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsNavPosition_Text, nullptr, "nav.position" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsFieldValue_Text, nullptr, "config.stopBits" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsRangeHint_Text, nullptr, "config.editor.range" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC4StopBitsFlows[] = {
    { "f-next", "Next entry", "config-c5-led-pulse-vol", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c3-parity", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c4-stop-bits-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolHdrTitle_Text = { "Config > LED Pulse Volume", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC5LedPulseVolElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolNavPosition_Text, nullptr, "nav.position" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolFieldValue_Text, nullptr, "config.ledPulseVolume" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolRangeHint_Text, nullptr, "config.editor.range" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC5LedPulseVolFlows[] = {
    { "f-next", "Next entry", "config-c6-led-pulse-period", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c4-stop-bits", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c5-led-pulse-vol-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodHdrTitle_Text = { "Config > LED Pulse Period", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC6LedPulsePeriodElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodNavPosition_Text, nullptr, "nav.position" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFieldValue_Text, nullptr, "config.ledPulsePeriod" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodRangeHint_Text, nullptr, "config.editor.range" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC6LedPulsePeriodFlows[] = {
    { "f-next", "Next entry", "config-c7-sensor-select", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c5-led-pulse-vol", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c6-led-pulse-period-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectHdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectBody1_Text = { "Channels 1-8", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectBody2_Text = { "Connection and calibration,", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectBody3_Text = { "per channel.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectFooterHint_Text = { "UP/DN pages  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC7SensorSelectElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectNavPosition_Text, nullptr, "nav.position" },
    { "body-1", ui_exporter::ElementType::Text, 2, 30, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectBody1_Text, nullptr, nullptr },
    { "body-2", ui_exporter::ElementType::Text, 2, 50, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectBody2_Text, nullptr, nullptr },
    { "body-3", ui_exporter::ElementType::Text, 2, 62, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectBody3_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC7SensorSelectFlows[] = {
    { "f-next", "Next entry", "config-root-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c6-led-pulse-period", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor list", "config-sensor-1", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigRootBack_ConfigRootBackHdrTitle_Text = { "Config", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigRootBack_ConfigRootBackNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigRootBack_ConfigRootBackBackLabel_Text = { "< BACK", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigRootBack_ConfigRootBackFooterHint_Text = { "ENTER go back", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigRootBackElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigRootBack_ConfigRootBackHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigRootBack_ConfigRootBackNavPosition_Text, nullptr, "nav.position" },
    { "back-label", ui_exporter::ElementType::Text, 2, 24, 0, 0, &kConfigRootBack_ConfigRootBackBackLabel_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigRootBack_ConfigRootBackFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigRootBackFlows[] = {
    { "f-next", "Next entry", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c7-sensor-select", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-back", "Back one level", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditHdrTitle_Text = { "Edit > Modbus ID", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC1ModbusIdEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditSavedValue_Text, nullptr, "config.modbusSlaveId" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditRangeHint_Text, nullptr, "config.editor.range" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 86, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC1ModbusIdEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditHdrTitle_Text = { "Edit > Baud Rate", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC2BaudRateEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditSavedValue_Text, nullptr, "config.baudRate" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditRangeHint_Text, nullptr, "config.editor.range" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 86, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC2BaudRateEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c2-baud-rate", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c2-baud-rate", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditHdrTitle_Text = { "Edit > Parity", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC3ParityEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditSavedValue_Text, nullptr, "config.parity" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditRangeHint_Text, nullptr, "config.editor.range" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 86, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC3ParityEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c3-parity", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c3-parity", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditHdrTitle_Text = { "Edit > Stop Bits", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC4StopBitsEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditSavedValue_Text, nullptr, "config.stopBits" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditRangeHint_Text, nullptr, "config.editor.range" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 86, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC4StopBitsEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c4-stop-bits", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c4-stop-bits", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditHdrTitle_Text = { "Edit > LED Pulse Volume", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC5LedPulseVolEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditSavedValue_Text, nullptr, "config.ledPulseVolume" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditRangeHint_Text, nullptr, "config.editor.range" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 86, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC5LedPulseVolEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c5-led-pulse-vol", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c5-led-pulse-vol", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditHdrTitle_Text = { "Edit > LED Pulse Period", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC6LedPulsePeriodEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditSavedValue_Text, nullptr, "config.ledPulsePeriod" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditRangeHint_Text, nullptr, "config.editor.range" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 86, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC6LedPulsePeriodEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c6-led-pulse-period", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c6-led-pulse-period", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1ChannelLabel_Text = { "Sensor 1", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1StatusLabel_Text = { "Status", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1FlowLabel_Text = { "Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1FlowValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1FooterHint_Text = { "UP/DN channels  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor1Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigSensor1_ConfigSensor1HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigSensor1_ConfigSensor1NavPosition_Text, nullptr, "nav.position" },
    { "channel-label", ui_exporter::ElementType::Value, 2, 28, 0, 0, &kConfigSensor1_ConfigSensor1ChannelLabel_Text, nullptr, nullptr },
    { "status-label", ui_exporter::ElementType::Text, 2, 56, 0, 0, &kConfigSensor1_ConfigSensor1StatusLabel_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 62, 56, 0, 0, &kConfigSensor1_ConfigSensor1StatusValue_Text, nullptr, "sensor.1.status" },
    { "flow-label", ui_exporter::ElementType::Text, 2, 78, 0, 0, &kConfigSensor1_ConfigSensor1FlowLabel_Text, nullptr, nullptr },
    { "flow-value", ui_exporter::ElementType::Value, 68, 78, 0, 0, &kConfigSensor1_ConfigSensor1FlowValue_Text, nullptr, "sensor.1.instantFlow" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigSensor1_ConfigSensor1FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor1Flows[] = {
    { "f-next", "Next entry", "config-sensor-2", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 1 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2ChannelLabel_Text = { "Sensor 2", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2StatusLabel_Text = { "Status", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2FlowLabel_Text = { "Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2FlowValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2FooterHint_Text = { "UP/DN channels  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor2Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigSensor2_ConfigSensor2HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigSensor2_ConfigSensor2NavPosition_Text, nullptr, "nav.position" },
    { "channel-label", ui_exporter::ElementType::Value, 2, 28, 0, 0, &kConfigSensor2_ConfigSensor2ChannelLabel_Text, nullptr, nullptr },
    { "status-label", ui_exporter::ElementType::Text, 2, 56, 0, 0, &kConfigSensor2_ConfigSensor2StatusLabel_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 62, 56, 0, 0, &kConfigSensor2_ConfigSensor2StatusValue_Text, nullptr, "sensor.2.status" },
    { "flow-label", ui_exporter::ElementType::Text, 2, 78, 0, 0, &kConfigSensor2_ConfigSensor2FlowLabel_Text, nullptr, nullptr },
    { "flow-value", ui_exporter::ElementType::Value, 68, 78, 0, 0, &kConfigSensor2_ConfigSensor2FlowValue_Text, nullptr, "sensor.2.instantFlow" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigSensor2_ConfigSensor2FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor2Flows[] = {
    { "f-next", "Next entry", "config-sensor-3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-1", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 2 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3ChannelLabel_Text = { "Sensor 3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3StatusLabel_Text = { "Status", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3FlowLabel_Text = { "Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3FlowValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3FooterHint_Text = { "UP/DN channels  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor3Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigSensor3_ConfigSensor3HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigSensor3_ConfigSensor3NavPosition_Text, nullptr, "nav.position" },
    { "channel-label", ui_exporter::ElementType::Value, 2, 28, 0, 0, &kConfigSensor3_ConfigSensor3ChannelLabel_Text, nullptr, nullptr },
    { "status-label", ui_exporter::ElementType::Text, 2, 56, 0, 0, &kConfigSensor3_ConfigSensor3StatusLabel_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 62, 56, 0, 0, &kConfigSensor3_ConfigSensor3StatusValue_Text, nullptr, "sensor.3.status" },
    { "flow-label", ui_exporter::ElementType::Text, 2, 78, 0, 0, &kConfigSensor3_ConfigSensor3FlowLabel_Text, nullptr, nullptr },
    { "flow-value", ui_exporter::ElementType::Value, 68, 78, 0, 0, &kConfigSensor3_ConfigSensor3FlowValue_Text, nullptr, "sensor.3.instantFlow" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigSensor3_ConfigSensor3FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor3Flows[] = {
    { "f-next", "Next entry", "config-sensor-4", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-2", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 3 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4ChannelLabel_Text = { "Sensor 4", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4StatusLabel_Text = { "Status", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4FlowLabel_Text = { "Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4FlowValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4FooterHint_Text = { "UP/DN channels  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor4Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigSensor4_ConfigSensor4HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigSensor4_ConfigSensor4NavPosition_Text, nullptr, "nav.position" },
    { "channel-label", ui_exporter::ElementType::Value, 2, 28, 0, 0, &kConfigSensor4_ConfigSensor4ChannelLabel_Text, nullptr, nullptr },
    { "status-label", ui_exporter::ElementType::Text, 2, 56, 0, 0, &kConfigSensor4_ConfigSensor4StatusLabel_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 62, 56, 0, 0, &kConfigSensor4_ConfigSensor4StatusValue_Text, nullptr, "sensor.4.status" },
    { "flow-label", ui_exporter::ElementType::Text, 2, 78, 0, 0, &kConfigSensor4_ConfigSensor4FlowLabel_Text, nullptr, nullptr },
    { "flow-value", ui_exporter::ElementType::Value, 68, 78, 0, 0, &kConfigSensor4_ConfigSensor4FlowValue_Text, nullptr, "sensor.4.instantFlow" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigSensor4_ConfigSensor4FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor4Flows[] = {
    { "f-next", "Next entry", "config-sensor-5", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 4 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5ChannelLabel_Text = { "Sensor 5", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5StatusLabel_Text = { "Status", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5FlowLabel_Text = { "Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5FlowValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5FooterHint_Text = { "UP/DN channels  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor5Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigSensor5_ConfigSensor5HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigSensor5_ConfigSensor5NavPosition_Text, nullptr, "nav.position" },
    { "channel-label", ui_exporter::ElementType::Value, 2, 28, 0, 0, &kConfigSensor5_ConfigSensor5ChannelLabel_Text, nullptr, nullptr },
    { "status-label", ui_exporter::ElementType::Text, 2, 56, 0, 0, &kConfigSensor5_ConfigSensor5StatusLabel_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 62, 56, 0, 0, &kConfigSensor5_ConfigSensor5StatusValue_Text, nullptr, "sensor.5.status" },
    { "flow-label", ui_exporter::ElementType::Text, 2, 78, 0, 0, &kConfigSensor5_ConfigSensor5FlowLabel_Text, nullptr, nullptr },
    { "flow-value", ui_exporter::ElementType::Value, 68, 78, 0, 0, &kConfigSensor5_ConfigSensor5FlowValue_Text, nullptr, "sensor.5.instantFlow" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigSensor5_ConfigSensor5FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor5Flows[] = {
    { "f-next", "Next entry", "config-sensor-6", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-4", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 5 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6ChannelLabel_Text = { "Sensor 6", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6StatusLabel_Text = { "Status", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6FlowLabel_Text = { "Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6FlowValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6FooterHint_Text = { "UP/DN channels  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor6Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigSensor6_ConfigSensor6HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigSensor6_ConfigSensor6NavPosition_Text, nullptr, "nav.position" },
    { "channel-label", ui_exporter::ElementType::Value, 2, 28, 0, 0, &kConfigSensor6_ConfigSensor6ChannelLabel_Text, nullptr, nullptr },
    { "status-label", ui_exporter::ElementType::Text, 2, 56, 0, 0, &kConfigSensor6_ConfigSensor6StatusLabel_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 62, 56, 0, 0, &kConfigSensor6_ConfigSensor6StatusValue_Text, nullptr, "sensor.6.status" },
    { "flow-label", ui_exporter::ElementType::Text, 2, 78, 0, 0, &kConfigSensor6_ConfigSensor6FlowLabel_Text, nullptr, nullptr },
    { "flow-value", ui_exporter::ElementType::Value, 68, 78, 0, 0, &kConfigSensor6_ConfigSensor6FlowValue_Text, nullptr, "sensor.6.instantFlow" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigSensor6_ConfigSensor6FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor6Flows[] = {
    { "f-next", "Next entry", "config-sensor-7", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-5", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 6 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7ChannelLabel_Text = { "Sensor 7", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7StatusLabel_Text = { "Status", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7FlowLabel_Text = { "Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7FlowValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7FooterHint_Text = { "UP/DN channels  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor7Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigSensor7_ConfigSensor7HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigSensor7_ConfigSensor7NavPosition_Text, nullptr, "nav.position" },
    { "channel-label", ui_exporter::ElementType::Value, 2, 28, 0, 0, &kConfigSensor7_ConfigSensor7ChannelLabel_Text, nullptr, nullptr },
    { "status-label", ui_exporter::ElementType::Text, 2, 56, 0, 0, &kConfigSensor7_ConfigSensor7StatusLabel_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 62, 56, 0, 0, &kConfigSensor7_ConfigSensor7StatusValue_Text, nullptr, "sensor.7.status" },
    { "flow-label", ui_exporter::ElementType::Text, 2, 78, 0, 0, &kConfigSensor7_ConfigSensor7FlowLabel_Text, nullptr, nullptr },
    { "flow-value", ui_exporter::ElementType::Value, 68, 78, 0, 0, &kConfigSensor7_ConfigSensor7FlowValue_Text, nullptr, "sensor.7.instantFlow" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigSensor7_ConfigSensor7FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor7Flows[] = {
    { "f-next", "Next entry", "config-sensor-8", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-6", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 7 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8ChannelLabel_Text = { "Sensor 8", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8StatusLabel_Text = { "Status", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8FlowLabel_Text = { "Flow (L/m)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8FlowValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8FooterHint_Text = { "UP/DN channels  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor8Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigSensor8_ConfigSensor8HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigSensor8_ConfigSensor8NavPosition_Text, nullptr, "nav.position" },
    { "channel-label", ui_exporter::ElementType::Value, 2, 28, 0, 0, &kConfigSensor8_ConfigSensor8ChannelLabel_Text, nullptr, nullptr },
    { "status-label", ui_exporter::ElementType::Text, 2, 56, 0, 0, &kConfigSensor8_ConfigSensor8StatusLabel_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 62, 56, 0, 0, &kConfigSensor8_ConfigSensor8StatusValue_Text, nullptr, "sensor.8.status" },
    { "flow-label", ui_exporter::ElementType::Text, 2, 78, 0, 0, &kConfigSensor8_ConfigSensor8FlowLabel_Text, nullptr, nullptr },
    { "flow-value", ui_exporter::ElementType::Value, 68, 78, 0, 0, &kConfigSensor8_ConfigSensor8FlowValue_Text, nullptr, "sensor.8.instantFlow" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigSensor8_ConfigSensor8FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor8Flows[] = {
    { "f-next", "Next entry", "config-sensor-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-7", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 8 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensorBack_ConfigSensorBackHdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensorBack_ConfigSensorBackNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensorBack_ConfigSensorBackBackLabel_Text = { "< BACK", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensorBack_ConfigSensorBackFooterHint_Text = { "ENTER go back", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensorBackElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigSensorBack_ConfigSensorBackHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigSensorBack_ConfigSensorBackNavPosition_Text, nullptr, "nav.position" },
    { "back-label", ui_exporter::ElementType::Text, 2, 24, 0, 0, &kConfigSensorBack_ConfigSensorBackBackLabel_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigSensorBack_ConfigSensorBackFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensorBackFlows[] = {
    { "f-next", "Next entry", "config-sensor-1", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-8", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-back", "Back one level", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedHdrTitle_Text = { "Sensor > Connected", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedSensorIndex_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS1ConnectedElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS1Connected_ConfigS1ConnectedHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS1Connected_ConfigS1ConnectedNavPosition_Text, nullptr, "nav.position" },
    { "sensor-index", ui_exporter::ElementType::Value, 210, 2, 0, 0, &kConfigS1Connected_ConfigS1ConnectedSensorIndex_Text, nullptr, "config.selectedSensor" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigS1Connected_ConfigS1ConnectedFieldValue_Text, nullptr, "config.sensor.connected" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS1Connected_ConfigS1ConnectedRangeHint_Text, nullptr, "config.editor.range" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS1Connected_ConfigS1ConnectedNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS1Connected_ConfigS1ConnectedFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS1ConnectedFlows[] = {
    { "f-next", "Next entry", "config-s2-calibration", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-settings-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-s1-connected-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditHdrTitle_Text = { "Edit > Connected", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS1ConnectedEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditSavedValue_Text, nullptr, "config.sensor.connected" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditRangeHint_Text, nullptr, "config.editor.range" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 86, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS1ConnectedEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS2Calibration_ConfigS2CalibrationHdrTitle_Text = { "Sensor > Calibration", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS2Calibration_ConfigS2CalibrationNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2Calibration_ConfigS2CalibrationSensorIndex_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS2Calibration_ConfigS2CalibrationFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS2Calibration_ConfigS2CalibrationRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2Calibration_ConfigS2CalibrationContextNote_Text = { "Formula: F = m*Q + a.  Pulses/L: K p/L.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2Calibration_ConfigS2CalibrationNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS2Calibration_ConfigS2CalibrationFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS2CalibrationElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS2Calibration_ConfigS2CalibrationHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS2Calibration_ConfigS2CalibrationNavPosition_Text, nullptr, "nav.position" },
    { "sensor-index", ui_exporter::ElementType::Value, 210, 2, 0, 0, &kConfigS2Calibration_ConfigS2CalibrationSensorIndex_Text, nullptr, "config.selectedSensor" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigS2Calibration_ConfigS2CalibrationFieldValue_Text, nullptr, "config.sensor.calibrationType" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS2Calibration_ConfigS2CalibrationRangeHint_Text, nullptr, "config.editor.range" },
    { "context-note", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS2Calibration_ConfigS2CalibrationContextNote_Text, nullptr, nullptr },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 88, 0, 0, &kConfigS2Calibration_ConfigS2CalibrationNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS2Calibration_ConfigS2CalibrationFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS2CalibrationFlows[] = {
    { "f-next", "Next entry", "config-s3-pulses-per-l", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-s2-calibration-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS2CalibrationEdit_ConfigS2CalibrationEditHdrTitle_Text = { "Edit > Calibration", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS2CalibrationEdit_ConfigS2CalibrationEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2CalibrationEdit_ConfigS2CalibrationEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2CalibrationEdit_ConfigS2CalibrationEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS2CalibrationEdit_ConfigS2CalibrationEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2CalibrationEdit_ConfigS2CalibrationEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS2CalibrationEdit_ConfigS2CalibrationEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2CalibrationEdit_ConfigS2CalibrationEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS2CalibrationEdit_ConfigS2CalibrationEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS2CalibrationEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS2CalibrationEdit_ConfigS2CalibrationEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS2CalibrationEdit_ConfigS2CalibrationEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigS2CalibrationEdit_ConfigS2CalibrationEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigS2CalibrationEdit_ConfigS2CalibrationEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS2CalibrationEdit_ConfigS2CalibrationEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigS2CalibrationEdit_ConfigS2CalibrationEditSavedValue_Text, nullptr, "config.sensor.calibrationType" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS2CalibrationEdit_ConfigS2CalibrationEditRangeHint_Text, nullptr, "config.editor.range" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 88, 0, 0, &kConfigS2CalibrationEdit_ConfigS2CalibrationEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS2CalibrationEdit_ConfigS2CalibrationEditFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS2CalibrationEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-s2-calibration", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-s2-calibration", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS3PulsesPerL_ConfigS3PulsesPerLHdrTitle_Text = { "Sensor > Pulses per litre", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerL_ConfigS3PulsesPerLNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerL_ConfigS3PulsesPerLSensorIndex_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerL_ConfigS3PulsesPerLFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerL_ConfigS3PulsesPerLRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerL_ConfigS3PulsesPerLContextNote_Text = { "Used when Calibration is Pulses/L.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerL_ConfigS3PulsesPerLNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerL_ConfigS3PulsesPerLFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS3PulsesPerLElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS3PulsesPerL_ConfigS3PulsesPerLHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS3PulsesPerL_ConfigS3PulsesPerLNavPosition_Text, nullptr, "nav.position" },
    { "sensor-index", ui_exporter::ElementType::Value, 210, 2, 0, 0, &kConfigS3PulsesPerL_ConfigS3PulsesPerLSensorIndex_Text, nullptr, "config.selectedSensor" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigS3PulsesPerL_ConfigS3PulsesPerLFieldValue_Text, nullptr, "config.sensor.pulsesPerLiter" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS3PulsesPerL_ConfigS3PulsesPerLRangeHint_Text, nullptr, "config.editor.range" },
    { "context-note", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS3PulsesPerL_ConfigS3PulsesPerLContextNote_Text, nullptr, nullptr },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 88, 0, 0, &kConfigS3PulsesPerL_ConfigS3PulsesPerLNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS3PulsesPerL_ConfigS3PulsesPerLFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS3PulsesPerLFlows[] = {
    { "f-next", "Next entry", "config-s4-multiplier", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-s2-calibration", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-s3-pulses-per-l-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditHdrTitle_Text = { "Edit > Pulses per litre", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS3PulsesPerLEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditSavedValue_Text, nullptr, "config.sensor.pulsesPerLiter" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditRangeHint_Text, nullptr, "config.editor.range" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 88, 0, 0, &kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS3PulsesPerLEdit_ConfigS3PulsesPerLEditFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS3PulsesPerLEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-s3-pulses-per-l", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-s3-pulses-per-l", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierHdrTitle_Text = { "Sensor > Multiplier (F)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierSensorIndex_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierFEq_Text = { "F =", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierFMult_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierFQ_Text = { "*Q", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierFAdj_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierFRange_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4Multiplier_ConfigS4MultiplierFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS4MultiplierElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierNavPosition_Text, nullptr, "nav.position" },
    { "sensor-index", ui_exporter::ElementType::Value, 210, 2, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierSensorIndex_Text, nullptr, "config.selectedSensor" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierFieldValue_Text, nullptr, "config.sensor.multiplier" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierRangeHint_Text, nullptr, "config.editor.range" },
    { "f-eq", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierFEq_Text, nullptr, nullptr },
    { "f-mult", ui_exporter::ElementType::Value, 24, 66, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierFMult_Text, nullptr, "config.sensor.multiplier" },
    { "f-q", ui_exporter::ElementType::Text, 62, 66, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierFQ_Text, nullptr, nullptr },
    { "f-adj", ui_exporter::ElementType::Text, 80, 66, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierFAdj_Text, nullptr, "config.sensor.adjustTerm" },
    { "f-range", ui_exporter::ElementType::Text, 128, 66, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierFRange_Text, nullptr, "config.sensor.formulaQ" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 88, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS4Multiplier_ConfigS4MultiplierFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS4MultiplierFlows[] = {
    { "f-next", "Next entry", "config-s5-adjust", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-s3-pulses-per-l", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-s4-multiplier-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditHdrTitle_Text = { "Edit > Multiplier (F)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditFEq_Text = { "F =", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditFMult_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditFQ_Text = { "*Q", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditFAdj_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditFRange_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4MultiplierEdit_ConfigS4MultiplierEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS4MultiplierEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditSavedValue_Text, nullptr, "config.sensor.multiplier" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditRangeHint_Text, nullptr, "config.editor.range" },
    { "f-eq", ui_exporter::ElementType::Text, 2, 88, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditFEq_Text, nullptr, nullptr },
    { "f-mult", ui_exporter::ElementType::Value, 24, 88, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditFMult_Text, nullptr, "config.sensor.multiplier" },
    { "f-q", ui_exporter::ElementType::Text, 62, 88, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditFQ_Text, nullptr, nullptr },
    { "f-adj", ui_exporter::ElementType::Text, 80, 88, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditFAdj_Text, nullptr, "config.sensor.adjustTerm" },
    { "f-range", ui_exporter::ElementType::Text, 128, 88, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditFRange_Text, nullptr, "config.sensor.formulaQ" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 106, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS4MultiplierEdit_ConfigS4MultiplierEditFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS4MultiplierEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-s4-multiplier", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-s4-multiplier", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustHdrTitle_Text = { "Sensor > Adjust", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustSensorIndex_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustFEq_Text = { "F =", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustFMult_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustFQ_Text = { "*Q", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustFAdj_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustFRange_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS5Adjust_ConfigS5AdjustFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS5AdjustElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS5Adjust_ConfigS5AdjustHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS5Adjust_ConfigS5AdjustNavPosition_Text, nullptr, "nav.position" },
    { "sensor-index", ui_exporter::ElementType::Value, 210, 2, 0, 0, &kConfigS5Adjust_ConfigS5AdjustSensorIndex_Text, nullptr, "config.selectedSensor" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigS5Adjust_ConfigS5AdjustFieldValue_Text, nullptr, "config.sensor.adjust" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS5Adjust_ConfigS5AdjustRangeHint_Text, nullptr, "config.editor.range" },
    { "f-eq", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS5Adjust_ConfigS5AdjustFEq_Text, nullptr, nullptr },
    { "f-mult", ui_exporter::ElementType::Value, 24, 66, 0, 0, &kConfigS5Adjust_ConfigS5AdjustFMult_Text, nullptr, "config.sensor.multiplier" },
    { "f-q", ui_exporter::ElementType::Text, 62, 66, 0, 0, &kConfigS5Adjust_ConfigS5AdjustFQ_Text, nullptr, nullptr },
    { "f-adj", ui_exporter::ElementType::Text, 80, 66, 0, 0, &kConfigS5Adjust_ConfigS5AdjustFAdj_Text, nullptr, "config.sensor.adjustTerm" },
    { "f-range", ui_exporter::ElementType::Text, 128, 66, 0, 0, &kConfigS5Adjust_ConfigS5AdjustFRange_Text, nullptr, "config.sensor.formulaQ" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 88, 0, 0, &kConfigS5Adjust_ConfigS5AdjustNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS5Adjust_ConfigS5AdjustFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS5AdjustFlows[] = {
    { "f-next", "Next entry", "config-s6-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-s4-multiplier", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-s5-adjust-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditHdrTitle_Text = { "Edit > Adjust", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditFEq_Text = { "F =", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditFMult_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditFQ_Text = { "*Q", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditFAdj_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditFRange_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS5AdjustEdit_ConfigS5AdjustEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS5AdjustEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditSavedValue_Text, nullptr, "config.sensor.adjust" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditRangeHint_Text, nullptr, "config.editor.range" },
    { "f-eq", ui_exporter::ElementType::Text, 2, 88, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditFEq_Text, nullptr, nullptr },
    { "f-mult", ui_exporter::ElementType::Value, 24, 88, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditFMult_Text, nullptr, "config.sensor.multiplier" },
    { "f-q", ui_exporter::ElementType::Text, 62, 88, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditFQ_Text, nullptr, nullptr },
    { "f-adj", ui_exporter::ElementType::Text, 80, 88, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditFAdj_Text, nullptr, "config.sensor.adjustTerm" },
    { "f-range", ui_exporter::ElementType::Text, 128, 88, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditFRange_Text, nullptr, "config.sensor.formulaQ" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 106, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS5AdjustEdit_ConfigS5AdjustEditFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS5AdjustEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-s5-adjust", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-s5-adjust", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowHdrTitle_Text = { "Sensor > Max Flow (Q)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowSensorIndex_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowFEq_Text = { "F =", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowFMult_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowFQ_Text = { "*Q", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowFAdj_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowFRange_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlow_ConfigS6MaxFlowFooterHint_Text = { "UP/DN pages  ENTER edit  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS6MaxFlowElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowNavPosition_Text, nullptr, "nav.position" },
    { "sensor-index", ui_exporter::ElementType::Value, 210, 2, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowSensorIndex_Text, nullptr, "config.selectedSensor" },
    { "field-value", ui_exporter::ElementType::Value, 2, 24, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowFieldValue_Text, nullptr, "config.sensor.maxFlow" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowRangeHint_Text, nullptr, "config.editor.range" },
    { "f-eq", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowFEq_Text, nullptr, nullptr },
    { "f-mult", ui_exporter::ElementType::Value, 24, 66, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowFMult_Text, nullptr, "config.sensor.multiplier" },
    { "f-q", ui_exporter::ElementType::Text, 62, 66, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowFQ_Text, nullptr, nullptr },
    { "f-adj", ui_exporter::ElementType::Text, 80, 66, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowFAdj_Text, nullptr, "config.sensor.adjustTerm" },
    { "f-range", ui_exporter::ElementType::Text, 128, 66, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowFRange_Text, nullptr, "config.sensor.formulaQ" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 88, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS6MaxFlow_ConfigS6MaxFlowFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS6MaxFlowFlows[] = {
    { "f-next", "Next entry", "config-sensor-settings-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-s5-adjust", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-s6-max-flow-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditHdrTitle_Text = { "Edit > Max Flow (Q)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditPendingLabel_Text = { "New", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditRangeHint_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFEq_Text = { "F =", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFMult_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFQ_Text = { "*Q", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFAdj_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFRange_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold=cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS6MaxFlowEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditNavPosition_Text, nullptr, "nav.position" },
    { "pending-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 44, 26, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditPendingValue_Text, nullptr, "config.editor.pending" },
    { "saved-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 44, 44, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditSavedValue_Text, nullptr, "config.sensor.maxFlow" },
    { "range-hint", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditRangeHint_Text, nullptr, "config.editor.range" },
    { "f-eq", ui_exporter::ElementType::Text, 2, 88, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFEq_Text, nullptr, nullptr },
    { "f-mult", ui_exporter::ElementType::Value, 24, 88, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFMult_Text, nullptr, "config.sensor.multiplier" },
    { "f-q", ui_exporter::ElementType::Text, 62, 88, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFQ_Text, nullptr, nullptr },
    { "f-adj", ui_exporter::ElementType::Text, 80, 88, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFAdj_Text, nullptr, "config.sensor.adjustTerm" },
    { "f-range", ui_exporter::ElementType::Text, 128, 88, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFRange_Text, nullptr, "config.sensor.formulaQ" },
    { "nyquist-warning", ui_exporter::ElementType::Text, 2, 106, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigS6MaxFlowEdit_ConfigS6MaxFlowEditFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS6MaxFlowEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-s6-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-s6-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensorSettingsBack_ConfigSensorSettingsBackHdrTitle_Text = { "Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensorSettingsBack_ConfigSensorSettingsBackNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensorSettingsBack_ConfigSensorSettingsBackBackLabel_Text = { "< BACK", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensorSettingsBack_ConfigSensorSettingsBackFooterHint_Text = { "ENTER go back", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensorSettingsBackElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigSensorSettingsBack_ConfigSensorSettingsBackHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kConfigSensorSettingsBack_ConfigSensorSettingsBackNavPosition_Text, nullptr, "nav.position" },
    { "back-label", ui_exporter::ElementType::Text, 2, 24, 0, 0, &kConfigSensorSettingsBack_ConfigSensorSettingsBackBackLabel_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kConfigSensorSettingsBack_ConfigSensorSettingsBackFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensorSettingsBackFlows[] = {
    { "f-next", "Next entry", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-s6-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-back", "Back one level", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetWifiRoot_NetWifiRootHdrTitle_Text = { "WiFi", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiRoot_NetWifiRootNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiRoot_NetWifiRootLine1_Text = { "Radio, network name and", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetWifiRoot_NetWifiRootLine2_Text = { "passphrase.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetWifiRoot_NetWifiRootFooterHint_Text = { "UP/DN pages  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetWifiRootElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetWifiRoot_NetWifiRootHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetWifiRoot_NetWifiRootNavPosition_Text, nullptr, "nav.position" },
    { "line-1", ui_exporter::ElementType::Text, 2, 28, 0, 0, &kNetWifiRoot_NetWifiRootLine1_Text, nullptr, nullptr },
    { "line-2", ui_exporter::ElementType::Text, 2, 40, 0, 0, &kNetWifiRoot_NetWifiRootLine2_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetWifiRoot_NetWifiRootFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetWifiRootFlows[] = {
    { "f-next", "Next page", "net-mqtt-root", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous page", "info-p6-factory-reset", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open WiFi settings", "net-wifi-info", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetMqttRoot_NetMqttRootHdrTitle_Text = { "MQTT", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttRoot_NetMqttRootNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttRoot_NetMqttRootLine1_Text = { "Broker, credentials and", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetMqttRoot_NetMqttRootLine2_Text = { "Home Assistant discovery.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetMqttRoot_NetMqttRootFooterHint_Text = { "UP/DN pages  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetMqttRootElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetMqttRoot_NetMqttRootHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetMqttRoot_NetMqttRootNavPosition_Text, nullptr, "nav.position" },
    { "line-1", ui_exporter::ElementType::Text, 2, 28, 0, 0, &kNetMqttRoot_NetMqttRootLine1_Text, nullptr, nullptr },
    { "line-2", ui_exporter::ElementType::Text, 2, 40, 0, 0, &kNetMqttRoot_NetMqttRootLine2_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetMqttRoot_NetMqttRootFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetMqttRootFlows[] = {
    { "f-next", "Next page", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous page", "net-wifi-root", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open MQTT settings", "net-mqtt-info", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoHdrTitle_Text = { "WiFi > WiFi", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoRow0Label_Text = { "State", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoRow0Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoRow1Label_Text = { "Enabled", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoRow1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoRow2Label_Text = { "Network", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoRow2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoRow3Label_Text = { "Passphrase", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoRow3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiInfo_NetWifiInfoFooterHint_Text = { "UP/DN pages  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetWifiInfoElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetWifiInfo_NetWifiInfoHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetWifiInfo_NetWifiInfoNavPosition_Text, nullptr, "nav.position" },
    { "row0-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kNetWifiInfo_NetWifiInfoRow0Label_Text, nullptr, nullptr },
    { "row0-value", ui_exporter::ElementType::Text, 84, 26, 0, 0, &kNetWifiInfo_NetWifiInfoRow0Value_Text, nullptr, "net.wifi.state" },
    { "row1-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kNetWifiInfo_NetWifiInfoRow1Label_Text, nullptr, nullptr },
    { "row1-value", ui_exporter::ElementType::Text, 84, 44, 0, 0, &kNetWifiInfo_NetWifiInfoRow1Value_Text, nullptr, "config.wifi.enabled" },
    { "row2-label", ui_exporter::ElementType::Text, 2, 62, 0, 0, &kNetWifiInfo_NetWifiInfoRow2Label_Text, nullptr, nullptr },
    { "row2-value", ui_exporter::ElementType::Text, 84, 62, 0, 0, &kNetWifiInfo_NetWifiInfoRow2Value_Text, nullptr, "config.wifi.ssid" },
    { "row3-label", ui_exporter::ElementType::Text, 2, 80, 0, 0, &kNetWifiInfo_NetWifiInfoRow3Label_Text, nullptr, nullptr },
    { "row3-value", ui_exporter::ElementType::Text, 84, 80, 0, 0, &kNetWifiInfo_NetWifiInfoRow3Value_Text, nullptr, "config.wifi.psk" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetWifiInfo_NetWifiInfoFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetWifiInfoFlows[] = {
    { "f-next", "Next entry", "net-wifi-info-2", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "net-wifi-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetWifiInfo2_NetWifiInfo2HdrTitle_Text = { "WiFi > WiFi link", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetWifiInfo2_NetWifiInfo2NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiInfo2_NetWifiInfo2Row0Label_Text = { "Address", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiInfo2_NetWifiInfo2Row0Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiInfo2_NetWifiInfo2Row1Label_Text = { "Signal (dBm)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiInfo2_NetWifiInfo2Row1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiInfo2_NetWifiInfo2FooterHint_Text = { "UP/DN pages  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetWifiInfo2Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetWifiInfo2_NetWifiInfo2HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetWifiInfo2_NetWifiInfo2NavPosition_Text, nullptr, "nav.position" },
    { "row0-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kNetWifiInfo2_NetWifiInfo2Row0Label_Text, nullptr, nullptr },
    { "row0-value", ui_exporter::ElementType::Text, 84, 26, 0, 0, &kNetWifiInfo2_NetWifiInfo2Row0Value_Text, nullptr, "net.wifi.ip" },
    { "row1-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kNetWifiInfo2_NetWifiInfo2Row1Label_Text, nullptr, nullptr },
    { "row1-value", ui_exporter::ElementType::Text, 84, 44, 0, 0, &kNetWifiInfo2_NetWifiInfo2Row1Value_Text, nullptr, "net.wifi.rssi" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetWifiInfo2_NetWifiInfo2FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetWifiInfo2Flows[] = {
    { "f-next", "Next entry", "net-wifi-portal-reset", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "net-wifi-info", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetWifiPortalReset_NetWifiPortalResetHdrTitle_Text = { "WiFi > Reset portal login", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiPortalReset_NetWifiPortalResetNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiPortalReset_NetWifiPortalResetFooterHint_Text = { "UP/DN pages  ENTER open  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetWifiPortalResetElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetWifiPortalReset_NetWifiPortalResetHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetWifiPortalReset_NetWifiPortalResetNavPosition_Text, nullptr, "nav.position" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetWifiPortalReset_NetWifiPortalResetFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetWifiPortalResetFlows[] = {
    { "f-next", "Next entry", "net-wifi-ap-info", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "net-wifi-info-2", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open Reset portal login", "confirm-reset-portal-login", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetWifiBack_NetWifiBackHdrTitle_Text = { "WiFi", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiBack_NetWifiBackNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiBack_NetWifiBackBackLabel_Text = { "< BACK", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiBack_NetWifiBackFooterHint_Text = { "ENTER go back", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetWifiBackElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetWifiBack_NetWifiBackHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetWifiBack_NetWifiBackNavPosition_Text, nullptr, "nav.position" },
    { "back-label", ui_exporter::ElementType::Text, 2, 24, 0, 0, &kNetWifiBack_NetWifiBackBackLabel_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetWifiBack_NetWifiBackFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetWifiBackFlows[] = {
    { "f-next", "Next entry", "net-wifi-info", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "net-wifi-ap-info", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-back", "Back one level", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoHdrTitle_Text = { "WiFi > AP info", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoRow0Label_Text = { "AP network", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoRow0Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoRow1Label_Text = { "AP key", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoRow1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoRow2Label_Text = { "Browse to", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoRow2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoRow3Label_Text = { "Closes in (s)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoRow3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetWifiApInfo_NetWifiApInfoFooterHint_Text = { "UP/DN pages  hold=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetWifiApInfoElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetWifiApInfo_NetWifiApInfoHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetWifiApInfo_NetWifiApInfoNavPosition_Text, nullptr, "nav.position" },
    { "row0-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kNetWifiApInfo_NetWifiApInfoRow0Label_Text, nullptr, nullptr },
    { "row0-value", ui_exporter::ElementType::Text, 84, 26, 0, 0, &kNetWifiApInfo_NetWifiApInfoRow0Value_Text, nullptr, "net.ap.ssid" },
    { "row1-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kNetWifiApInfo_NetWifiApInfoRow1Label_Text, nullptr, nullptr },
    { "row1-value", ui_exporter::ElementType::Text, 84, 44, 0, 0, &kNetWifiApInfo_NetWifiApInfoRow1Value_Text, nullptr, "net.ap.password" },
    { "row2-label", ui_exporter::ElementType::Text, 2, 62, 0, 0, &kNetWifiApInfo_NetWifiApInfoRow2Label_Text, nullptr, nullptr },
    { "row2-value", ui_exporter::ElementType::Text, 84, 62, 0, 0, &kNetWifiApInfo_NetWifiApInfoRow2Value_Text, nullptr, "net.ap.ip" },
    { "row3-label", ui_exporter::ElementType::Text, 2, 80, 0, 0, &kNetWifiApInfo_NetWifiApInfoRow3Label_Text, nullptr, nullptr },
    { "row3-value", ui_exporter::ElementType::Text, 84, 80, 0, 0, &kNetWifiApInfo_NetWifiApInfoRow3Value_Text, nullptr, "net.portal.remaining" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetWifiApInfo_NetWifiApInfoFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetWifiApInfoFlows[] = {
    { "f-next", "Next entry", "net-wifi-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "net-wifi-portal-reset", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoHdrTitle_Text = { "MQTT > MQTT", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoRow0Label_Text = { "State", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoRow0Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoRow1Label_Text = { "Enabled", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoRow1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoRow2Label_Text = { "Broker", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoRow2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoRow3Label_Text = { "Port", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoRow3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo_NetMqttInfoFooterHint_Text = { "Set via web portal or RS485", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetMqttInfoElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetMqttInfo_NetMqttInfoHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetMqttInfo_NetMqttInfoNavPosition_Text, nullptr, "nav.position" },
    { "row0-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kNetMqttInfo_NetMqttInfoRow0Label_Text, nullptr, nullptr },
    { "row0-value", ui_exporter::ElementType::Text, 84, 26, 0, 0, &kNetMqttInfo_NetMqttInfoRow0Value_Text, nullptr, "net.mqtt.state" },
    { "row1-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kNetMqttInfo_NetMqttInfoRow1Label_Text, nullptr, nullptr },
    { "row1-value", ui_exporter::ElementType::Text, 84, 44, 0, 0, &kNetMqttInfo_NetMqttInfoRow1Value_Text, nullptr, "config.mqtt.enabled" },
    { "row2-label", ui_exporter::ElementType::Text, 2, 62, 0, 0, &kNetMqttInfo_NetMqttInfoRow2Label_Text, nullptr, nullptr },
    { "row2-value", ui_exporter::ElementType::Text, 84, 62, 0, 0, &kNetMqttInfo_NetMqttInfoRow2Value_Text, nullptr, "config.mqtt.host" },
    { "row3-label", ui_exporter::ElementType::Text, 2, 80, 0, 0, &kNetMqttInfo_NetMqttInfoRow3Label_Text, nullptr, nullptr },
    { "row3-value", ui_exporter::ElementType::Text, 84, 80, 0, 0, &kNetMqttInfo_NetMqttInfoRow3Value_Text, nullptr, "config.mqtt.port" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetMqttInfo_NetMqttInfoFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetMqttInfoFlows[] = {
    { "f-next", "Next entry", "net-mqtt-info-2", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "net-mqtt-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2HdrTitle_Text = { "MQTT > MQTT broker", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2Row0Label_Text = { "Username", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2Row0Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2Row1Label_Text = { "Password", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2Row1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2Row2Label_Text = { "Topic", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2Row2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2Row3Label_Text = { "HA prefix", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2Row3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo2_NetMqttInfo2FooterHint_Text = { "Set via web portal or RS485", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetMqttInfo2Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetMqttInfo2_NetMqttInfo2HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetMqttInfo2_NetMqttInfo2NavPosition_Text, nullptr, "nav.position" },
    { "row0-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kNetMqttInfo2_NetMqttInfo2Row0Label_Text, nullptr, nullptr },
    { "row0-value", ui_exporter::ElementType::Text, 84, 26, 0, 0, &kNetMqttInfo2_NetMqttInfo2Row0Value_Text, nullptr, "config.mqtt.user" },
    { "row1-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kNetMqttInfo2_NetMqttInfo2Row1Label_Text, nullptr, nullptr },
    { "row1-value", ui_exporter::ElementType::Text, 84, 44, 0, 0, &kNetMqttInfo2_NetMqttInfo2Row1Value_Text, nullptr, "config.mqtt.password" },
    { "row2-label", ui_exporter::ElementType::Text, 2, 62, 0, 0, &kNetMqttInfo2_NetMqttInfo2Row2Label_Text, nullptr, nullptr },
    { "row2-value", ui_exporter::ElementType::Text, 84, 62, 0, 0, &kNetMqttInfo2_NetMqttInfo2Row2Value_Text, nullptr, "config.mqtt.baseTopic" },
    { "row3-label", ui_exporter::ElementType::Text, 2, 80, 0, 0, &kNetMqttInfo2_NetMqttInfo2Row3Label_Text, nullptr, nullptr },
    { "row3-value", ui_exporter::ElementType::Text, 84, 80, 0, 0, &kNetMqttInfo2_NetMqttInfo2Row3Value_Text, nullptr, "config.mqtt.discoveryPrefix" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetMqttInfo2_NetMqttInfo2FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetMqttInfo2Flows[] = {
    { "f-next", "Next entry", "net-mqtt-info-3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "net-mqtt-info", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetMqttInfo3_NetMqttInfo3HdrTitle_Text = { "MQTT > MQTT publish", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNetMqttInfo3_NetMqttInfo3NavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo3_NetMqttInfo3Row0Label_Text = { "HA discovery", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo3_NetMqttInfo3Row0Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo3_NetMqttInfo3Row1Label_Text = { "Period", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo3_NetMqttInfo3Row1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo3_NetMqttInfo3Row2Label_Text = { "QoS", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttInfo3_NetMqttInfo3Row2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttInfo3_NetMqttInfo3FooterHint_Text = { "Set via web portal or RS485", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetMqttInfo3Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetMqttInfo3_NetMqttInfo3HdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetMqttInfo3_NetMqttInfo3NavPosition_Text, nullptr, "nav.position" },
    { "row0-label", ui_exporter::ElementType::Text, 2, 26, 0, 0, &kNetMqttInfo3_NetMqttInfo3Row0Label_Text, nullptr, nullptr },
    { "row0-value", ui_exporter::ElementType::Text, 84, 26, 0, 0, &kNetMqttInfo3_NetMqttInfo3Row0Value_Text, nullptr, "config.mqtt.haDiscovery" },
    { "row1-label", ui_exporter::ElementType::Text, 2, 44, 0, 0, &kNetMqttInfo3_NetMqttInfo3Row1Label_Text, nullptr, nullptr },
    { "row1-value", ui_exporter::ElementType::Text, 84, 44, 0, 0, &kNetMqttInfo3_NetMqttInfo3Row1Value_Text, nullptr, "config.mqtt.publishPeriod" },
    { "row2-label", ui_exporter::ElementType::Text, 2, 62, 0, 0, &kNetMqttInfo3_NetMqttInfo3Row2Label_Text, nullptr, nullptr },
    { "row2-value", ui_exporter::ElementType::Text, 84, 62, 0, 0, &kNetMqttInfo3_NetMqttInfo3Row2Value_Text, nullptr, "config.mqtt.qos" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetMqttInfo3_NetMqttInfo3FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetMqttInfo3Flows[] = {
    { "f-next", "Next entry", "net-mqtt-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "net-mqtt-info-2", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNetMqttBack_NetMqttBackHdrTitle_Text = { "MQTT", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttBack_NetMqttBackNavPosition_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNetMqttBack_NetMqttBackBackLabel_Text = { "< BACK", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNetMqttBack_NetMqttBackFooterHint_Text = { "ENTER go back", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNetMqttBackElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kNetMqttBack_NetMqttBackHdrTitle_Text, nullptr, nullptr },
    { "nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kNetMqttBack_NetMqttBackNavPosition_Text, nullptr, "nav.position" },
    { "back-label", ui_exporter::ElementType::Text, 2, 24, 0, 0, &kNetMqttBack_NetMqttBackBackLabel_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kNetMqttBack_NetMqttBackFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNetMqttBackFlows[] = {
    { "f-next", "Next entry", "net-mqtt-info", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "net-mqtt-info-3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-back", "Back one level", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP6FactoryReset_InfoP6FactoryResetHdrTitle_Text = { "Factory Reset", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP6FactoryReset_InfoP6FactoryResetWarning1_Text = { "Erases totals, calibration,", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP6FactoryReset_InfoP6FactoryResetWarning2_Text = { "LED, Modbus link and WiFi", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP6FactoryReset_InfoP6FactoryResetWarning3_Text = { "and MQTT credentials.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP6FactoryReset_InfoP6FactoryResetWarning4_Text = { "Re-provisioning needs the AP", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP6FactoryReset_InfoP6FactoryResetWarning5_Text = { "portal, at the device.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP6FactoryReset_InfoP6FactoryResetFooterHint_Text = { "ENTER opens confirm screen", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP6FactoryResetElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP6FactoryReset_InfoP6FactoryResetHdrTitle_Text, nullptr, nullptr },
    { "warning-1", ui_exporter::ElementType::Text, 2, 28, 0, 0, &kInfoP6FactoryReset_InfoP6FactoryResetWarning1_Text, nullptr, nullptr },
    { "warning-2", ui_exporter::ElementType::Text, 2, 40, 0, 0, &kInfoP6FactoryReset_InfoP6FactoryResetWarning2_Text, nullptr, nullptr },
    { "warning-3", ui_exporter::ElementType::Text, 2, 52, 0, 0, &kInfoP6FactoryReset_InfoP6FactoryResetWarning3_Text, nullptr, nullptr },
    { "warning-4", ui_exporter::ElementType::Text, 2, 72, 0, 0, &kInfoP6FactoryReset_InfoP6FactoryResetWarning4_Text, nullptr, nullptr },
    { "warning-5", ui_exporter::ElementType::Text, 2, 84, 0, 0, &kInfoP6FactoryReset_InfoP6FactoryResetWarning5_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP6FactoryReset_InfoP6FactoryResetFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP6FactoryResetFlows[] = {
    { "f-next", "Next page", "net-wifi-root", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous page", "info-p5-enter-config", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open confirm screen", "confirm-factory-reset", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfirmResetTotals_ConfirmResetTotalsTitle_Text = { "RESET TOTALS?", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfirmResetTotals_ConfirmResetTotalsWarning1_Text = { "Persistent cumulative volume", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfirmResetTotals_ConfirmResetTotalsWarning2_Text = { "cannot be recovered.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfirmResetTotals_ConfirmResetTotalsTimerValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfirmResetTotals_ConfirmResetTotalsFooterHint_Text = { "ENTER exit  hold ENTER confirm", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfirmResetTotalsElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 240, 135, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfirmResetTotals_ConfirmResetTotalsTitle_Text, nullptr, nullptr },
    { "warning-1", ui_exporter::ElementType::Text, 8, 48, 0, 0, &kConfirmResetTotals_ConfirmResetTotalsWarning1_Text, nullptr, nullptr },
    { "warning-2", ui_exporter::ElementType::Text, 8, 60, 0, 0, &kConfirmResetTotals_ConfirmResetTotalsWarning2_Text, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 104, 84, 0, 0, &kConfirmResetTotals_ConfirmResetTotalsTimerValue_Text, nullptr, "countdown.value" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfirmResetTotals_ConfirmResetTotalsFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfirmResetTotalsFlows[] = {
    { "f-exit", "Exit without acting", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-confirm", "Reset totals", "toast-totals-reset", ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 3000, nullptr, nullptr, nullptr, "core.action.reset-all-measured", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfirmResetSession_ConfirmResetSessionTitle_Text = { "RESET SESSION?", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfirmResetSession_ConfirmResetSessionWarning1_Text = { "Session totals and max flow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfirmResetSession_ConfirmResetSessionWarning2_Text = { "return to zero.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfirmResetSession_ConfirmResetSessionTimerValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfirmResetSession_ConfirmResetSessionFooterHint_Text = { "ENTER exit  hold ENTER confirm", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfirmResetSessionElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 240, 135, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfirmResetSession_ConfirmResetSessionTitle_Text, nullptr, nullptr },
    { "warning-1", ui_exporter::ElementType::Text, 8, 48, 0, 0, &kConfirmResetSession_ConfirmResetSessionWarning1_Text, nullptr, nullptr },
    { "warning-2", ui_exporter::ElementType::Text, 8, 60, 0, 0, &kConfirmResetSession_ConfirmResetSessionWarning2_Text, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 104, 84, 0, 0, &kConfirmResetSession_ConfirmResetSessionTimerValue_Text, nullptr, "countdown.value" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfirmResetSession_ConfirmResetSessionFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfirmResetSessionFlows[] = {
    { "f-exit", "Exit without acting", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-confirm", "Reset session", "toast-session-reset", ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 1500, nullptr, nullptr, nullptr, "core.action.reset-session", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfirmFactoryReset_ConfirmFactoryResetTitle_Text = { "FACTORY RESET?", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfirmFactoryReset_ConfirmFactoryResetWarning1_Text = { "Wipes NVS and reboots.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfirmFactoryReset_ConfirmFactoryResetWarning2_Text = { "This cannot be undone.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfirmFactoryReset_ConfirmFactoryResetTimerValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfirmFactoryReset_ConfirmFactoryResetFooterHint_Text = { "ENTER exit  hold ENTER confirm", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfirmFactoryResetElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 240, 135, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfirmFactoryReset_ConfirmFactoryResetTitle_Text, nullptr, nullptr },
    { "warning-1", ui_exporter::ElementType::Text, 8, 48, 0, 0, &kConfirmFactoryReset_ConfirmFactoryResetWarning1_Text, nullptr, nullptr },
    { "warning-2", ui_exporter::ElementType::Text, 8, 60, 0, 0, &kConfirmFactoryReset_ConfirmFactoryResetWarning2_Text, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 104, 84, 0, 0, &kConfirmFactoryReset_ConfirmFactoryResetTimerValue_Text, nullptr, "countdown.value" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfirmFactoryReset_ConfirmFactoryResetFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfirmFactoryResetFlows[] = {
    { "f-exit", "Exit without acting", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-confirm", "Factory reset", nullptr, ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 30000, nullptr, nullptr, nullptr, "core.action.factory-reset", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfirmResetPortalLogin_ConfirmResetPortalLoginTitle_Text = { "RESET PORTAL LOGIN?", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfirmResetPortalLogin_ConfirmResetPortalLoginWarning1_Text = { "Restores admin/admin. Totals,", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfirmResetPortalLogin_ConfirmResetPortalLoginWarning2_Text = { "config and calibration kept.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfirmResetPortalLogin_ConfirmResetPortalLoginTimerValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfirmResetPortalLogin_ConfirmResetPortalLoginFooterHint_Text = { "ENTER exit  hold ENTER confirm", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfirmResetPortalLoginElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 240, 135, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfirmResetPortalLogin_ConfirmResetPortalLoginTitle_Text, nullptr, nullptr },
    { "warning-1", ui_exporter::ElementType::Text, 8, 48, 0, 0, &kConfirmResetPortalLogin_ConfirmResetPortalLoginWarning1_Text, nullptr, nullptr },
    { "warning-2", ui_exporter::ElementType::Text, 8, 60, 0, 0, &kConfirmResetPortalLogin_ConfirmResetPortalLoginWarning2_Text, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 104, 84, 0, 0, &kConfirmResetPortalLogin_ConfirmResetPortalLoginTimerValue_Text, nullptr, "countdown.value" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfirmResetPortalLogin_ConfirmResetPortalLoginFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfirmResetPortalLoginFlows[] = {
    { "f-exit", "Exit without acting", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-confirm", "Reset portal login", "toast-portal-login-reset", ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 3000, nullptr, nullptr, nullptr, "core.action.reset-portal-login", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kToastTotalsReset_ToastTotalsResetMessage_Text = { "TOTALS RESET", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kToastTotalsReset_ToastTotalsResetSub_Text = { "Returning...", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kToastTotalsResetElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 240, 135, nullptr, nullptr, nullptr },
    { "message", ui_exporter::ElementType::Text, 8, 58, 0, 0, &kToastTotalsReset_ToastTotalsResetMessage_Text, nullptr, nullptr },
    { "sub", ui_exporter::ElementType::Text, 8, 74, 0, 0, &kToastTotalsReset_ToastTotalsResetSub_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kToastTotalsResetFlows[] = {
    { "f-dismiss", "Dismiss", nullptr, ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::None, ui_exporter::FlowGesture::Short, 2000, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kToastSessionReset_ToastSessionResetMessage_Text = { "SESSION RESET", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kToastSessionReset_ToastSessionResetSub_Text = { "Returning...", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kToastSessionResetElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 240, 135, nullptr, nullptr, nullptr },
    { "message", ui_exporter::ElementType::Text, 8, 58, 0, 0, &kToastSessionReset_ToastSessionResetMessage_Text, nullptr, nullptr },
    { "sub", ui_exporter::ElementType::Text, 8, 74, 0, 0, &kToastSessionReset_ToastSessionResetSub_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kToastSessionResetFlows[] = {
    { "f-dismiss", "Dismiss", nullptr, ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::None, ui_exporter::FlowGesture::Short, 2000, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kToastPortalLoginReset_ToastPortalLoginResetMessage_Text = { "LOGIN: admin/admin", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kToastPortalLoginReset_ToastPortalLoginResetSub_Text = { "Returning...", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kToastPortalLoginResetElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 240, 135, nullptr, nullptr, nullptr },
    { "message", ui_exporter::ElementType::Text, 8, 58, 0, 0, &kToastPortalLoginReset_ToastPortalLoginResetMessage_Text, nullptr, nullptr },
    { "sub", ui_exporter::ElementType::Text, 8, 74, 0, 0, &kToastPortalLoginReset_ToastPortalLoginResetSub_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kToastPortalLoginResetFlows[] = {
    { "f-dismiss", "Dismiss", nullptr, ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::None, ui_exporter::FlowGesture::Short, 2000, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 }
};

const ui_exporter::Screen kGeneratedScreens[] = {
    { "info-p0-global-status", "P0 — Global Status", kInfoP0GlobalStatusElements, sizeof(kInfoP0GlobalStatusElements) / sizeof(kInfoP0GlobalStatusElements[0]), kInfoP0GlobalStatusFlows, sizeof(kInfoP0GlobalStatusFlows) / sizeof(kInfoP0GlobalStatusFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p1-instant-flow", "P1 — Instant Flow", kInfoP1InstantFlowElements, sizeof(kInfoP1InstantFlowElements) / sizeof(kInfoP1InstantFlowElements[0]), kInfoP1InstantFlowFlows, sizeof(kInfoP1InstantFlowFlows) / sizeof(kInfoP1InstantFlowFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p2-cumulative-m3", "P2 — Cumulative Volume", kInfoP2CumulativeM3Elements, sizeof(kInfoP2CumulativeM3Elements) / sizeof(kInfoP2CumulativeM3Elements[0]), kInfoP2CumulativeM3Flows, sizeof(kInfoP2CumulativeM3Flows) / sizeof(kInfoP2CumulativeM3Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p3-session-m3", "P3 — Session Volume", kInfoP3SessionM3Elements, sizeof(kInfoP3SessionM3Elements) / sizeof(kInfoP3SessionM3Elements[0]), kInfoP3SessionM3Flows, sizeof(kInfoP3SessionM3Flows) / sizeof(kInfoP3SessionM3Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p4-max-flow", "P4 — Max Flow Since Reset", kInfoP4MaxFlowElements, sizeof(kInfoP4MaxFlowElements) / sizeof(kInfoP4MaxFlowElements[0]), kInfoP4MaxFlowFlows, sizeof(kInfoP4MaxFlowFlows) / sizeof(kInfoP4MaxFlowFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p5-enter-config", "P5 — Enter Configuration", kInfoP5EnterConfigElements, sizeof(kInfoP5EnterConfigElements) / sizeof(kInfoP5EnterConfigElements[0]), kInfoP5EnterConfigFlows, sizeof(kInfoP5EnterConfigFlows) / sizeof(kInfoP5EnterConfigFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "nyquist-warning", "Nyquist Validation Warning", kNyquistWarningElements, sizeof(kNyquistWarningElements) / sizeof(kNyquistWarningElements[0]), kNyquistWarningFlows, sizeof(kNyquistWarningFlows) / sizeof(kNyquistWarningFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "state-idle", "Idle (display off)", kStateIdleElements, sizeof(kStateIdleElements) / sizeof(kStateIdleElements[0]), kStateIdleFlows, sizeof(kStateIdleFlows) / sizeof(kStateIdleFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c1-modbus-id", "C1 — Modbus ID", kConfigC1ModbusIdElements, sizeof(kConfigC1ModbusIdElements) / sizeof(kConfigC1ModbusIdElements[0]), kConfigC1ModbusIdFlows, sizeof(kConfigC1ModbusIdFlows) / sizeof(kConfigC1ModbusIdFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c2-baud-rate", "C2 — Baud Rate", kConfigC2BaudRateElements, sizeof(kConfigC2BaudRateElements) / sizeof(kConfigC2BaudRateElements[0]), kConfigC2BaudRateFlows, sizeof(kConfigC2BaudRateFlows) / sizeof(kConfigC2BaudRateFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c3-parity", "C3 — Parity", kConfigC3ParityElements, sizeof(kConfigC3ParityElements) / sizeof(kConfigC3ParityElements[0]), kConfigC3ParityFlows, sizeof(kConfigC3ParityFlows) / sizeof(kConfigC3ParityFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c4-stop-bits", "C4 — Stop Bits", kConfigC4StopBitsElements, sizeof(kConfigC4StopBitsElements) / sizeof(kConfigC4StopBitsElements[0]), kConfigC4StopBitsFlows, sizeof(kConfigC4StopBitsFlows) / sizeof(kConfigC4StopBitsFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c5-led-pulse-vol", "C5 — LED Pulse Volume", kConfigC5LedPulseVolElements, sizeof(kConfigC5LedPulseVolElements) / sizeof(kConfigC5LedPulseVolElements[0]), kConfigC5LedPulseVolFlows, sizeof(kConfigC5LedPulseVolFlows) / sizeof(kConfigC5LedPulseVolFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c6-led-pulse-period", "C6 — LED Pulse Period", kConfigC6LedPulsePeriodElements, sizeof(kConfigC6LedPulsePeriodElements) / sizeof(kConfigC6LedPulsePeriodElements[0]), kConfigC6LedPulsePeriodFlows, sizeof(kConfigC6LedPulsePeriodFlows) / sizeof(kConfigC6LedPulsePeriodFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c7-sensor-select", "C7 — Sensors", kConfigC7SensorSelectElements, sizeof(kConfigC7SensorSelectElements) / sizeof(kConfigC7SensorSelectElements[0]), kConfigC7SensorSelectFlows, sizeof(kConfigC7SensorSelectFlows) / sizeof(kConfigC7SensorSelectFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-root-back", "C.BACK — Back", kConfigRootBackElements, sizeof(kConfigRootBackElements) / sizeof(kConfigRootBackElements[0]), kConfigRootBackFlows, sizeof(kConfigRootBackFlows) / sizeof(kConfigRootBackFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c1-modbus-id-edit", "C1.V — Edit Modbus ID", kConfigC1ModbusIdEditElements, sizeof(kConfigC1ModbusIdEditElements) / sizeof(kConfigC1ModbusIdEditElements[0]), kConfigC1ModbusIdEditFlows, sizeof(kConfigC1ModbusIdEditFlows) / sizeof(kConfigC1ModbusIdEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c2-baud-rate-edit", "C2.V — Edit Baud Rate", kConfigC2BaudRateEditElements, sizeof(kConfigC2BaudRateEditElements) / sizeof(kConfigC2BaudRateEditElements[0]), kConfigC2BaudRateEditFlows, sizeof(kConfigC2BaudRateEditFlows) / sizeof(kConfigC2BaudRateEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c3-parity-edit", "C3.V — Edit Parity", kConfigC3ParityEditElements, sizeof(kConfigC3ParityEditElements) / sizeof(kConfigC3ParityEditElements[0]), kConfigC3ParityEditFlows, sizeof(kConfigC3ParityEditFlows) / sizeof(kConfigC3ParityEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c4-stop-bits-edit", "C4.V — Edit Stop Bits", kConfigC4StopBitsEditElements, sizeof(kConfigC4StopBitsEditElements) / sizeof(kConfigC4StopBitsEditElements[0]), kConfigC4StopBitsEditFlows, sizeof(kConfigC4StopBitsEditFlows) / sizeof(kConfigC4StopBitsEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c5-led-pulse-vol-edit", "C5.V — Edit LED Pulse Volume", kConfigC5LedPulseVolEditElements, sizeof(kConfigC5LedPulseVolEditElements) / sizeof(kConfigC5LedPulseVolEditElements[0]), kConfigC5LedPulseVolEditFlows, sizeof(kConfigC5LedPulseVolEditFlows) / sizeof(kConfigC5LedPulseVolEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c6-led-pulse-period-edit", "C6.V — Edit LED Pulse Period", kConfigC6LedPulsePeriodEditElements, sizeof(kConfigC6LedPulsePeriodEditElements) / sizeof(kConfigC6LedPulsePeriodEditElements[0]), kConfigC6LedPulsePeriodEditFlows, sizeof(kConfigC6LedPulsePeriodEditFlows) / sizeof(kConfigC6LedPulsePeriodEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-1", "SEN1 — Sensor 1", kConfigSensor1Elements, sizeof(kConfigSensor1Elements) / sizeof(kConfigSensor1Elements[0]), kConfigSensor1Flows, sizeof(kConfigSensor1Flows) / sizeof(kConfigSensor1Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-2", "SEN2 — Sensor 2", kConfigSensor2Elements, sizeof(kConfigSensor2Elements) / sizeof(kConfigSensor2Elements[0]), kConfigSensor2Flows, sizeof(kConfigSensor2Flows) / sizeof(kConfigSensor2Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-3", "SEN3 — Sensor 3", kConfigSensor3Elements, sizeof(kConfigSensor3Elements) / sizeof(kConfigSensor3Elements[0]), kConfigSensor3Flows, sizeof(kConfigSensor3Flows) / sizeof(kConfigSensor3Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-4", "SEN4 — Sensor 4", kConfigSensor4Elements, sizeof(kConfigSensor4Elements) / sizeof(kConfigSensor4Elements[0]), kConfigSensor4Flows, sizeof(kConfigSensor4Flows) / sizeof(kConfigSensor4Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-5", "SEN5 — Sensor 5", kConfigSensor5Elements, sizeof(kConfigSensor5Elements) / sizeof(kConfigSensor5Elements[0]), kConfigSensor5Flows, sizeof(kConfigSensor5Flows) / sizeof(kConfigSensor5Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-6", "SEN6 — Sensor 6", kConfigSensor6Elements, sizeof(kConfigSensor6Elements) / sizeof(kConfigSensor6Elements[0]), kConfigSensor6Flows, sizeof(kConfigSensor6Flows) / sizeof(kConfigSensor6Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-7", "SEN7 — Sensor 7", kConfigSensor7Elements, sizeof(kConfigSensor7Elements) / sizeof(kConfigSensor7Elements[0]), kConfigSensor7Flows, sizeof(kConfigSensor7Flows) / sizeof(kConfigSensor7Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-8", "SEN8 — Sensor 8", kConfigSensor8Elements, sizeof(kConfigSensor8Elements) / sizeof(kConfigSensor8Elements[0]), kConfigSensor8Flows, sizeof(kConfigSensor8Flows) / sizeof(kConfigSensor8Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-back", "SEN.BACK — Back", kConfigSensorBackElements, sizeof(kConfigSensorBackElements) / sizeof(kConfigSensorBackElements[0]), kConfigSensorBackFlows, sizeof(kConfigSensorBackFlows) / sizeof(kConfigSensorBackFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s1-connected", "S1 — Connected", kConfigS1ConnectedElements, sizeof(kConfigS1ConnectedElements) / sizeof(kConfigS1ConnectedElements[0]), kConfigS1ConnectedFlows, sizeof(kConfigS1ConnectedFlows) / sizeof(kConfigS1ConnectedFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s1-connected-edit", "S1.V — Edit Connected", kConfigS1ConnectedEditElements, sizeof(kConfigS1ConnectedEditElements) / sizeof(kConfigS1ConnectedEditElements[0]), kConfigS1ConnectedEditFlows, sizeof(kConfigS1ConnectedEditFlows) / sizeof(kConfigS1ConnectedEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s2-calibration", "S2 — Calibration", kConfigS2CalibrationElements, sizeof(kConfigS2CalibrationElements) / sizeof(kConfigS2CalibrationElements[0]), kConfigS2CalibrationFlows, sizeof(kConfigS2CalibrationFlows) / sizeof(kConfigS2CalibrationFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s2-calibration-edit", "S2.V — Edit Calibration", kConfigS2CalibrationEditElements, sizeof(kConfigS2CalibrationEditElements) / sizeof(kConfigS2CalibrationEditElements[0]), kConfigS2CalibrationEditFlows, sizeof(kConfigS2CalibrationEditFlows) / sizeof(kConfigS2CalibrationEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s3-pulses-per-l", "S3 — Pulses per litre", kConfigS3PulsesPerLElements, sizeof(kConfigS3PulsesPerLElements) / sizeof(kConfigS3PulsesPerLElements[0]), kConfigS3PulsesPerLFlows, sizeof(kConfigS3PulsesPerLFlows) / sizeof(kConfigS3PulsesPerLFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s3-pulses-per-l-edit", "S3.V — Edit Pulses per litre", kConfigS3PulsesPerLEditElements, sizeof(kConfigS3PulsesPerLEditElements) / sizeof(kConfigS3PulsesPerLEditElements[0]), kConfigS3PulsesPerLEditFlows, sizeof(kConfigS3PulsesPerLEditFlows) / sizeof(kConfigS3PulsesPerLEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s4-multiplier", "S4 — Multiplier (F)", kConfigS4MultiplierElements, sizeof(kConfigS4MultiplierElements) / sizeof(kConfigS4MultiplierElements[0]), kConfigS4MultiplierFlows, sizeof(kConfigS4MultiplierFlows) / sizeof(kConfigS4MultiplierFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s4-multiplier-edit", "S4.V — Edit Multiplier (F)", kConfigS4MultiplierEditElements, sizeof(kConfigS4MultiplierEditElements) / sizeof(kConfigS4MultiplierEditElements[0]), kConfigS4MultiplierEditFlows, sizeof(kConfigS4MultiplierEditFlows) / sizeof(kConfigS4MultiplierEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s5-adjust", "S5 — Adjust", kConfigS5AdjustElements, sizeof(kConfigS5AdjustElements) / sizeof(kConfigS5AdjustElements[0]), kConfigS5AdjustFlows, sizeof(kConfigS5AdjustFlows) / sizeof(kConfigS5AdjustFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s5-adjust-edit", "S5.V — Edit Adjust", kConfigS5AdjustEditElements, sizeof(kConfigS5AdjustEditElements) / sizeof(kConfigS5AdjustEditElements[0]), kConfigS5AdjustEditFlows, sizeof(kConfigS5AdjustEditFlows) / sizeof(kConfigS5AdjustEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s6-max-flow", "S6 — Max Flow (Q)", kConfigS6MaxFlowElements, sizeof(kConfigS6MaxFlowElements) / sizeof(kConfigS6MaxFlowElements[0]), kConfigS6MaxFlowFlows, sizeof(kConfigS6MaxFlowFlows) / sizeof(kConfigS6MaxFlowFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s6-max-flow-edit", "S6.V — Edit Max Flow (Q)", kConfigS6MaxFlowEditElements, sizeof(kConfigS6MaxFlowEditElements) / sizeof(kConfigS6MaxFlowEditElements[0]), kConfigS6MaxFlowEditFlows, sizeof(kConfigS6MaxFlowEditFlows) / sizeof(kConfigS6MaxFlowEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-settings-back", "S.BACK — Back", kConfigSensorSettingsBackElements, sizeof(kConfigSensorSettingsBackElements) / sizeof(kConfigSensorSettingsBackElements[0]), kConfigSensorSettingsBackFlows, sizeof(kConfigSensorSettingsBackFlows) / sizeof(kConfigSensorSettingsBackFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-wifi-root", "WIFI — WiFi", kNetWifiRootElements, sizeof(kNetWifiRootElements) / sizeof(kNetWifiRootElements[0]), kNetWifiRootFlows, sizeof(kNetWifiRootFlows) / sizeof(kNetWifiRootFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-mqtt-root", "MQTT — MQTT", kNetMqttRootElements, sizeof(kNetMqttRootElements) / sizeof(kNetMqttRootElements[0]), kNetMqttRootFlows, sizeof(kNetMqttRootFlows) / sizeof(kNetMqttRootFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-wifi-info", "W.I1 — WiFi", kNetWifiInfoElements, sizeof(kNetWifiInfoElements) / sizeof(kNetWifiInfoElements[0]), kNetWifiInfoFlows, sizeof(kNetWifiInfoFlows) / sizeof(kNetWifiInfoFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-wifi-info-2", "W.I2 — WiFi link", kNetWifiInfo2Elements, sizeof(kNetWifiInfo2Elements) / sizeof(kNetWifiInfo2Elements[0]), kNetWifiInfo2Flows, sizeof(kNetWifiInfo2Flows) / sizeof(kNetWifiInfo2Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-wifi-portal-reset", "W4 — Reset portal login", kNetWifiPortalResetElements, sizeof(kNetWifiPortalResetElements) / sizeof(kNetWifiPortalResetElements[0]), kNetWifiPortalResetFlows, sizeof(kNetWifiPortalResetFlows) / sizeof(kNetWifiPortalResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-wifi-back", "W.BACK — Back", kNetWifiBackElements, sizeof(kNetWifiBackElements) / sizeof(kNetWifiBackElements[0]), kNetWifiBackFlows, sizeof(kNetWifiBackFlows) / sizeof(kNetWifiBackFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-wifi-ap-info", "W6 — AP info", kNetWifiApInfoElements, sizeof(kNetWifiApInfoElements) / sizeof(kNetWifiApInfoElements[0]), kNetWifiApInfoFlows, sizeof(kNetWifiApInfoFlows) / sizeof(kNetWifiApInfoFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-mqtt-info", "M.I1 — MQTT", kNetMqttInfoElements, sizeof(kNetMqttInfoElements) / sizeof(kNetMqttInfoElements[0]), kNetMqttInfoFlows, sizeof(kNetMqttInfoFlows) / sizeof(kNetMqttInfoFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-mqtt-info-2", "M.I2 — MQTT broker", kNetMqttInfo2Elements, sizeof(kNetMqttInfo2Elements) / sizeof(kNetMqttInfo2Elements[0]), kNetMqttInfo2Flows, sizeof(kNetMqttInfo2Flows) / sizeof(kNetMqttInfo2Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-mqtt-info-3", "M.I3 — MQTT publish", kNetMqttInfo3Elements, sizeof(kNetMqttInfo3Elements) / sizeof(kNetMqttInfo3Elements[0]), kNetMqttInfo3Flows, sizeof(kNetMqttInfo3Flows) / sizeof(kNetMqttInfo3Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "net-mqtt-back", "M.BACK — Back", kNetMqttBackElements, sizeof(kNetMqttBackElements) / sizeof(kNetMqttBackElements[0]), kNetMqttBackFlows, sizeof(kNetMqttBackFlows) / sizeof(kNetMqttBackFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p6-factory-reset", "P6 — Factory Reset", kInfoP6FactoryResetElements, sizeof(kInfoP6FactoryResetElements) / sizeof(kInfoP6FactoryResetElements[0]), kInfoP6FactoryResetFlows, sizeof(kInfoP6FactoryResetFlows) / sizeof(kInfoP6FactoryResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "confirm-reset-totals", "Reset totals?", kConfirmResetTotalsElements, sizeof(kConfirmResetTotalsElements) / sizeof(kConfirmResetTotalsElements[0]), kConfirmResetTotalsFlows, sizeof(kConfirmResetTotalsFlows) / sizeof(kConfirmResetTotalsFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "confirm-reset-session", "Reset session?", kConfirmResetSessionElements, sizeof(kConfirmResetSessionElements) / sizeof(kConfirmResetSessionElements[0]), kConfirmResetSessionFlows, sizeof(kConfirmResetSessionFlows) / sizeof(kConfirmResetSessionFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "confirm-factory-reset", "Factory reset?", kConfirmFactoryResetElements, sizeof(kConfirmFactoryResetElements) / sizeof(kConfirmFactoryResetElements[0]), kConfirmFactoryResetFlows, sizeof(kConfirmFactoryResetFlows) / sizeof(kConfirmFactoryResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "confirm-reset-portal-login", "Reset portal login?", kConfirmResetPortalLoginElements, sizeof(kConfirmResetPortalLoginElements) / sizeof(kConfirmResetPortalLoginElements[0]), kConfirmResetPortalLoginFlows, sizeof(kConfirmResetPortalLoginFlows) / sizeof(kConfirmResetPortalLoginFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "toast-totals-reset", "Totals reset", kToastTotalsResetElements, sizeof(kToastTotalsResetElements) / sizeof(kToastTotalsResetElements[0]), kToastTotalsResetFlows, sizeof(kToastTotalsResetFlows) / sizeof(kToastTotalsResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "toast-session-reset", "Session reset", kToastSessionResetElements, sizeof(kToastSessionResetElements) / sizeof(kToastSessionResetElements[0]), kToastSessionResetFlows, sizeof(kToastSessionResetFlows) / sizeof(kToastSessionResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "toast-portal-login-reset", "Portal login reset", kToastPortalLoginResetElements, sizeof(kToastPortalLoginResetElements) / sizeof(kToastPortalLoginResetElements[0]), kToastPortalLoginResetFlows, sizeof(kToastPortalLoginResetFlows) / sizeof(kToastPortalLoginResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 }
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
    "2026-08-10T12:27:17.978Z", 63, 525
};

}  // namespace ui_exporter
