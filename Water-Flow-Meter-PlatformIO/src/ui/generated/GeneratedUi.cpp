#include "GeneratedUi.h"

namespace ui_exporter {

static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusHdrTitle_Text = { "System Status", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusTotalFlowLabel_Text = { "Total Flow (L/s):", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusTotalFlowValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusFooterHint_Text = { "UP/DN pages   UP+DN display off", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP0GlobalStatus_InfoP0GlobalStatusLegendLed_Text = { "LED: Red=Pulse Grn=Ready Blu=Flow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP0GlobalStatusElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusHdrTitle_Text, nullptr, nullptr },
    { "total-flow-label", ui_exporter::ElementType::Text, 2, 20, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusTotalFlowLabel_Text, nullptr, nullptr },
    { "total-flow-value", ui_exporter::ElementType::Text, 2, 32, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusTotalFlowValue_Text, nullptr, "telemetry.total" },
    { "flow-dots", ui_exporter::ElementType::Icon, 40, 70, 55, 55, nullptr, "flow-dots", nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr },
    { "legend-led", ui_exporter::ElementType::Text, 8, 112, 0, 0, &kInfoP0GlobalStatus_InfoP0GlobalStatusLegendLed_Text, nullptr, "legend.led" }
};


static constexpr ui_exporter::Flow kInfoP0GlobalStatusFlows[] = {
    { "f-next", "Next page", "info-p1-instant-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p8-factory-reset", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowHdrTitle_Text = { "Instant Flow (L/s)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS1Label_Text = { "S1", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS1Status_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS2Label_Text = { "S2", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS2Status_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS3Label_Text = { "S3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS3Status_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS4Label_Text = { "S4", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS4Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS4Status_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS5Label_Text = { "S5", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS5Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS5Status_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS6Label_Text = { "S6", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS6Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS6Status_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS7Label_Text = { "S7", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS7Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS7Status_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS8Label_Text = { "S8", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS8Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowS8Status_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowFooterHint_Text = { "UP/DN pages   UP+DN display off", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP1InstantFlowElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowHdrTitle_Text, nullptr, nullptr },
    { "s1-label", ui_exporter::ElementType::Text, 2, 18, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS1Label_Text, nullptr, nullptr },
    { "s1-value", ui_exporter::ElementType::Value, 14, 18, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS1Value_Text, nullptr, "sensor.1.instantFlow" },
    { "s1-status", ui_exporter::ElementType::Badge, 60, 18, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS1Status_Text, nullptr, "sensor.1.status" },
    { "s2-label", ui_exporter::ElementType::Text, 2, 30, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS2Label_Text, nullptr, nullptr },
    { "s2-value", ui_exporter::ElementType::Value, 14, 30, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS2Value_Text, nullptr, "sensor.2.instantFlow" },
    { "s2-status", ui_exporter::ElementType::Badge, 60, 30, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS2Status_Text, nullptr, "sensor.2.status" },
    { "s3-label", ui_exporter::ElementType::Text, 2, 42, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS3Label_Text, nullptr, nullptr },
    { "s3-value", ui_exporter::ElementType::Value, 14, 42, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS3Value_Text, nullptr, "sensor.3.instantFlow" },
    { "s3-status", ui_exporter::ElementType::Badge, 60, 42, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS3Status_Text, nullptr, "sensor.3.status" },
    { "s4-label", ui_exporter::ElementType::Text, 2, 54, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS4Label_Text, nullptr, nullptr },
    { "s4-value", ui_exporter::ElementType::Value, 14, 54, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS4Value_Text, nullptr, "sensor.4.instantFlow" },
    { "s4-status", ui_exporter::ElementType::Badge, 60, 54, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS4Status_Text, nullptr, "sensor.4.status" },
    { "s5-label", ui_exporter::ElementType::Text, 69, 18, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS5Label_Text, nullptr, nullptr },
    { "s5-value", ui_exporter::ElementType::Value, 81, 18, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS5Value_Text, nullptr, "sensor.5.instantFlow" },
    { "s5-status", ui_exporter::ElementType::Badge, 120, 18, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS5Status_Text, nullptr, "sensor.5.status" },
    { "s6-label", ui_exporter::ElementType::Text, 69, 30, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS6Label_Text, nullptr, nullptr },
    { "s6-value", ui_exporter::ElementType::Value, 81, 30, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS6Value_Text, nullptr, "sensor.6.instantFlow" },
    { "s6-status", ui_exporter::ElementType::Badge, 120, 30, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS6Status_Text, nullptr, "sensor.6.status" },
    { "s7-label", ui_exporter::ElementType::Text, 69, 42, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS7Label_Text, nullptr, nullptr },
    { "s7-value", ui_exporter::ElementType::Value, 81, 42, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS7Value_Text, nullptr, "sensor.7.instantFlow" },
    { "s7-status", ui_exporter::ElementType::Badge, 120, 42, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS7Status_Text, nullptr, "sensor.7.status" },
    { "s8-label", ui_exporter::ElementType::Text, 69, 54, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS8Label_Text, nullptr, nullptr },
    { "s8-value", ui_exporter::ElementType::Value, 81, 54, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS8Value_Text, nullptr, "sensor.8.instantFlow" },
    { "s8-status", ui_exporter::ElementType::Badge, 120, 54, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowS8Status_Text, nullptr, "sensor.8.status" },
    { "divider-1", ui_exporter::ElementType::Box, 0, 65, 240, 1, nullptr, nullptr, nullptr },
    { "divider-2", ui_exporter::ElementType::Box, 0, 124, 240, 1, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP1InstantFlowFlows[] = {
    { "f-next", "Next page", "info-p2-cumulative-liters", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersHdrTitle_Text = { "Cumulative (L)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS1Label_Text = { "S1", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS2Label_Text = { "S2", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS3Label_Text = { "S3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS4Label_Text = { "S4", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS4Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS5Label_Text = { "S5", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS5Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS6Label_Text = { "S6", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS6Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS7Label_Text = { "S7", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS7Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS8Label_Text = { "S8", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersS8Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersFooterHint_Text = { "ENTER reset totals (hold 3s)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersUndersamplingBadge_Text = { "! Undersampling", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP2CumulativeLitersElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersHdrTitle_Text, nullptr, nullptr },
    { "s1-label", ui_exporter::ElementType::Text, 2, 18, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS1Label_Text, nullptr, nullptr },
    { "s1-value", ui_exporter::ElementType::Value, 14, 18, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS1Value_Text, nullptr, "sensor.1.cumulativeLiters" },
    { "s2-label", ui_exporter::ElementType::Text, 2, 30, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS2Label_Text, nullptr, nullptr },
    { "s2-value", ui_exporter::ElementType::Value, 14, 30, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS2Value_Text, nullptr, "sensor.2.cumulativeLiters" },
    { "s3-label", ui_exporter::ElementType::Text, 2, 42, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS3Label_Text, nullptr, nullptr },
    { "s3-value", ui_exporter::ElementType::Value, 14, 42, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS3Value_Text, nullptr, "sensor.3.cumulativeLiters" },
    { "s4-label", ui_exporter::ElementType::Text, 2, 54, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS4Label_Text, nullptr, nullptr },
    { "s4-value", ui_exporter::ElementType::Value, 14, 54, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS4Value_Text, nullptr, "sensor.4.cumulativeLiters" },
    { "s5-label", ui_exporter::ElementType::Text, 69, 18, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS5Label_Text, nullptr, nullptr },
    { "s5-value", ui_exporter::ElementType::Value, 81, 18, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS5Value_Text, nullptr, "sensor.5.cumulativeLiters" },
    { "s6-label", ui_exporter::ElementType::Text, 69, 30, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS6Label_Text, nullptr, nullptr },
    { "s6-value", ui_exporter::ElementType::Value, 81, 30, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS6Value_Text, nullptr, "sensor.6.cumulativeLiters" },
    { "s7-label", ui_exporter::ElementType::Text, 69, 42, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS7Label_Text, nullptr, nullptr },
    { "s7-value", ui_exporter::ElementType::Value, 81, 42, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS7Value_Text, nullptr, "sensor.7.cumulativeLiters" },
    { "s8-label", ui_exporter::ElementType::Text, 69, 54, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS8Label_Text, nullptr, nullptr },
    { "s8-value", ui_exporter::ElementType::Value, 81, 54, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersS8Value_Text, nullptr, "sensor.8.cumulativeLiters" },
    { "divider-1", ui_exporter::ElementType::Box, 0, 65, 240, 1, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersFooterHint_Text, nullptr, nullptr },
    { "undersampling-badge", ui_exporter::ElementType::Badge, 176, 4, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersUndersamplingBadge_Text, nullptr, "diagnostics.undersampling" },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP2CumulativeLitersFlows[] = {
    { "f-next", "Next page", "info-p3-cumulative-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p1-instant-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open", "confirm-reset-totals", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3HdrTitle_Text = { "Cumulative (m3)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S1Label_Text = { "S1", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S2Label_Text = { "S2", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S3Label_Text = { "S3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S4Label_Text = { "S4", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S4Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S5Label_Text = { "S5", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S5Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S6Label_Text = { "S6", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S6Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S7Label_Text = { "S7", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S7Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S8Label_Text = { "S8", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3S8Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3FooterHint_Text = { "ENTER reset totals (hold 3s)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3UndersamplingBadge_Text = { "! Undersampling", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP3CumulativeM3Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3HdrTitle_Text, nullptr, nullptr },
    { "s1-label", ui_exporter::ElementType::Text, 2, 18, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S1Label_Text, nullptr, nullptr },
    { "s1-value", ui_exporter::ElementType::Value, 14, 18, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S1Value_Text, nullptr, "sensor.1.cumulativeM3" },
    { "s2-label", ui_exporter::ElementType::Text, 2, 30, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S2Label_Text, nullptr, nullptr },
    { "s2-value", ui_exporter::ElementType::Value, 14, 30, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S2Value_Text, nullptr, "sensor.2.cumulativeM3" },
    { "s3-label", ui_exporter::ElementType::Text, 2, 42, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S3Label_Text, nullptr, nullptr },
    { "s3-value", ui_exporter::ElementType::Value, 14, 42, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S3Value_Text, nullptr, "sensor.3.cumulativeM3" },
    { "s4-label", ui_exporter::ElementType::Text, 2, 54, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S4Label_Text, nullptr, nullptr },
    { "s4-value", ui_exporter::ElementType::Value, 14, 54, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S4Value_Text, nullptr, "sensor.4.cumulativeM3" },
    { "s5-label", ui_exporter::ElementType::Text, 69, 18, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S5Label_Text, nullptr, nullptr },
    { "s5-value", ui_exporter::ElementType::Value, 81, 18, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S5Value_Text, nullptr, "sensor.5.cumulativeM3" },
    { "s6-label", ui_exporter::ElementType::Text, 69, 30, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S6Label_Text, nullptr, nullptr },
    { "s6-value", ui_exporter::ElementType::Value, 81, 30, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S6Value_Text, nullptr, "sensor.6.cumulativeM3" },
    { "s7-label", ui_exporter::ElementType::Text, 69, 42, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S7Label_Text, nullptr, nullptr },
    { "s7-value", ui_exporter::ElementType::Value, 81, 42, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S7Value_Text, nullptr, "sensor.7.cumulativeM3" },
    { "s8-label", ui_exporter::ElementType::Text, 69, 54, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S8Label_Text, nullptr, nullptr },
    { "s8-value", ui_exporter::ElementType::Value, 81, 54, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3S8Value_Text, nullptr, "sensor.8.cumulativeM3" },
    { "divider-1", ui_exporter::ElementType::Box, 0, 65, 240, 1, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3FooterHint_Text, nullptr, nullptr },
    { "undersampling-badge", ui_exporter::ElementType::Badge, 176, 4, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3UndersamplingBadge_Text, nullptr, "diagnostics.undersampling" },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP3CumulativeM3Flows[] = {
    { "f-next", "Next page", "info-p4-session-liters", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p2-cumulative-liters", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open", "confirm-reset-totals", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersHdrTitle_Text = { "Session (L)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS1Label_Text = { "S1", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS2Label_Text = { "S2", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS3Label_Text = { "S3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS4Label_Text = { "S4", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS4Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS5Label_Text = { "S5", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS5Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS6Label_Text = { "S6", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS6Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS7Label_Text = { "S7", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS7Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS8Label_Text = { "S8", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersS8Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersUndersamplingDiag_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersFooterHint_Text = { "ENTER reset session", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP4SessionLitersElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersHdrTitle_Text, nullptr, nullptr },
    { "s1-label", ui_exporter::ElementType::Text, 2, 18, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS1Label_Text, nullptr, nullptr },
    { "s1-value", ui_exporter::ElementType::Value, 14, 18, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS1Value_Text, nullptr, "sensor.1.sessionLiters" },
    { "s2-label", ui_exporter::ElementType::Text, 2, 30, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS2Label_Text, nullptr, nullptr },
    { "s2-value", ui_exporter::ElementType::Value, 14, 30, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS2Value_Text, nullptr, "sensor.2.sessionLiters" },
    { "s3-label", ui_exporter::ElementType::Text, 2, 42, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS3Label_Text, nullptr, nullptr },
    { "s3-value", ui_exporter::ElementType::Value, 14, 42, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS3Value_Text, nullptr, "sensor.3.sessionLiters" },
    { "s4-label", ui_exporter::ElementType::Text, 2, 54, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS4Label_Text, nullptr, nullptr },
    { "s4-value", ui_exporter::ElementType::Value, 14, 54, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS4Value_Text, nullptr, "sensor.4.sessionLiters" },
    { "s5-label", ui_exporter::ElementType::Text, 69, 18, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS5Label_Text, nullptr, nullptr },
    { "s5-value", ui_exporter::ElementType::Value, 81, 18, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS5Value_Text, nullptr, "sensor.5.sessionLiters" },
    { "s6-label", ui_exporter::ElementType::Text, 69, 30, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS6Label_Text, nullptr, nullptr },
    { "s6-value", ui_exporter::ElementType::Value, 81, 30, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS6Value_Text, nullptr, "sensor.6.sessionLiters" },
    { "s7-label", ui_exporter::ElementType::Text, 69, 42, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS7Label_Text, nullptr, nullptr },
    { "s7-value", ui_exporter::ElementType::Value, 81, 42, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS7Value_Text, nullptr, "sensor.7.sessionLiters" },
    { "s8-label", ui_exporter::ElementType::Text, 69, 54, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS8Label_Text, nullptr, nullptr },
    { "s8-value", ui_exporter::ElementType::Value, 81, 54, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersS8Value_Text, nullptr, "sensor.8.sessionLiters" },
    { "undersampling-diag", ui_exporter::ElementType::Badge, 2, 66, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersUndersamplingDiag_Text, nullptr, "diagnostics.undersampling" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP4SessionLitersFlows[] = {
    { "f-next", "Next page", "info-p5-session-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p3-cumulative-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open", "confirm-reset-session", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3HdrTitle_Text = { "Session (m3)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S1Label_Text = { "S1", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S2Label_Text = { "S2", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S3Label_Text = { "S3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S4Label_Text = { "S4", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S4Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S5Label_Text = { "S5", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S5Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S6Label_Text = { "S6", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S6Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S7Label_Text = { "S7", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S7Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S8Label_Text = { "S8", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3S8Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3FooterHint_Text = { "ENTER reset session", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3UndersamplingBadge_Text = { "! Undersampling", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP5SessionM3Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3HdrTitle_Text, nullptr, nullptr },
    { "s1-label", ui_exporter::ElementType::Text, 2, 18, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S1Label_Text, nullptr, nullptr },
    { "s1-value", ui_exporter::ElementType::Value, 14, 18, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S1Value_Text, nullptr, "sensor.1.sessionM3" },
    { "s2-label", ui_exporter::ElementType::Text, 2, 30, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S2Label_Text, nullptr, nullptr },
    { "s2-value", ui_exporter::ElementType::Value, 14, 30, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S2Value_Text, nullptr, "sensor.2.sessionM3" },
    { "s3-label", ui_exporter::ElementType::Text, 2, 42, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S3Label_Text, nullptr, nullptr },
    { "s3-value", ui_exporter::ElementType::Value, 14, 42, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S3Value_Text, nullptr, "sensor.3.sessionM3" },
    { "s4-label", ui_exporter::ElementType::Text, 2, 54, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S4Label_Text, nullptr, nullptr },
    { "s4-value", ui_exporter::ElementType::Value, 14, 54, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S4Value_Text, nullptr, "sensor.4.sessionM3" },
    { "s5-label", ui_exporter::ElementType::Text, 69, 18, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S5Label_Text, nullptr, nullptr },
    { "s5-value", ui_exporter::ElementType::Value, 81, 18, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S5Value_Text, nullptr, "sensor.5.sessionM3" },
    { "s6-label", ui_exporter::ElementType::Text, 69, 30, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S6Label_Text, nullptr, nullptr },
    { "s6-value", ui_exporter::ElementType::Value, 81, 30, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S6Value_Text, nullptr, "sensor.6.sessionM3" },
    { "s7-label", ui_exporter::ElementType::Text, 69, 42, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S7Label_Text, nullptr, nullptr },
    { "s7-value", ui_exporter::ElementType::Value, 81, 42, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S7Value_Text, nullptr, "sensor.7.sessionM3" },
    { "s8-label", ui_exporter::ElementType::Text, 69, 54, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S8Label_Text, nullptr, nullptr },
    { "s8-value", ui_exporter::ElementType::Value, 81, 54, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3S8Value_Text, nullptr, "sensor.8.sessionM3" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3FooterHint_Text, nullptr, nullptr },
    { "undersampling-badge", ui_exporter::ElementType::Badge, 176, 4, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3UndersamplingBadge_Text, nullptr, "diagnostics.undersampling" },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP5SessionM3Flows[] = {
    { "f-next", "Next page", "info-p6-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p4-session-liters", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open", "confirm-reset-session", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowHdrTitle_Text = { "Max Flow (L/s)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS1Label_Text = { "S1", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS1Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS2Label_Text = { "S2", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS2Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS3Label_Text = { "S3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS3Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS4Label_Text = { "S4", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS4Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS5Label_Text = { "S5", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS5Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS6Label_Text = { "S6", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS6Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS7Label_Text = { "S7", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS7Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS8Label_Text = { "S8", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowS8Value_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowFooterHint_Text = { "ENTER reset session", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowUndersamplingBadge_Text = { "! Undersampling", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP6MaxFlowElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowHdrTitle_Text, nullptr, nullptr },
    { "s1-label", ui_exporter::ElementType::Text, 2, 18, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS1Label_Text, nullptr, nullptr },
    { "s1-value", ui_exporter::ElementType::Value, 14, 18, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS1Value_Text, nullptr, "sensor.1.maxFlowSinceReset" },
    { "s2-label", ui_exporter::ElementType::Text, 2, 30, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS2Label_Text, nullptr, nullptr },
    { "s2-value", ui_exporter::ElementType::Value, 14, 30, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS2Value_Text, nullptr, "sensor.2.maxFlowSinceReset" },
    { "s3-label", ui_exporter::ElementType::Text, 2, 42, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS3Label_Text, nullptr, nullptr },
    { "s3-value", ui_exporter::ElementType::Value, 14, 42, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS3Value_Text, nullptr, "sensor.3.maxFlowSinceReset" },
    { "s4-label", ui_exporter::ElementType::Text, 2, 54, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS4Label_Text, nullptr, nullptr },
    { "s4-value", ui_exporter::ElementType::Value, 14, 54, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS4Value_Text, nullptr, "sensor.4.maxFlowSinceReset" },
    { "s5-label", ui_exporter::ElementType::Text, 69, 18, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS5Label_Text, nullptr, nullptr },
    { "s5-value", ui_exporter::ElementType::Value, 81, 18, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS5Value_Text, nullptr, "sensor.5.maxFlowSinceReset" },
    { "s6-label", ui_exporter::ElementType::Text, 69, 30, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS6Label_Text, nullptr, nullptr },
    { "s6-value", ui_exporter::ElementType::Value, 81, 30, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS6Value_Text, nullptr, "sensor.6.maxFlowSinceReset" },
    { "s7-label", ui_exporter::ElementType::Text, 69, 42, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS7Label_Text, nullptr, nullptr },
    { "s7-value", ui_exporter::ElementType::Value, 81, 42, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS7Value_Text, nullptr, "sensor.7.maxFlowSinceReset" },
    { "s8-label", ui_exporter::ElementType::Text, 69, 54, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS8Label_Text, nullptr, nullptr },
    { "s8-value", ui_exporter::ElementType::Value, 81, 54, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowS8Value_Text, nullptr, "sensor.8.maxFlowSinceReset" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowFooterHint_Text, nullptr, nullptr },
    { "undersampling-badge", ui_exporter::ElementType::Badge, 176, 4, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowUndersamplingBadge_Text, nullptr, "diagnostics.undersampling" },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP6MaxFlowFlows[] = {
    { "f-next", "Next page", "info-p7-enter-config", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p5-session-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open", "confirm-reset-session", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Back to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP7EnterConfig_InfoP7EnterConfigHdrTitle_Text = { "Configuration", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP7EnterConfig_InfoP7EnterConfigPromptLine1_Text = { "Press ENTER to open", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP7EnterConfig_InfoP7EnterConfigPromptLine2_Text = { "Configuration.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP7EnterConfig_InfoP7EnterConfigFooterHint_Text = { "ENTER opens Configuration", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP7EnterConfigElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP7EnterConfig_InfoP7EnterConfigHdrTitle_Text, nullptr, nullptr },
    { "prompt-line1", ui_exporter::ElementType::Text, 10, 100, 0, 0, &kInfoP7EnterConfig_InfoP7EnterConfigPromptLine1_Text, nullptr, nullptr },
    { "prompt-line2", ui_exporter::ElementType::Text, 10, 112, 0, 0, &kInfoP7EnterConfig_InfoP7EnterConfigPromptLine2_Text, nullptr, nullptr },
    { "divider-1", ui_exporter::ElementType::Box, 20, 120, 95, 1, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kInfoP7EnterConfig_InfoP7EnterConfigFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP7EnterConfigFlows[] = {
    { "f-next", "Next page", "info-p8-factory-reset", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p6-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
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

static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdHdrTitle_Text = { "Config > Modbus ID", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdFieldLabel_Text = { "Current", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdFooterHint_Text = { "UP/DN page  ENTER edit  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC1ModbusIdElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdFieldValue_Text, nullptr, "config.modbusSlaveId" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC1ModbusIdFlows[] = {
    { "f-next", "Next entry", "config-c2-baud-rate", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-root-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c1-modbus-id-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateHdrTitle_Text = { "Config > Baud Rate", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateFieldLabel_Text = { "Current", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateFooterHint_Text = { "UP/DN page  ENTER edit  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC2BaudRateElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateFieldValue_Text, nullptr, "config.baudRate" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC2BaudRateFlows[] = {
    { "f-next", "Next entry", "config-c3-parity", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c2-baud-rate-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityHdrTitle_Text = { "Config > Parity", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityFieldLabel_Text = { "Current", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityFooterHint_Text = { "UP/DN page  ENTER edit  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC3ParityElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC3Parity_ConfigC3ParityHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC3Parity_ConfigC3ParityFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC3Parity_ConfigC3ParityFieldValue_Text, nullptr, "config.parity" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC3Parity_ConfigC3ParityFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC3ParityFlows[] = {
    { "f-next", "Next entry", "config-c4-stop-bits", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c2-baud-rate", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c3-parity-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsHdrTitle_Text = { "Config > Stop Bits", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsFieldLabel_Text = { "Current", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsFooterHint_Text = { "UP/DN page  ENTER edit  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC4StopBitsElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsFieldValue_Text, nullptr, "config.stopBits" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC4StopBitsFlows[] = {
    { "f-next", "Next entry", "config-c5-led-pulse-vol", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c3-parity", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c4-stop-bits-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolHdrTitle_Text = { "Config > LED Pulse Volume", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolFieldLabel_Text = { "Current L", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolFooterHint_Text = { "UP/DN page  ENTER edit  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC5LedPulseVolElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolFieldValue_Text, nullptr, "config.ledPulseVolume" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC5LedPulseVolFlows[] = {
    { "f-next", "Next entry", "config-c6-led-pulse-period", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c4-stop-bits", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c5-led-pulse-vol-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodHdrTitle_Text = { "Config > LED Pulse Period", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFieldLabel_Text = { "Current ms", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFooterHint_Text = { "UP/DN page  ENTER edit  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC6LedPulsePeriodElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFieldValue_Text, nullptr, "config.ledPulsePeriod" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC6LedPulsePeriodFlows[] = {
    { "f-next", "Next entry", "config-c7-sensor-select", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c5-led-pulse-vol", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-c6-led-pulse-period-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectHdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectFieldLabel_Text = { "Select a sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectFieldValue_Text = { "Sensors 1-8 >", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectFooterHint_Text = { "UP/DN page  ENTER open  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC7SensorSelectElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectFieldValue_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC7SensorSelectFlows[] = {
    { "f-next", "Next entry", "config-root-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c6-led-pulse-period", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor list", "config-sensor-1", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigRootBack_ConfigRootBackHdrTitle_Text = { "Config", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigRootBack_ConfigRootBackBackLabel_Text = { "< BACK", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigRootBack_ConfigRootBackFooterHint_Text = { "ENTER go back", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigRootBackElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigRootBack_ConfigRootBackHdrTitle_Text, nullptr, nullptr },
    { "back-label", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigRootBack_ConfigRootBackBackLabel_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigRootBack_ConfigRootBackFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigRootBackFlows[] = {
    { "f-next", "Next entry", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-c7-sensor-select", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-back", "Back one level", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditHdrTitle_Text = { "Edit > Modbus ID", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditRangeHint_Text = { "1 to 247", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditPendingLabel_Text = { "New value", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold ENTER discard", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC1ModbusIdEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditHdrTitle_Text, nullptr, nullptr },
    { "range-hint", ui_exporter::ElementType::Text, 8, 16, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditRangeHint_Text, nullptr, nullptr },
    { "pending-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditPendingValue_Text, nullptr, "config.modbusSlaveId" },
    { "saved-label", ui_exporter::ElementType::Text, 8, 78, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditSavedValue_Text, nullptr, "config.modbusSlaveId" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC1ModbusIdEdit_ConfigC1ModbusIdEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC1ModbusIdEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditHdrTitle_Text = { "Edit > Baud Rate", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditRangeHint_Text = { "1200 / 2400 / 4800 / 9600 / 19200 / 38400 / 57600 / 115200", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditPendingLabel_Text = { "New value", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC2BaudRateEdit_ConfigC2BaudRateEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold ENTER discard", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC2BaudRateEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditHdrTitle_Text, nullptr, nullptr },
    { "range-hint", ui_exporter::ElementType::Text, 8, 16, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditRangeHint_Text, nullptr, nullptr },
    { "pending-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditPendingValue_Text, nullptr, "config.baudRate" },
    { "saved-label", ui_exporter::ElementType::Text, 8, 78, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditSavedValue_Text, nullptr, "config.baudRate" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC2BaudRateEdit_ConfigC2BaudRateEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC2BaudRateEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c2-baud-rate", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c2-baud-rate", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditHdrTitle_Text = { "Edit > Parity", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditRangeHint_Text = { "None / Even / Odd", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditPendingLabel_Text = { "New value", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3ParityEdit_ConfigC3ParityEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold ENTER discard", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC3ParityEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditHdrTitle_Text, nullptr, nullptr },
    { "range-hint", ui_exporter::ElementType::Text, 8, 16, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditRangeHint_Text, nullptr, nullptr },
    { "pending-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditPendingValue_Text, nullptr, "config.parity" },
    { "saved-label", ui_exporter::ElementType::Text, 8, 78, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditSavedValue_Text, nullptr, "config.parity" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC3ParityEdit_ConfigC3ParityEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC3ParityEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c3-parity", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c3-parity", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditHdrTitle_Text = { "Edit > Stop Bits", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditRangeHint_Text = { "1 / 2", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditPendingLabel_Text = { "New value", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC4StopBitsEdit_ConfigC4StopBitsEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold ENTER discard", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC4StopBitsEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditHdrTitle_Text, nullptr, nullptr },
    { "range-hint", ui_exporter::ElementType::Text, 8, 16, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditRangeHint_Text, nullptr, nullptr },
    { "pending-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditPendingValue_Text, nullptr, "config.stopBits" },
    { "saved-label", ui_exporter::ElementType::Text, 8, 78, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditSavedValue_Text, nullptr, "config.stopBits" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC4StopBitsEdit_ConfigC4StopBitsEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC4StopBitsEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c4-stop-bits", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c4-stop-bits", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditHdrTitle_Text = { "Edit > LED Pulse Volume", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditRangeHint_Text = { "1 / 10 / 100 L", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditPendingLabel_Text = { "New value", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold ENTER discard", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC5LedPulseVolEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditHdrTitle_Text, nullptr, nullptr },
    { "range-hint", ui_exporter::ElementType::Text, 8, 16, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditRangeHint_Text, nullptr, nullptr },
    { "pending-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditPendingValue_Text, nullptr, "config.ledPulseVolume" },
    { "saved-label", ui_exporter::ElementType::Text, 8, 78, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditSavedValue_Text, nullptr, "config.ledPulseVolume" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC5LedPulseVolEdit_ConfigC5LedPulseVolEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC5LedPulseVolEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c5-led-pulse-vol", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c5-led-pulse-vol", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditHdrTitle_Text = { "Edit > LED Pulse Period", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditRangeHint_Text = { "100 to 2000 ms", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditPendingLabel_Text = { "New value", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold ENTER discard", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC6LedPulsePeriodEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditHdrTitle_Text, nullptr, nullptr },
    { "range-hint", ui_exporter::ElementType::Text, 8, 16, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditRangeHint_Text, nullptr, nullptr },
    { "pending-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditPendingValue_Text, nullptr, "config.ledPulsePeriod" },
    { "saved-label", ui_exporter::ElementType::Text, 8, 78, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditSavedValue_Text, nullptr, "config.ledPulsePeriod" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigC6LedPulsePeriodEdit_ConfigC6LedPulsePeriodEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC6LedPulsePeriodEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-c6-led-pulse-period", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-c6-led-pulse-period", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1FieldLabel_Text = { "Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1FieldValue_Text = { "1 >", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor1_ConfigSensor1FooterHint_Text = { "UP/DN sensor  ENTER open  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor1Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigSensor1_ConfigSensor1HdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigSensor1_ConfigSensor1FieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigSensor1_ConfigSensor1FieldValue_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigSensor1_ConfigSensor1StatusValue_Text, nullptr, "sensor.1.status" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigSensor1_ConfigSensor1FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor1Flows[] = {
    { "f-next", "Next entry", "config-sensor-2", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 1 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2FieldLabel_Text = { "Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2FieldValue_Text = { "2 >", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor2_ConfigSensor2FooterHint_Text = { "UP/DN sensor  ENTER open  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor2Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigSensor2_ConfigSensor2HdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigSensor2_ConfigSensor2FieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigSensor2_ConfigSensor2FieldValue_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigSensor2_ConfigSensor2StatusValue_Text, nullptr, "sensor.2.status" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigSensor2_ConfigSensor2FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor2Flows[] = {
    { "f-next", "Next entry", "config-sensor-3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-1", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 2 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3FieldLabel_Text = { "Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3FieldValue_Text = { "3 >", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor3_ConfigSensor3FooterHint_Text = { "UP/DN sensor  ENTER open  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor3Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigSensor3_ConfigSensor3HdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigSensor3_ConfigSensor3FieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigSensor3_ConfigSensor3FieldValue_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigSensor3_ConfigSensor3StatusValue_Text, nullptr, "sensor.3.status" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigSensor3_ConfigSensor3FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor3Flows[] = {
    { "f-next", "Next entry", "config-sensor-4", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-2", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 3 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4FieldLabel_Text = { "Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4FieldValue_Text = { "4 >", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor4_ConfigSensor4FooterHint_Text = { "UP/DN sensor  ENTER open  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor4Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigSensor4_ConfigSensor4HdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigSensor4_ConfigSensor4FieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigSensor4_ConfigSensor4FieldValue_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigSensor4_ConfigSensor4StatusValue_Text, nullptr, "sensor.4.status" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigSensor4_ConfigSensor4FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor4Flows[] = {
    { "f-next", "Next entry", "config-sensor-5", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 4 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5FieldLabel_Text = { "Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5FieldValue_Text = { "5 >", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor5_ConfigSensor5FooterHint_Text = { "UP/DN sensor  ENTER open  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor5Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigSensor5_ConfigSensor5HdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigSensor5_ConfigSensor5FieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigSensor5_ConfigSensor5FieldValue_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigSensor5_ConfigSensor5StatusValue_Text, nullptr, "sensor.5.status" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigSensor5_ConfigSensor5FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor5Flows[] = {
    { "f-next", "Next entry", "config-sensor-6", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-4", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 5 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6FieldLabel_Text = { "Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6FieldValue_Text = { "6 >", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor6_ConfigSensor6FooterHint_Text = { "UP/DN sensor  ENTER open  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor6Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigSensor6_ConfigSensor6HdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigSensor6_ConfigSensor6FieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigSensor6_ConfigSensor6FieldValue_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigSensor6_ConfigSensor6StatusValue_Text, nullptr, "sensor.6.status" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigSensor6_ConfigSensor6FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor6Flows[] = {
    { "f-next", "Next entry", "config-sensor-7", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-5", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 6 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7FieldLabel_Text = { "Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7FieldValue_Text = { "7 >", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor7_ConfigSensor7FooterHint_Text = { "UP/DN sensor  ENTER open  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor7Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigSensor7_ConfigSensor7HdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigSensor7_ConfigSensor7FieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigSensor7_ConfigSensor7FieldValue_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigSensor7_ConfigSensor7StatusValue_Text, nullptr, "sensor.7.status" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigSensor7_ConfigSensor7FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor7Flows[] = {
    { "f-next", "Next entry", "config-sensor-8", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-6", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 7 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8HdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8FieldLabel_Text = { "Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8FieldValue_Text = { "8 >", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8StatusValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigSensor8_ConfigSensor8FooterHint_Text = { "UP/DN sensor  ENTER open  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensor8Elements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigSensor8_ConfigSensor8HdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigSensor8_ConfigSensor8FieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigSensor8_ConfigSensor8FieldValue_Text, nullptr, nullptr },
    { "status-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigSensor8_ConfigSensor8StatusValue_Text, nullptr, "sensor.8.status" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigSensor8_ConfigSensor8FooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensor8Flows[] = {
    { "f-next", "Next entry", "config-sensor-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-7", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Open sensor 8 settings", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensorBack_ConfigSensorBackHdrTitle_Text = { "Config > Sensors", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensorBack_ConfigSensorBackBackLabel_Text = { "< BACK", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensorBack_ConfigSensorBackFooterHint_Text = { "ENTER go back", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensorBackElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigSensorBack_ConfigSensorBackHdrTitle_Text, nullptr, nullptr },
    { "back-label", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigSensorBack_ConfigSensorBackBackLabel_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigSensorBack_ConfigSensorBackFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensorBackFlows[] = {
    { "f-next", "Next entry", "config-sensor-1", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-8", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-back", "Back one level", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedHdrTitle_Text = { "Sensor > Connected", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedSensorIndex_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedFieldLabel_Text = { "Current", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedFooterHint_Text = { "UP/DN page  ENTER edit  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS1ConnectedElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigS1Connected_ConfigS1ConnectedHdrTitle_Text, nullptr, nullptr },
    { "sensor-index", ui_exporter::ElementType::Value, 200, 2, 0, 0, &kConfigS1Connected_ConfigS1ConnectedSensorIndex_Text, nullptr, "config.selectedSensor" },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigS1Connected_ConfigS1ConnectedFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigS1Connected_ConfigS1ConnectedFieldValue_Text, nullptr, "config.sensor.connected" },
    { "nyquist-warning", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigS1Connected_ConfigS1ConnectedNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigS1Connected_ConfigS1ConnectedFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS1ConnectedFlows[] = {
    { "f-next", "Next entry", "config-s2-multiplier", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-sensor-settings-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-s1-connected-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditHdrTitle_Text = { "Edit > Connected", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditPendingLabel_Text = { "New value", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1ConnectedEdit_ConfigS1ConnectedEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold ENTER discard", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS1ConnectedEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditHdrTitle_Text, nullptr, nullptr },
    { "pending-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditPendingValue_Text, nullptr, "config.sensor.connected" },
    { "saved-label", ui_exporter::ElementType::Text, 8, 78, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditSavedValue_Text, nullptr, "config.sensor.connected" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigS1ConnectedEdit_ConfigS1ConnectedEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS1ConnectedEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierHdrTitle_Text = { "Sensor > Multiplier (F)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierSensorIndex_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierFieldLabel_Text = { "Current", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierFooterHint_Text = { "UP/DN page  ENTER edit  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS2MultiplierElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierHdrTitle_Text, nullptr, nullptr },
    { "sensor-index", ui_exporter::ElementType::Value, 200, 2, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierSensorIndex_Text, nullptr, "config.selectedSensor" },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierFieldValue_Text, nullptr, "config.sensor.multiplier" },
    { "nyquist-warning", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS2MultiplierFlows[] = {
    { "f-next", "Next entry", "config-s3-adjust", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-s2-multiplier-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS2MultiplierEdit_ConfigS2MultiplierEditHdrTitle_Text = { "Edit > Multiplier (F)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS2MultiplierEdit_ConfigS2MultiplierEditRangeHint_Text = { "-32768 to 32767", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2MultiplierEdit_ConfigS2MultiplierEditPendingLabel_Text = { "New value", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2MultiplierEdit_ConfigS2MultiplierEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS2MultiplierEdit_ConfigS2MultiplierEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2MultiplierEdit_ConfigS2MultiplierEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2MultiplierEdit_ConfigS2MultiplierEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold ENTER discard", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS2MultiplierEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigS2MultiplierEdit_ConfigS2MultiplierEditHdrTitle_Text, nullptr, nullptr },
    { "range-hint", ui_exporter::ElementType::Text, 8, 16, 0, 0, &kConfigS2MultiplierEdit_ConfigS2MultiplierEditRangeHint_Text, nullptr, nullptr },
    { "pending-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigS2MultiplierEdit_ConfigS2MultiplierEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigS2MultiplierEdit_ConfigS2MultiplierEditPendingValue_Text, nullptr, "config.sensor.multiplier" },
    { "saved-label", ui_exporter::ElementType::Text, 8, 78, 0, 0, &kConfigS2MultiplierEdit_ConfigS2MultiplierEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigS2MultiplierEdit_ConfigS2MultiplierEditSavedValue_Text, nullptr, "config.sensor.multiplier" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigS2MultiplierEdit_ConfigS2MultiplierEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS2MultiplierEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-s2-multiplier", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-s2-multiplier", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS3Adjust_ConfigS3AdjustHdrTitle_Text = { "Sensor > Adjust", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS3Adjust_ConfigS3AdjustSensorIndex_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3Adjust_ConfigS3AdjustFieldLabel_Text = { "Current", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3Adjust_ConfigS3AdjustFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS3Adjust_ConfigS3AdjustNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3Adjust_ConfigS3AdjustFooterHint_Text = { "UP/DN page  ENTER edit  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS3AdjustElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigS3Adjust_ConfigS3AdjustHdrTitle_Text, nullptr, nullptr },
    { "sensor-index", ui_exporter::ElementType::Value, 200, 2, 0, 0, &kConfigS3Adjust_ConfigS3AdjustSensorIndex_Text, nullptr, "config.selectedSensor" },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigS3Adjust_ConfigS3AdjustFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigS3Adjust_ConfigS3AdjustFieldValue_Text, nullptr, "config.sensor.adjust" },
    { "nyquist-warning", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigS3Adjust_ConfigS3AdjustNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigS3Adjust_ConfigS3AdjustFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS3AdjustFlows[] = {
    { "f-next", "Next entry", "config-s4-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-s2-multiplier", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-s3-adjust-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS3AdjustEdit_ConfigS3AdjustEditHdrTitle_Text = { "Edit > Adjust", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS3AdjustEdit_ConfigS3AdjustEditRangeHint_Text = { "-32768 to 32767", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3AdjustEdit_ConfigS3AdjustEditPendingLabel_Text = { "New value", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3AdjustEdit_ConfigS3AdjustEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS3AdjustEdit_ConfigS3AdjustEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3AdjustEdit_ConfigS3AdjustEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3AdjustEdit_ConfigS3AdjustEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold ENTER discard", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS3AdjustEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigS3AdjustEdit_ConfigS3AdjustEditHdrTitle_Text, nullptr, nullptr },
    { "range-hint", ui_exporter::ElementType::Text, 8, 16, 0, 0, &kConfigS3AdjustEdit_ConfigS3AdjustEditRangeHint_Text, nullptr, nullptr },
    { "pending-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigS3AdjustEdit_ConfigS3AdjustEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigS3AdjustEdit_ConfigS3AdjustEditPendingValue_Text, nullptr, "config.sensor.adjust" },
    { "saved-label", ui_exporter::ElementType::Text, 8, 78, 0, 0, &kConfigS3AdjustEdit_ConfigS3AdjustEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigS3AdjustEdit_ConfigS3AdjustEditSavedValue_Text, nullptr, "config.sensor.adjust" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigS3AdjustEdit_ConfigS3AdjustEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS3AdjustEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-s3-adjust", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-s3-adjust", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowHdrTitle_Text = { "Sensor > Max Flow (Q)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowSensorIndex_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowFieldLabel_Text = { "Current L/min", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowNyquistWarning_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowFooterHint_Text = { "UP/DN page  ENTER edit  hold ENTER exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS4MaxFlowElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowHdrTitle_Text, nullptr, nullptr },
    { "sensor-index", ui_exporter::ElementType::Value, 200, 2, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowSensorIndex_Text, nullptr, "config.selectedSensor" },
    { "field-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowFieldValue_Text, nullptr, "config.sensor.maxFlow" },
    { "nyquist-warning", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowNyquistWarning_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS4MaxFlowFlows[] = {
    { "f-next", "Next entry", "config-sensor-settings-back", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-s3-adjust", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-enter", "Edit value", "config-s4-max-flow-edit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.descend", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditHdrTitle_Text = { "Edit > Max Flow (Q)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditRangeHint_Text = { "0 to 65535 L/min", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditPendingLabel_Text = { "New value", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditPendingValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditSavedLabel_Text = { "Saved", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditSavedValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditFooterHint_Text = { "UP/DN adjust  ENTER save  hold ENTER discard", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS4MaxFlowEditElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditHdrTitle_Text, nullptr, nullptr },
    { "range-hint", ui_exporter::ElementType::Text, 8, 16, 0, 0, &kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditRangeHint_Text, nullptr, nullptr },
    { "pending-label", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditPendingLabel_Text, nullptr, nullptr },
    { "pending-value", ui_exporter::ElementType::Value, 8, 50, 0, 0, &kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditPendingValue_Text, nullptr, "config.sensor.maxFlow" },
    { "saved-label", ui_exporter::ElementType::Text, 8, 78, 0, 0, &kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditSavedLabel_Text, nullptr, nullptr },
    { "saved-value", ui_exporter::ElementType::Value, 8, 92, 0, 0, &kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditSavedValue_Text, nullptr, "config.sensor.maxFlow" },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigS4MaxFlowEdit_ConfigS4MaxFlowEditFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigS4MaxFlowEditFlows[] = {
    { "f-inc", "Increase", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.increment", nullptr, 0 },
    { "f-dec", "Decrease", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.decrement", nullptr, 0 },
    { "f-commit", "Save and go back", "config-s4-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.value.commit", nullptr, 0 },
    { "f-discard", "Discard and go back", "config-s4-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "config.action.value.discard", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigSensorSettingsBack_ConfigSensorSettingsBackHdrTitle_Text = { "Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigSensorSettingsBack_ConfigSensorSettingsBackBackLabel_Text = { "< BACK", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigSensorSettingsBack_ConfigSensorSettingsBackFooterHint_Text = { "ENTER go back", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigSensorSettingsBackElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kConfigSensorSettingsBack_ConfigSensorSettingsBackHdrTitle_Text, nullptr, nullptr },
    { "back-label", ui_exporter::ElementType::Text, 8, 50, 0, 0, &kConfigSensorSettingsBack_ConfigSensorSettingsBackBackLabel_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kConfigSensorSettingsBack_ConfigSensorSettingsBackFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigSensorSettingsBackFlows[] = {
    { "f-next", "Next entry", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous entry", "config-s4-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-back", "Back one level", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.nav.back", nullptr, 0 },
    { "f-escape", "Exit to main screen", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.nav.escape", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP8FactoryReset_InfoP8FactoryResetHdrTitle_Text = { "P8 FACTORY RESET", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP8FactoryReset_InfoP8FactoryResetWarning1_Text = { "Erases all totals, sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP8FactoryReset_InfoP8FactoryResetWarning2_Text = { "config and LED settings.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP8FactoryReset_InfoP8FactoryResetPrompt_Text = { "ENTER to continue >", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP8FactoryReset_InfoP8FactoryResetFooterHint_Text = { "UP/DN page  ENTER confirm screen", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP8FactoryResetElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 8, 2, 0, 0, &kInfoP8FactoryReset_InfoP8FactoryResetHdrTitle_Text, nullptr, nullptr },
    { "warning-1", ui_exporter::ElementType::Text, 8, 30, 0, 0, &kInfoP8FactoryReset_InfoP8FactoryResetWarning1_Text, nullptr, nullptr },
    { "warning-2", ui_exporter::ElementType::Text, 8, 42, 0, 0, &kInfoP8FactoryReset_InfoP8FactoryResetWarning2_Text, nullptr, nullptr },
    { "prompt", ui_exporter::ElementType::Text, 8, 64, 0, 0, &kInfoP8FactoryReset_InfoP8FactoryResetPrompt_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 8, 124, 0, 0, &kInfoP8FactoryReset_InfoP8FactoryResetFooterHint_Text, nullptr, nullptr },
    { "level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 104, nullptr, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP8FactoryResetFlows[] = {
    { "f-next", "Next page", "info-p0-global-status", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Previous page", "info-p7-enter-config", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
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

const ui_exporter::Screen kGeneratedScreens[] = {
    { "info-p0-global-status", "P0 — Global Status", kInfoP0GlobalStatusElements, sizeof(kInfoP0GlobalStatusElements) / sizeof(kInfoP0GlobalStatusElements[0]), kInfoP0GlobalStatusFlows, sizeof(kInfoP0GlobalStatusFlows) / sizeof(kInfoP0GlobalStatusFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p1-instant-flow", "P1 — Instant Flow", kInfoP1InstantFlowElements, sizeof(kInfoP1InstantFlowElements) / sizeof(kInfoP1InstantFlowElements[0]), kInfoP1InstantFlowFlows, sizeof(kInfoP1InstantFlowFlows) / sizeof(kInfoP1InstantFlowFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p2-cumulative-liters", "P2 — Cumulative Liters", kInfoP2CumulativeLitersElements, sizeof(kInfoP2CumulativeLitersElements) / sizeof(kInfoP2CumulativeLitersElements[0]), kInfoP2CumulativeLitersFlows, sizeof(kInfoP2CumulativeLitersFlows) / sizeof(kInfoP2CumulativeLitersFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p3-cumulative-m3", "P3 — Cumulative m³", kInfoP3CumulativeM3Elements, sizeof(kInfoP3CumulativeM3Elements) / sizeof(kInfoP3CumulativeM3Elements[0]), kInfoP3CumulativeM3Flows, sizeof(kInfoP3CumulativeM3Flows) / sizeof(kInfoP3CumulativeM3Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p4-session-liters", "P4 — Session Liters", kInfoP4SessionLitersElements, sizeof(kInfoP4SessionLitersElements) / sizeof(kInfoP4SessionLitersElements[0]), kInfoP4SessionLitersFlows, sizeof(kInfoP4SessionLitersFlows) / sizeof(kInfoP4SessionLitersFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p5-session-m3", "P5 — Session m³", kInfoP5SessionM3Elements, sizeof(kInfoP5SessionM3Elements) / sizeof(kInfoP5SessionM3Elements[0]), kInfoP5SessionM3Flows, sizeof(kInfoP5SessionM3Flows) / sizeof(kInfoP5SessionM3Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p6-max-flow", "P6 — Max Flow Since Reset", kInfoP6MaxFlowElements, sizeof(kInfoP6MaxFlowElements) / sizeof(kInfoP6MaxFlowElements[0]), kInfoP6MaxFlowFlows, sizeof(kInfoP6MaxFlowFlows) / sizeof(kInfoP6MaxFlowFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p7-enter-config", "P7 — Enter Configuration", kInfoP7EnterConfigElements, sizeof(kInfoP7EnterConfigElements) / sizeof(kInfoP7EnterConfigElements[0]), kInfoP7EnterConfigFlows, sizeof(kInfoP7EnterConfigFlows) / sizeof(kInfoP7EnterConfigFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
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
    { "config-s2-multiplier", "S2 — Multiplier (F)", kConfigS2MultiplierElements, sizeof(kConfigS2MultiplierElements) / sizeof(kConfigS2MultiplierElements[0]), kConfigS2MultiplierFlows, sizeof(kConfigS2MultiplierFlows) / sizeof(kConfigS2MultiplierFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s2-multiplier-edit", "S2.V — Edit Multiplier (F)", kConfigS2MultiplierEditElements, sizeof(kConfigS2MultiplierEditElements) / sizeof(kConfigS2MultiplierEditElements[0]), kConfigS2MultiplierEditFlows, sizeof(kConfigS2MultiplierEditFlows) / sizeof(kConfigS2MultiplierEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s3-adjust", "S3 — Adjust", kConfigS3AdjustElements, sizeof(kConfigS3AdjustElements) / sizeof(kConfigS3AdjustElements[0]), kConfigS3AdjustFlows, sizeof(kConfigS3AdjustFlows) / sizeof(kConfigS3AdjustFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s3-adjust-edit", "S3.V — Edit Adjust", kConfigS3AdjustEditElements, sizeof(kConfigS3AdjustEditElements) / sizeof(kConfigS3AdjustEditElements[0]), kConfigS3AdjustEditFlows, sizeof(kConfigS3AdjustEditFlows) / sizeof(kConfigS3AdjustEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s4-max-flow", "S4 — Max Flow (Q)", kConfigS4MaxFlowElements, sizeof(kConfigS4MaxFlowElements) / sizeof(kConfigS4MaxFlowElements[0]), kConfigS4MaxFlowFlows, sizeof(kConfigS4MaxFlowFlows) / sizeof(kConfigS4MaxFlowFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s4-max-flow-edit", "S4.V — Edit Max Flow (Q)", kConfigS4MaxFlowEditElements, sizeof(kConfigS4MaxFlowEditElements) / sizeof(kConfigS4MaxFlowEditElements[0]), kConfigS4MaxFlowEditFlows, sizeof(kConfigS4MaxFlowEditFlows) / sizeof(kConfigS4MaxFlowEditFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-sensor-settings-back", "S.BACK — Back", kConfigSensorSettingsBackElements, sizeof(kConfigSensorSettingsBackElements) / sizeof(kConfigSensorSettingsBackElements[0]), kConfigSensorSettingsBackFlows, sizeof(kConfigSensorSettingsBackFlows) / sizeof(kConfigSensorSettingsBackFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p8-factory-reset", "P8 — Factory Reset", kInfoP8FactoryResetElements, sizeof(kInfoP8FactoryResetElements) / sizeof(kInfoP8FactoryResetElements[0]), kInfoP8FactoryResetFlows, sizeof(kInfoP8FactoryResetFlows) / sizeof(kInfoP8FactoryResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "confirm-reset-totals", "Reset totals?", kConfirmResetTotalsElements, sizeof(kConfirmResetTotalsElements) / sizeof(kConfirmResetTotalsElements[0]), kConfirmResetTotalsFlows, sizeof(kConfirmResetTotalsFlows) / sizeof(kConfirmResetTotalsFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "confirm-reset-session", "Reset session?", kConfirmResetSessionElements, sizeof(kConfirmResetSessionElements) / sizeof(kConfirmResetSessionElements[0]), kConfirmResetSessionFlows, sizeof(kConfirmResetSessionFlows) / sizeof(kConfirmResetSessionFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "confirm-factory-reset", "Factory reset?", kConfirmFactoryResetElements, sizeof(kConfirmFactoryResetElements) / sizeof(kConfirmFactoryResetElements[0]), kConfirmFactoryResetFlows, sizeof(kConfirmFactoryResetFlows) / sizeof(kConfirmFactoryResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "toast-totals-reset", "Totals reset", kToastTotalsResetElements, sizeof(kToastTotalsResetElements) / sizeof(kToastTotalsResetElements[0]), kToastTotalsResetFlows, sizeof(kToastTotalsResetFlows) / sizeof(kToastTotalsResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "toast-session-reset", "Session reset", kToastSessionResetElements, sizeof(kToastSessionResetElements) / sizeof(kToastSessionResetElements[0]), kToastSessionResetFlows, sizeof(kToastSessionResetFlows) / sizeof(kToastSessionResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 }
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
    "2026-07-31T12:47:18.527Z", 48, 375
};

}  // namespace ui_exporter
