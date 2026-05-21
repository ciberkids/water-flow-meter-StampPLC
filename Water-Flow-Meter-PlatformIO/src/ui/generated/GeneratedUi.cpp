#include "GeneratedUi.h"

namespace ui_exporter {

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
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowLegendLed_Text = { "Red=Pulse • Grn=Ready • Blu=Flow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP1InstantFlow_InfoP1InstantFlowFooterHint_Text = { "↑↓ pages  ENTER 3s→idle", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

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
    { "divider-1", ui_exporter::ElementType::Box, 0, 65, 135, 1, nullptr, nullptr, nullptr },
    { "propeller", ui_exporter::ElementType::Text, 40, 70, 55, 55, nullptr, nullptr, nullptr },
    { "divider-2", ui_exporter::ElementType::Box, 0, 130, 135, 1, nullptr, nullptr, nullptr },
    { "legend-led", ui_exporter::ElementType::Text, 2, 133, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowLegendLed_Text, nullptr, "legend.led" },
    { "scroll-pos", ui_exporter::ElementType::Text, 131, 14, 4, 120, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kInfoP1InstantFlow_InfoP1InstantFlowFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP1InstantFlowFlows[] = {
    { "f-next", "Next page", "info-p2-cumulative-liters", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p7-enter-config", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-idle", "Enter idle", "state-idle", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, "ui.action.mode.idle", nullptr, 0 }
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
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersLegendLed_Text = { "Red=Pulse • Grn=Ready • Blu=Flow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersFooterHint_Text = { "ENTER→30s reset  ↑↓ pages", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP2CumulativeLiters_InfoP2CumulativeLitersUndersamplingBadge_Text = { "⚠ Undersampling", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

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
    { "divider-1", ui_exporter::ElementType::Box, 0, 65, 135, 1, nullptr, nullptr, nullptr },
    { "legend-led", ui_exporter::ElementType::Text, 2, 70, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersLegendLed_Text, nullptr, "legend.led" },
    { "scroll-pos", ui_exporter::ElementType::Text, 131, 14, 4, 120, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersFooterHint_Text, nullptr, nullptr },
    { "undersampling-badge", ui_exporter::ElementType::Badge, 2, 4, 0, 0, &kInfoP2CumulativeLiters_InfoP2CumulativeLitersUndersamplingBadge_Text, nullptr, "diagnostics.undersampling" }
};


static constexpr ui_exporter::Flow kInfoP2CumulativeLitersFlows[] = {
    { "f-next", "Next page", "info-p3-cumulative-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p1-instant-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-reset-all", "Reset all countdown", "countdown-reset-all", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3HdrTitle_Text = { "Cumulative (m³)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
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
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3LegendLed_Text = { "Red=Pulse • Grn=Ready • Blu=Flow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3FooterHint_Text = { "ENTER→30s reset  ↑↓ pages", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP3CumulativeM3_InfoP3CumulativeM3UndersamplingBadge_Text = { "⚠ Undersampling", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

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
    { "divider-1", ui_exporter::ElementType::Box, 0, 65, 135, 1, nullptr, nullptr, nullptr },
    { "legend-led", ui_exporter::ElementType::Text, 2, 70, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3LegendLed_Text, nullptr, "legend.led" },
    { "scroll-pos", ui_exporter::ElementType::Text, 131, 14, 4, 120, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3FooterHint_Text, nullptr, nullptr },
    { "undersampling-badge", ui_exporter::ElementType::Badge, 2, 4, 0, 0, &kInfoP3CumulativeM3_InfoP3CumulativeM3UndersamplingBadge_Text, nullptr, "diagnostics.undersampling" }
};


static constexpr ui_exporter::Flow kInfoP3CumulativeM3Flows[] = {
    { "f-next", "Next page", "info-p4-session-liters", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p2-cumulative-liters", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-reset-all", "Reset all countdown", "countdown-reset-all", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
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
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersLegendLed_Text = { "Red=Pulse • Grn=Ready • Blu=Flow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP4SessionLiters_InfoP4SessionLitersFooterHint_Text = { "ENTER→3s reset  ↑↓ pages", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

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
    { "legend-led", ui_exporter::ElementType::Text, 2, 78, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersLegendLed_Text, nullptr, "legend.led" },
    { "scroll-pos", ui_exporter::ElementType::Text, 131, 14, 4, 120, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kInfoP4SessionLiters_InfoP4SessionLitersFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP4SessionLitersFlows[] = {
    { "f-next", "Next page", "info-p5-session-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p3-cumulative-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-countdown", "Session reset countdown", "countdown-reset-session", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3HdrTitle_Text = { "Session (m³)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
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
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3LegendLed_Text = { "Red=Pulse • Grn=Ready • Blu=Flow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3FooterHint_Text = { "ENTER→3s reset  ↑↓ pages", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP5SessionM3_InfoP5SessionM3UndersamplingBadge_Text = { "⚠ Undersampling", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

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
    { "legend-led", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3LegendLed_Text, nullptr, "legend.led" },
    { "scroll-pos", ui_exporter::ElementType::Text, 131, 14, 4, 120, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3FooterHint_Text, nullptr, nullptr },
    { "undersampling-badge", ui_exporter::ElementType::Badge, 2, 4, 0, 0, &kInfoP5SessionM3_InfoP5SessionM3UndersamplingBadge_Text, nullptr, "diagnostics.undersampling" }
};


static constexpr ui_exporter::Flow kInfoP5SessionM3Flows[] = {
    { "f-next", "Next page", "info-p6-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p4-session-liters", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-countdown", "Session reset countdown", "countdown-reset-session", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
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
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowLegendLed_Text = { "Red=Pulse • Grn=Ready • Blu=Flow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowFooterHint_Text = { "ENTER→3s reset  ↑↓ pages", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP6MaxFlow_InfoP6MaxFlowUndersamplingBadge_Text = { "⚠ Undersampling", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

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
    { "legend-led", ui_exporter::ElementType::Text, 2, 66, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowLegendLed_Text, nullptr, "legend.led" },
    { "scroll-pos", ui_exporter::ElementType::Text, 131, 14, 4, 120, nullptr, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowFooterHint_Text, nullptr, nullptr },
    { "undersampling-badge", ui_exporter::ElementType::Badge, 2, 4, 0, 0, &kInfoP6MaxFlow_InfoP6MaxFlowUndersamplingBadge_Text, nullptr, "diagnostics.undersampling" }
};


static constexpr ui_exporter::Flow kInfoP6MaxFlowFlows[] = {
    { "f-next", "Next page", "info-p7-enter-config", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p5-session-m3", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-countdown", "Session reset countdown", "countdown-reset-session", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kInfoP7EnterConfig_InfoP7EnterConfigHdrTitle_Text = { "Configuration", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kInfoP7EnterConfig_InfoP7EnterConfigPromptLine1_Text = { "Press & hold ENTER", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP7EnterConfig_InfoP7EnterConfigPromptLine2_Text = { "to enter Config mode.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kInfoP7EnterConfig_InfoP7EnterConfigLegendLed_Text = { "Red=Pulse • Grn=Ready • Blu=Flow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kInfoP7EnterConfig_InfoP7EnterConfigFooterHint_Text = { "Short ENTER→3s config countdown", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kInfoP7EnterConfigElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kInfoP7EnterConfig_InfoP7EnterConfigHdrTitle_Text, nullptr, nullptr },
    { "prompt-line1", ui_exporter::ElementType::Text, 10, 90, 0, 0, &kInfoP7EnterConfig_InfoP7EnterConfigPromptLine1_Text, nullptr, nullptr },
    { "prompt-line2", ui_exporter::ElementType::Text, 10, 104, 0, 0, &kInfoP7EnterConfig_InfoP7EnterConfigPromptLine2_Text, nullptr, nullptr },
    { "divider-1", ui_exporter::ElementType::Box, 20, 120, 95, 1, nullptr, nullptr, nullptr },
    { "scroll-pos", ui_exporter::ElementType::Text, 131, 14, 4, 120, nullptr, nullptr, nullptr },
    { "legend-led", ui_exporter::ElementType::Text, 2, 214, 0, 0, &kInfoP7EnterConfig_InfoP7EnterConfigLegendLed_Text, nullptr, "legend.led" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kInfoP7EnterConfig_InfoP7EnterConfigFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kInfoP7EnterConfigFlows[] = {
    { "f-next", "Next page", "info-p1-instant-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.next", nullptr, 0 },
    { "f-prev", "Prev page", "info-p6-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.page.previous", nullptr, 0 },
    { "f-config-countdown", "Config countdown", "countdown-enter-config", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kCountdownEnterConfig_CountdownEnterConfigTitle_Text = { "Entering Config…", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kCountdownEnterConfig_CountdownEnterConfigTimerValue_Text = { "3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdownEnterConfig_CountdownEnterConfigHint_Text = { "Hold ENTER. Release to cancel.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };

static constexpr ui_exporter::Element kCountdownEnterConfigElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 135, 240, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 10, 80, 0, 0, &kCountdownEnterConfig_CountdownEnterConfigTitle_Text, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 50, 110, 0, 0, &kCountdownEnterConfig_CountdownEnterConfigTimerValue_Text, nullptr, "countdown.value" },
    { "hint", ui_exporter::ElementType::Text, 5, 150, 0, 0, &kCountdownEnterConfig_CountdownEnterConfigHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kCountdownEnterConfigFlows[] = {
    { "f-confirm", "Enter config mode", "config-c1-modbus-id", ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::None, ui_exporter::FlowGesture::Short, 3000, nullptr, nullptr, nullptr, "ui.action.mode.configuration", nullptr, 0 },
    { "f-cancel", "Cancel", "info-p7-enter-config", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kCountdownResetSession_CountdownResetSessionTitle_Text = { "Reset session?", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kCountdownResetSession_CountdownResetSessionTimerValue_Text = { "3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdownResetSession_CountdownResetSessionHint_Text = { "Hold ENTER. Release to cancel.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };

static constexpr ui_exporter::Element kCountdownResetSessionElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 135, 240, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 5, 80, 0, 0, &kCountdownResetSession_CountdownResetSessionTitle_Text, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 50, 110, 0, 0, &kCountdownResetSession_CountdownResetSessionTimerValue_Text, nullptr, "countdown.value" },
    { "hint", ui_exporter::ElementType::Text, 5, 150, 0, 0, &kCountdownResetSession_CountdownResetSessionHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kCountdownResetSessionFlows[] = {
    { "f-confirm", "Reset session", "info-p4-session-liters", ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::None, ui_exporter::FlowGesture::Short, 3000, nullptr, nullptr, nullptr, "core.action.reset-session", nullptr, 0 },
    { "f-cancel", "Cancel", "info-p4-session-liters", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kCountdownResetAll_CountdownResetAllTitle_Text = { "Reset all totals?", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kCountdownResetAll_CountdownResetAllWarning_Text = { "Hold ENTER to reset totals", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kCountdownResetAll_CountdownResetAllTimerValue_Text = { "30", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdownResetAll_CountdownResetAllHint1_Text = { "(30 → 0)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdownResetAll_CountdownResetAllHint2_Text = { "Release to cancel.", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };

static constexpr ui_exporter::Element kCountdownResetAllElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 135, 240, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 5, 60, 0, 0, &kCountdownResetAll_CountdownResetAllTitle_Text, nullptr, nullptr },
    { "warning", ui_exporter::ElementType::Text, 5, 76, 0, 0, &kCountdownResetAll_CountdownResetAllWarning_Text, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 40, 100, 0, 0, &kCountdownResetAll_CountdownResetAllTimerValue_Text, nullptr, "countdown.value" },
    { "hint-1", ui_exporter::ElementType::Text, 5, 140, 0, 0, &kCountdownResetAll_CountdownResetAllHint1_Text, nullptr, nullptr },
    { "hint-2", ui_exporter::ElementType::Text, 5, 155, 0, 0, &kCountdownResetAll_CountdownResetAllHint2_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kCountdownResetAllFlows[] = {
    { "f-confirm", "Reset all measured", "info-p2-cumulative-liters", ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::None, ui_exporter::FlowGesture::Short, 30000, nullptr, nullptr, nullptr, "core.action.reset-all-measured", nullptr, 0 },
    { "f-cancel", "Cancel", "info-p2-cumulative-liters", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kCountdownFactoryReset_CountdownFactoryResetTitle_Text = { "⚠ FACTORY RESET", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kCountdownFactoryReset_CountdownFactoryResetWarningText_Text = { "Keep holding UP+DOWN", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kCountdownFactoryReset_CountdownFactoryResetWarningDetail_Text = { "Wipes NVS & Modbus config!", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kCountdownFactoryReset_CountdownFactoryResetTimerValue_Text = { "30", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdownFactoryReset_CountdownFactoryResetTimerUnit_Text = { "s", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdownFactoryReset_CountdownFactoryResetFooterHint_Text = { "Release to cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kCountdownFactoryResetElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 135, 240, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 5, 50, 0, 0, &kCountdownFactoryReset_CountdownFactoryResetTitle_Text, nullptr, nullptr },
    { "warning-text", ui_exporter::ElementType::Text, 5, 70, 0, 0, &kCountdownFactoryReset_CountdownFactoryResetWarningText_Text, nullptr, nullptr },
    { "warning-detail", ui_exporter::ElementType::Text, 5, 86, 0, 0, &kCountdownFactoryReset_CountdownFactoryResetWarningDetail_Text, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 40, 110, 0, 0, &kCountdownFactoryReset_CountdownFactoryResetTimerValue_Text, nullptr, "countdown.value" },
    { "timer-unit", ui_exporter::ElementType::Text, 70, 110, 0, 0, &kCountdownFactoryReset_CountdownFactoryResetTimerUnit_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 5, 226, 0, 0, &kCountdownFactoryReset_CountdownFactoryResetFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kCountdownFactoryResetFlows[] = {
    { "f-execute", "Execute factory reset", "info-p1-instant-flow", ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::None, ui_exporter::FlowGesture::Short, 30000, nullptr, nullptr, nullptr, "core.action.factory-reset", nullptr, 0 },
    { "f-cancel", "Cancel", "info-p1-instant-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kCountdownSensorSave_CountdownSensorSaveTitle_Text = { "Save & Validate?", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kCountdownSensorSave_CountdownSensorSaveWarning_Text = { "Hold ENTER to save", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kCountdownSensorSave_CountdownSensorSaveTimerValue_Text = { "3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdownSensorSave_CountdownSensorSaveTimerUnit_Text = { "s", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdownSensorSave_CountdownSensorSaveFooterHint_Text = { "Release to cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kCountdownSensorSaveElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 135, 240, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 5, 60, 0, 0, &kCountdownSensorSave_CountdownSensorSaveTitle_Text, nullptr, nullptr },
    { "warning", ui_exporter::ElementType::Text, 5, 76, 0, 0, &kCountdownSensorSave_CountdownSensorSaveWarning_Text, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 55, 100, 0, 0, &kCountdownSensorSave_CountdownSensorSaveTimerValue_Text, nullptr, "countdown.value" },
    { "timer-unit", ui_exporter::ElementType::Text, 72, 100, 0, 0, &kCountdownSensorSave_CountdownSensorSaveTimerUnit_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 5, 226, 0, 0, &kCountdownSensorSave_CountdownSensorSaveFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kCountdownSensorSaveFlows[] = {
    { "f-confirm", "Save sensor settings", "config-s1-connected", ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::None, ui_exporter::FlowGesture::Short, 3000, nullptr, nullptr, nullptr, "config.action.sensor.save", nullptr, 0 },
    { "f-cancel", "Cancel", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kCountdownConfigExit_CountdownConfigExitTitle_Text = { "Exit Configuration?", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kCountdownConfigExit_CountdownConfigExitWarning_Text = { "Hold ENTER to exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kCountdownConfigExit_CountdownConfigExitTimerValue_Text = { "3", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdownConfigExit_CountdownConfigExitTimerUnit_Text = { "s", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kCountdownConfigExit_CountdownConfigExitFooterHint_Text = { "Release to cancel", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kCountdownConfigExitElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 135, 240, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 5, 60, 0, 0, &kCountdownConfigExit_CountdownConfigExitTitle_Text, nullptr, nullptr },
    { "warning", ui_exporter::ElementType::Text, 5, 76, 0, 0, &kCountdownConfigExit_CountdownConfigExitWarning_Text, nullptr, nullptr },
    { "timer-value", ui_exporter::ElementType::Value, 55, 100, 0, 0, &kCountdownConfigExit_CountdownConfigExitTimerValue_Text, nullptr, "countdown.value" },
    { "timer-unit", ui_exporter::ElementType::Text, 72, 100, 0, 0, &kCountdownConfigExit_CountdownConfigExitTimerUnit_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 5, 226, 0, 0, &kCountdownConfigExit_CountdownConfigExitFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kCountdownConfigExitFlows[] = {
    { "f-confirm", "Exit to info mode", "state-idle", ui_exporter::FlowTrigger::Timeout, ui_exporter::FlowButton::None, ui_exporter::FlowGesture::Short, 3000, nullptr, nullptr, nullptr, "ui.action.mode.info", nullptr, 0 },
    { "f-cancel", "Cancel", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningTitle_Text = { "⚠ Sampling too slow", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningDetail1_Text = { "f_theoretical exceeds", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningDetail2_Text = { "Nyquist limit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningNyquistValue_Text = { "—", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningOptionUp_Text = { "UP = Edit values", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningOptionDown_Text = { "DOWN = Save anyway", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kNyquistWarning_NyquistWarningFooterHint_Text = { "Choose UP or DOWN", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kNyquistWarningElements[] = {
    { "overlay-bg", ui_exporter::ElementType::Box, 0, 0, 135, 240, nullptr, nullptr, nullptr },
    { "title", ui_exporter::ElementType::Text, 5, 40, 0, 0, &kNyquistWarning_NyquistWarningTitle_Text, nullptr, nullptr },
    { "detail-1", ui_exporter::ElementType::Text, 5, 60, 0, 0, &kNyquistWarning_NyquistWarningDetail1_Text, nullptr, nullptr },
    { "detail-2", ui_exporter::ElementType::Text, 5, 72, 0, 0, &kNyquistWarning_NyquistWarningDetail2_Text, nullptr, nullptr },
    { "nyquist-value", ui_exporter::ElementType::Value, 5, 92, 0, 0, &kNyquistWarning_NyquistWarningNyquistValue_Text, nullptr, "config.sensor.nyquistWarning" },
    { "option-up", ui_exporter::ElementType::Text, 5, 130, 0, 0, &kNyquistWarning_NyquistWarningOptionUp_Text, nullptr, nullptr },
    { "option-down", ui_exporter::ElementType::Text, 5, 150, 0, 0, &kNyquistWarning_NyquistWarningOptionDown_Text, nullptr, nullptr },
    { "footer-hint", ui_exporter::ElementType::Text, 5, 226, 0, 0, &kNyquistWarning_NyquistWarningFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kNyquistWarningFlows[] = {
    { "f-edit", "Return to edit", "config-s4-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-force-save", "Force save (ignore warning)", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.sensor.save", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kStateIdle_StateIdleIdlePlaceholder_Text = { "— Display off —", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kStateIdleElements[] = {
    { "idle-placeholder", ui_exporter::ElementType::Text, 2, 110, 0, 0, &kStateIdle_StateIdleIdlePlaceholder_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kStateIdleFlows[] = {
    { "f-wake", "Wake to info", "info-p1-instant-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.mode.info", nullptr, 0 },
    { "f-wake-up", "Wake to info", "info-p1-instant-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.mode.info", nullptr, 0 },
    { "f-wake-down", "Wake to info", "info-p1-instant-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "ui.action.mode.info", nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdHdrTitle_Text = { "Config: Modbus ID", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdFieldLabel_Text = { "Slave ID (1–255)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC1ModbusId_ConfigC1ModbusIdFooterHint_Text = { "ENTER=edit  ↑↓=adjust  Long=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC1ModbusIdElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 4, 90, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdFieldValue_Text, nullptr, "config.modbusSlaveId" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigC1ModbusId_ConfigC1ModbusIdFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfigC1ModbusId_Flow2ActionParams[] = {
    { "fieldId", "modbusSlaveId" }
};

static constexpr ui_exporter::Flow kConfigC1ModbusIdFlows[] = {
    { "f-next", "Next config", "config-c2-baud-rate", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Prev config", "config-c7-sensor-select", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-edit", "Edit field", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.field.edit", kConfigC1ModbusId_Flow2ActionParams, sizeof(kConfigC1ModbusId_Flow2ActionParams) / sizeof(kConfigC1ModbusId_Flow2ActionParams[0]) },
    { "f-exit", "Exit config", "countdown-config-exit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateHdrTitle_Text = { "Config: Baud Rate", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateFieldLabel_Text = { "Baud Rate", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC2BaudRate_ConfigC2BaudRateFooterHint_Text = { "ENTER=cycle  Long=exit config", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC2BaudRateElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 4, 90, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateFieldValue_Text, nullptr, "config.baudRate" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigC2BaudRate_ConfigC2BaudRateFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfigC2BaudRate_Flow2ActionParams[] = {
    { "fieldId", "baudRate" }
};

static constexpr ui_exporter::Flow kConfigC2BaudRateFlows[] = {
    { "f-next", "Next config", "config-c3-parity", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Prev config", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-edit", "Cycle baud rate", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.field.edit", kConfigC2BaudRate_Flow2ActionParams, sizeof(kConfigC2BaudRate_Flow2ActionParams) / sizeof(kConfigC2BaudRate_Flow2ActionParams[0]) },
    { "f-exit", "Exit config", "countdown-config-exit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityHdrTitle_Text = { "Config: Parity", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityFieldLabel_Text = { "Parity", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityFrameSummary_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigC3Parity_ConfigC3ParityFooterHint_Text = { "ENTER=cycle  Long=exit config", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC3ParityElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC3Parity_ConfigC3ParityHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 4, 90, 0, 0, &kConfigC3Parity_ConfigC3ParityFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigC3Parity_ConfigC3ParityFieldValue_Text, nullptr, "config.parity" },
    { "frame-summary", ui_exporter::ElementType::Value, 4, 118, 0, 0, &kConfigC3Parity_ConfigC3ParityFrameSummary_Text, nullptr, "config.uartFrameSummary" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigC3Parity_ConfigC3ParityFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfigC3Parity_Flow2ActionParams[] = {
    { "fieldId", "parity" }
};

static constexpr ui_exporter::Flow kConfigC3ParityFlows[] = {
    { "f-next", "Next config", "config-c4-stop-bits", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Prev config", "config-c2-baud-rate", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-edit", "Cycle parity", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.field.edit", kConfigC3Parity_Flow2ActionParams, sizeof(kConfigC3Parity_Flow2ActionParams) / sizeof(kConfigC3Parity_Flow2ActionParams[0]) },
    { "f-exit", "Exit config", "countdown-config-exit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsHdrTitle_Text = { "Config: Stop Bits", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsFieldLabel_Text = { "Stop Bits", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC4StopBits_ConfigC4StopBitsFooterHint_Text = { "ENTER=toggle  Long=exit config", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC4StopBitsElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 4, 90, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsFieldValue_Text, nullptr, "config.stopBits" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigC4StopBits_ConfigC4StopBitsFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfigC4StopBits_Flow2ActionParams[] = {
    { "fieldId", "stopBits" }
};

static constexpr ui_exporter::Flow kConfigC4StopBitsFlows[] = {
    { "f-next", "Next config", "config-c5-led-pulse-vol", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Prev config", "config-c3-parity", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-edit", "Toggle stop bits", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.field.edit", kConfigC4StopBits_Flow2ActionParams, sizeof(kConfigC4StopBits_Flow2ActionParams) / sizeof(kConfigC4StopBits_Flow2ActionParams[0]) },
    { "f-exit", "Exit config", "countdown-config-exit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolHdrTitle_Text = { "Config: LED Volume", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolFieldLabel_Text = { "Pulse Volume", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC5LedPulseVol_ConfigC5LedPulseVolFooterHint_Text = { "ENTER=cycle  Long=exit config", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC5LedPulseVolElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 4, 90, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolFieldValue_Text, nullptr, "config.ledPulseVolume" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigC5LedPulseVol_ConfigC5LedPulseVolFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfigC5LedPulseVol_Flow2ActionParams[] = {
    { "fieldId", "ledPulseVolume" }
};

static constexpr ui_exporter::Flow kConfigC5LedPulseVolFlows[] = {
    { "f-next", "Next config", "config-c6-led-pulse-period", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Prev config", "config-c4-stop-bits", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-edit", "Cycle pulse volume", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.field.edit", kConfigC5LedPulseVol_Flow2ActionParams, sizeof(kConfigC5LedPulseVol_Flow2ActionParams) / sizeof(kConfigC5LedPulseVol_Flow2ActionParams[0]) },
    { "f-exit", "Exit config", "countdown-config-exit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodHdrTitle_Text = { "Config: LED Period", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFieldLabel_Text = { "Pulse Period (ms)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFooterHint_Text = { "ENTER=edit  ↑↓=adjust  Long=exit", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC6LedPulsePeriodElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 4, 90, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFieldValue_Text, nullptr, "config.ledPulsePeriod" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigC6LedPulsePeriod_ConfigC6LedPulsePeriodFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfigC6LedPulsePeriod_Flow2ActionParams[] = {
    { "fieldId", "ledPulsePeriod" }
};

static constexpr ui_exporter::Flow kConfigC6LedPulsePeriodFlows[] = {
    { "f-next", "Next config", "config-c7-sensor-select", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Prev config", "config-c5-led-pulse-vol", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-edit", "Edit period", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.field.edit", kConfigC6LedPulsePeriod_Flow2ActionParams, sizeof(kConfigC6LedPulsePeriod_Flow2ActionParams) / sizeof(kConfigC6LedPulsePeriod_Flow2ActionParams[0]) },
    { "f-exit", "Exit config", "countdown-config-exit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectHdrTitle_Text = { "Config: Sensor", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectFieldLabel_Text = { "Select sensor (1–8)", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigC7SensorSelect_ConfigC7SensorSelectFooterHint_Text = { "↑↓=select  ENTER=open sub-menu", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigC7SensorSelectElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectHdrTitle_Text, nullptr, nullptr },
    { "field-label", ui_exporter::ElementType::Text, 4, 90, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectFieldLabel_Text, nullptr, nullptr },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectFieldValue_Text, nullptr, "config.selectedSensor" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigC7SensorSelect_ConfigC7SensorSelectFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::Flow kConfigC7SensorSelectFlows[] = {
    { "f-next", "Next config", "config-c1-modbus-id", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Prev config", "config-c6-led-pulse-period", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-open-sensor", "Open sensor sub-menu", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-exit", "Exit config", "countdown-config-exit", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedHdrTitle_Text = { "Sensor: Connected", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedSensorLabel_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS1Connected_ConfigS1ConnectedFooterHint_Text = { "ENTER=toggle  Long→save&validate", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS1ConnectedElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS1Connected_ConfigS1ConnectedHdrTitle_Text, nullptr, nullptr },
    { "sensor-label", ui_exporter::ElementType::Value, 4, 18, 0, 0, &kConfigS1Connected_ConfigS1ConnectedSensorLabel_Text, nullptr, "config.selectedSensor" },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigS1Connected_ConfigS1ConnectedFieldValue_Text, nullptr, "config.sensor.connected" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigS1Connected_ConfigS1ConnectedFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfigS1Connected_Flow2ActionParams[] = {
    { "fieldId", "connected" }
};

static constexpr ui_exporter::Flow kConfigS1ConnectedFlows[] = {
    { "f-next", "Next sensor page", "config-s2-multiplier", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Back to sensor select", "config-c7-sensor-select", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-toggle", "Toggle connected", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.field.edit", kConfigS1Connected_Flow2ActionParams, sizeof(kConfigS1Connected_Flow2ActionParams) / sizeof(kConfigS1Connected_Flow2ActionParams[0]) },
    { "f-save", "Save & validate", "countdown-sensor-save", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-inactive-exit", "Exit if sensor inactive", "config-c7-sensor-select", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, "sensor.connected == false", nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierHdrTitle_Text = { "Sensor: Multiplier F", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierSensorLabel_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierUndersamplingWarn_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS2Multiplier_ConfigS2MultiplierFooterHint_Text = { "↑↓=adjust  ENTER=commit  Long=save", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS2MultiplierElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierHdrTitle_Text, nullptr, nullptr },
    { "sensor-label", ui_exporter::ElementType::Value, 4, 18, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierSensorLabel_Text, nullptr, "config.selectedSensor" },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierFieldValue_Text, nullptr, "config.sensor.multiplier" },
    { "undersampling-warn", ui_exporter::ElementType::Badge, 4, 120, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierUndersamplingWarn_Text, nullptr, "config.sensor.undersamplingFlag" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigS2Multiplier_ConfigS2MultiplierFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfigS2Multiplier_Flow2ActionParams[] = {
    { "fieldId", "multiplier" }
};

static constexpr ui_exporter::Flow kConfigS2MultiplierFlows[] = {
    { "f-next", "Next sensor page", "config-s3-adjust", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Prev sensor page", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-commit", "Commit value", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.field.confirm", kConfigS2Multiplier_Flow2ActionParams, sizeof(kConfigS2Multiplier_Flow2ActionParams) / sizeof(kConfigS2Multiplier_Flow2ActionParams[0]) },
    { "f-save", "Save & validate", "countdown-sensor-save", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS3Adjust_ConfigS3AdjustHdrTitle_Text = { "Sensor: Adjust", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS3Adjust_ConfigS3AdjustSensorLabel_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS3Adjust_ConfigS3AdjustFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS3Adjust_ConfigS3AdjustFooterHint_Text = { "↑↓=adjust  ENTER=commit  Long=save", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS3AdjustElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS3Adjust_ConfigS3AdjustHdrTitle_Text, nullptr, nullptr },
    { "sensor-label", ui_exporter::ElementType::Value, 4, 18, 0, 0, &kConfigS3Adjust_ConfigS3AdjustSensorLabel_Text, nullptr, "config.selectedSensor" },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigS3Adjust_ConfigS3AdjustFieldValue_Text, nullptr, "config.sensor.adjust" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigS3Adjust_ConfigS3AdjustFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfigS3Adjust_Flow2ActionParams[] = {
    { "fieldId", "adjust" }
};

static constexpr ui_exporter::Flow kConfigS3AdjustFlows[] = {
    { "f-next", "Next sensor page", "config-s4-max-flow", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Prev sensor page", "config-s2-multiplier", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-commit", "Commit value", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.field.confirm", kConfigS3Adjust_Flow2ActionParams, sizeof(kConfigS3Adjust_Flow2ActionParams) / sizeof(kConfigS3Adjust_Flow2ActionParams[0]) },
    { "f-save", "Save & validate", "countdown-sensor-save", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowHdrTitle_Text = { "Sensor: Max Flow Q", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowSensorLabel_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowFieldValue_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowFieldUnit_Text = { "L/min", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowNyquistWarn_Text = { "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Normal };
static constexpr ui_exporter::TextPayload kConfigS4MaxFlow_ConfigS4MaxFlowFooterHint_Text = { "↑↓=adjust  ENTER=commit  Long=save", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted };

static constexpr ui_exporter::Element kConfigS4MaxFlowElements[] = {
    { "hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowHdrTitle_Text, nullptr, nullptr },
    { "sensor-label", ui_exporter::ElementType::Value, 4, 18, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowSensorLabel_Text, nullptr, "config.selectedSensor" },
    { "field-value", ui_exporter::ElementType::Value, 4, 104, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowFieldValue_Text, nullptr, "config.sensor.maxFlow" },
    { "field-unit", ui_exporter::ElementType::Text, 60, 104, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowFieldUnit_Text, nullptr, nullptr },
    { "nyquist-warn", ui_exporter::ElementType::Badge, 4, 120, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowNyquistWarn_Text, nullptr, "config.sensor.nyquistWarning" },
    { "footer-hint", ui_exporter::ElementType::Text, 2, 226, 0, 0, &kConfigS4MaxFlow_ConfigS4MaxFlowFooterHint_Text, nullptr, nullptr }
};


static constexpr ui_exporter::KeyValue kConfigS4MaxFlow_Flow2ActionParams[] = {
    { "fieldId", "maxFlow" }
};

static constexpr ui_exporter::Flow kConfigS4MaxFlowFlows[] = {
    { "f-next", "Back to S1", "config-s1-connected", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-prev", "Prev sensor page", "config-s3-adjust", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 },
    { "f-commit", "Commit value", nullptr, ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "config.action.field.confirm", kConfigS4MaxFlow_Flow2ActionParams, sizeof(kConfigS4MaxFlow_Flow2ActionParams) / sizeof(kConfigS4MaxFlow_Flow2ActionParams[0]) },
    { "f-save", "Save & validate", "countdown-sensor-save", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Long, 0, nullptr, nullptr, nullptr, nullptr, nullptr, 0 }
};

const ui_exporter::Screen kGeneratedScreens[] = {
    { "info-p1-instant-flow", "P1 — Instant Flow", kInfoP1InstantFlowElements, sizeof(kInfoP1InstantFlowElements) / sizeof(kInfoP1InstantFlowElements[0]), kInfoP1InstantFlowFlows, sizeof(kInfoP1InstantFlowFlows) / sizeof(kInfoP1InstantFlowFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p2-cumulative-liters", "P2 — Cumulative Liters", kInfoP2CumulativeLitersElements, sizeof(kInfoP2CumulativeLitersElements) / sizeof(kInfoP2CumulativeLitersElements[0]), kInfoP2CumulativeLitersFlows, sizeof(kInfoP2CumulativeLitersFlows) / sizeof(kInfoP2CumulativeLitersFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p3-cumulative-m3", "P3 — Cumulative m³", kInfoP3CumulativeM3Elements, sizeof(kInfoP3CumulativeM3Elements) / sizeof(kInfoP3CumulativeM3Elements[0]), kInfoP3CumulativeM3Flows, sizeof(kInfoP3CumulativeM3Flows) / sizeof(kInfoP3CumulativeM3Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p4-session-liters", "P4 — Session Liters", kInfoP4SessionLitersElements, sizeof(kInfoP4SessionLitersElements) / sizeof(kInfoP4SessionLitersElements[0]), kInfoP4SessionLitersFlows, sizeof(kInfoP4SessionLitersFlows) / sizeof(kInfoP4SessionLitersFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p5-session-m3", "P5 — Session m³", kInfoP5SessionM3Elements, sizeof(kInfoP5SessionM3Elements) / sizeof(kInfoP5SessionM3Elements[0]), kInfoP5SessionM3Flows, sizeof(kInfoP5SessionM3Flows) / sizeof(kInfoP5SessionM3Flows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p6-max-flow", "P6 — Max Flow Since Reset", kInfoP6MaxFlowElements, sizeof(kInfoP6MaxFlowElements) / sizeof(kInfoP6MaxFlowElements[0]), kInfoP6MaxFlowFlows, sizeof(kInfoP6MaxFlowFlows) / sizeof(kInfoP6MaxFlowFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "info-p7-enter-config", "P7 — Enter Configuration", kInfoP7EnterConfigElements, sizeof(kInfoP7EnterConfigElements) / sizeof(kInfoP7EnterConfigElements[0]), kInfoP7EnterConfigFlows, sizeof(kInfoP7EnterConfigFlows) / sizeof(kInfoP7EnterConfigFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "countdown-enter-config", "Countdown — Enter Config", kCountdownEnterConfigElements, sizeof(kCountdownEnterConfigElements) / sizeof(kCountdownEnterConfigElements[0]), kCountdownEnterConfigFlows, sizeof(kCountdownEnterConfigFlows) / sizeof(kCountdownEnterConfigFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "countdown-reset-session", "Countdown — Reset Session", kCountdownResetSessionElements, sizeof(kCountdownResetSessionElements) / sizeof(kCountdownResetSessionElements[0]), kCountdownResetSessionFlows, sizeof(kCountdownResetSessionFlows) / sizeof(kCountdownResetSessionFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "countdown-reset-all", "Countdown — Reset All Measured", kCountdownResetAllElements, sizeof(kCountdownResetAllElements) / sizeof(kCountdownResetAllElements[0]), kCountdownResetAllFlows, sizeof(kCountdownResetAllFlows) / sizeof(kCountdownResetAllFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "countdown-factory-reset", "Factory Reset Countdown", kCountdownFactoryResetElements, sizeof(kCountdownFactoryResetElements) / sizeof(kCountdownFactoryResetElements[0]), kCountdownFactoryResetFlows, sizeof(kCountdownFactoryResetFlows) / sizeof(kCountdownFactoryResetFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "countdown-sensor-save", "Save & Validate Countdown", kCountdownSensorSaveElements, sizeof(kCountdownSensorSaveElements) / sizeof(kCountdownSensorSaveElements[0]), kCountdownSensorSaveFlows, sizeof(kCountdownSensorSaveFlows) / sizeof(kCountdownSensorSaveFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "countdown-config-exit", "Exit Configuration Countdown", kCountdownConfigExitElements, sizeof(kCountdownConfigExitElements) / sizeof(kCountdownConfigExitElements[0]), kCountdownConfigExitFlows, sizeof(kCountdownConfigExitFlows) / sizeof(kCountdownConfigExitFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "nyquist-warning", "Nyquist Validation Warning", kNyquistWarningElements, sizeof(kNyquistWarningElements) / sizeof(kNyquistWarningElements[0]), kNyquistWarningFlows, sizeof(kNyquistWarningFlows) / sizeof(kNyquistWarningFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "state-idle", "Idle (display off)", kStateIdleElements, sizeof(kStateIdleElements) / sizeof(kStateIdleElements[0]), kStateIdleFlows, sizeof(kStateIdleFlows) / sizeof(kStateIdleFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c1-modbus-id", "C1 — Modbus Slave ID", kConfigC1ModbusIdElements, sizeof(kConfigC1ModbusIdElements) / sizeof(kConfigC1ModbusIdElements[0]), kConfigC1ModbusIdFlows, sizeof(kConfigC1ModbusIdFlows) / sizeof(kConfigC1ModbusIdFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c2-baud-rate", "C2 — Baud Rate", kConfigC2BaudRateElements, sizeof(kConfigC2BaudRateElements) / sizeof(kConfigC2BaudRateElements[0]), kConfigC2BaudRateFlows, sizeof(kConfigC2BaudRateFlows) / sizeof(kConfigC2BaudRateFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c3-parity", "C3 — Parity", kConfigC3ParityElements, sizeof(kConfigC3ParityElements) / sizeof(kConfigC3ParityElements[0]), kConfigC3ParityFlows, sizeof(kConfigC3ParityFlows) / sizeof(kConfigC3ParityFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c4-stop-bits", "C4 — Stop Bits", kConfigC4StopBitsElements, sizeof(kConfigC4StopBitsElements) / sizeof(kConfigC4StopBitsElements[0]), kConfigC4StopBitsFlows, sizeof(kConfigC4StopBitsFlows) / sizeof(kConfigC4StopBitsFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c5-led-pulse-vol", "C5 — LED Pulse Volume", kConfigC5LedPulseVolElements, sizeof(kConfigC5LedPulseVolElements) / sizeof(kConfigC5LedPulseVolElements[0]), kConfigC5LedPulseVolFlows, sizeof(kConfigC5LedPulseVolFlows) / sizeof(kConfigC5LedPulseVolFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c6-led-pulse-period", "C6 — LED Pulse Period", kConfigC6LedPulsePeriodElements, sizeof(kConfigC6LedPulsePeriodElements) / sizeof(kConfigC6LedPulsePeriodElements[0]), kConfigC6LedPulsePeriodFlows, sizeof(kConfigC6LedPulsePeriodFlows) / sizeof(kConfigC6LedPulsePeriodFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-c7-sensor-select", "C7 — Sensor Select", kConfigC7SensorSelectElements, sizeof(kConfigC7SensorSelectElements) / sizeof(kConfigC7SensorSelectElements[0]), kConfigC7SensorSelectFlows, sizeof(kConfigC7SensorSelectFlows) / sizeof(kConfigC7SensorSelectFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s1-connected", "S1 — Connected", kConfigS1ConnectedElements, sizeof(kConfigS1ConnectedElements) / sizeof(kConfigS1ConnectedElements[0]), kConfigS1ConnectedFlows, sizeof(kConfigS1ConnectedFlows) / sizeof(kConfigS1ConnectedFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s2-multiplier", "S2 — Multiplier (F)", kConfigS2MultiplierElements, sizeof(kConfigS2MultiplierElements) / sizeof(kConfigS2MultiplierElements[0]), kConfigS2MultiplierFlows, sizeof(kConfigS2MultiplierFlows) / sizeof(kConfigS2MultiplierFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s3-adjust", "S3 — Adjust", kConfigS3AdjustElements, sizeof(kConfigS3AdjustElements) / sizeof(kConfigS3AdjustElements[0]), kConfigS3AdjustFlows, sizeof(kConfigS3AdjustFlows) / sizeof(kConfigS3AdjustFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 },
    { "config-s4-max-flow", "S4 — Max Flow (Q, L/min)", kConfigS4MaxFlowElements, sizeof(kConfigS4MaxFlowElements) / sizeof(kConfigS4MaxFlowElements[0]), kConfigS4MaxFlowFlows, sizeof(kConfigS4MaxFlowFlows) / sizeof(kConfigS4MaxFlowFlows[0]), nullptr, 0, nullptr, 0, nullptr, 0 }
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
    "2026-05-21T21:33:58.513Z", 26, 235
};

}  // namespace ui_exporter
