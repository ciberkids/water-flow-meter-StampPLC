import { ExportIR, IRScreenElement, IRElementKind, IRFlow } from "./types.js";

const headerGuard = `#pragma once

#include <cstddef>
#include <cstdint>

namespace ui_exporter {

enum class ElementType : std::uint8_t {
  Text = 0,
  Value = 1,
  Badge = 2,
  Box = 3,
  Icon = 4,
  Scrollbar = 5
};

enum class TextAlign : std::uint8_t {
  Left = 0,
  Center = 1,
  Right = 2
};

enum class TextEmphasis : std::uint8_t {
  Normal = 0,
  Strong = 1,
  Muted = 2
};

enum class FlowTrigger : std::uint8_t {
  Button = 0,
  Timeout = 1,
  Data = 2
};

enum class FlowButton : std::uint8_t {
  None = 0,
  Up = 1,
  Down = 2,
  Enter = 3
};

enum class FlowGesture : std::uint8_t {
  Short = 0,
  Long = 1,
  Hold = 2
};

enum class AnimationKind : std::uint8_t {
  FrameSequence = 0,
  Property = 1
};

struct KeyValue {
  const char* key;
  const char* value;
};

struct TextPayload {
  const char* text;
  TextAlign align;
  TextEmphasis emphasis;
};

struct Element {
  const char* id;
  ElementType type;
  std::int16_t x;
  std::int16_t y;
  std::int16_t width;
  std::int16_t height;
  const TextPayload* text;
  const char* assetId;
  const char* bindingId;
};

struct Flow {
  const char* id;
  const char* label;
  const char* targetScreenId;
  FlowTrigger trigger;
  FlowButton button;
  FlowGesture gesture;
  std::uint32_t timeoutMs;
  const char* dataSource;
  const char* dataCondition;
  const char* guard;
  const char* actionId;
  const KeyValue* actionParams;
  std::size_t actionParamCount;
};

struct AnimationKeyframe {
  float at;
  const KeyValue* state;
  std::size_t stateCount;
  std::int32_t assetFrameIndex;
};

struct Animation {
  const char* id;
  const char* targetElementId;
  AnimationKind kind;
  bool loop;
  const char* easing;
  const AnimationKeyframe* frames;
  std::size_t frameCount;
};

struct GraphicAsset {
  const char* id;
  const char* type;
  const char* source;
  const char* const* frames;
  std::size_t frameCount;
  std::int32_t fps;
  const char* const* palette;
  std::size_t paletteCount;
};

struct Submenu {
  const char* id;
  const char* label;
  const char* screenId;
  const char* iconAssetId;
};

struct Screen {
  const char* id;
  const char* name;
  const Element* elements;
  std::size_t elementCount;
  const Flow* flows;
  std::size_t flowCount;
  const GraphicAsset* assets;
  std::size_t assetCount;
  const Animation* animations;
  std::size_t animationCount;
  const Submenu* submenus;
  std::size_t submenuCount;
};

struct ThemeColor {
  const char* key;
  std::uint32_t argb8888;
};

struct Theme {
  const char* name;
  const ThemeColor* colors;
  std::size_t colorCount;
  std::uint16_t typographyBase;
  std::uint16_t typographyValue;
  std::uint16_t typographyBadge;
  const char* animationEasing;
};

struct Metadata {
  const char* generatedAtIso8601;
  std::size_t screenCount;
  std::size_t elementCount;
};

extern const Screen kGeneratedScreens[];
extern const std::size_t kGeneratedScreenCount;
extern const Theme kGeneratedTheme;
extern const Metadata kGeneratedMetadata;

}  // namespace ui_exporter
`;

function escapeStringLiteral(value: string): string {
  return value
    .replace(/\\/g, "\\\\")
    .replace(/"/g, '\\"')
    .replace(/\n/g, "\\n");
}

function formatUint32Hex(value: number): string {
  const hex = value.toString(16).padStart(8, "0").toUpperCase();
  return `0x${hex}u`;
}

function sanitiseIdentifier(...parts: string[]): string {
  const raw = parts.join("_");
  const cleaned = raw
    .replace(/[^A-Za-z0-9_]+/g, "_")
    .replace(/_{2,}/g, "_")
    .replace(/^_+/, "")
    .replace(/_+$/, "")
    .toLowerCase();
  return cleaned || "unnamed";
}

function elementTypeLiteral(kind: IRElementKind): string {
  switch (kind.type) {
    case "text":
      return "ui_exporter::ElementType::Text";
    case "value":
      return "ui_exporter::ElementType::Value";
    case "badge":
      return "ui_exporter::ElementType::Badge";
    case "box":
      return "ui_exporter::ElementType::Box";
    case "icon":
      return "ui_exporter::ElementType::Icon";
    case "scrollbar":
      return "ui_exporter::ElementType::Scrollbar";
    default:
      return "ui_exporter::ElementType::Text";
  }
}

function alignLiteral(kind: IRElementKind): string {
  switch (kind.type) {
    case "text":
    case "value":
    case "badge":
      switch (kind.payload.align) {
        case "center":
          return "ui_exporter::TextAlign::Center";
        case "right":
          return "ui_exporter::TextAlign::Right";
        default:
          return "ui_exporter::TextAlign::Left";
      }
    default:
      return "ui_exporter::TextAlign::Left";
  }
}

function emphasisLiteral(kind: IRElementKind): string {
  switch (kind.type) {
    case "text":
    case "value":
    case "badge":
      switch (kind.payload.emphasis) {
        case "strong":
          return "ui_exporter::TextEmphasis::Strong";
        case "muted":
          return "ui_exporter::TextEmphasis::Muted";
        default:
          return "ui_exporter::TextEmphasis::Normal";
      }
    default:
      return "ui_exporter::TextEmphasis::Normal";
  }
}

function flowTriggerLiteral(trigger: IRFlow["trigger"] | undefined): string {
  if (!trigger) {
    return "ui_exporter::FlowTrigger::Button";
  }
  switch (trigger.type) {
    case "timeout":
      return "ui_exporter::FlowTrigger::Timeout";
    case "data":
      return "ui_exporter::FlowTrigger::Data";
    case "button":
    default:
      return "ui_exporter::FlowTrigger::Button";
  }
}

function flowButtonLiteral(trigger: IRFlow["trigger"] | undefined): string {
  if (!trigger || trigger.type !== "button") {
    return "ui_exporter::FlowButton::None";
  }
  switch (trigger.button) {
    case "up":
      return "ui_exporter::FlowButton::Up";
    case "down":
      return "ui_exporter::FlowButton::Down";
    case "enter":
    default:
      return "ui_exporter::FlowButton::Enter";
  }
}

function flowGestureLiteral(trigger: IRFlow["trigger"] | undefined): string {
  if (!trigger || trigger.type !== "button") {
    return "ui_exporter::FlowGesture::Short";
  }
  switch (trigger.gesture) {
    case "long":
      return "ui_exporter::FlowGesture::Long";
    case "hold":
      return "ui_exporter::FlowGesture::Hold";
    case "short":
    case undefined:
    default:
      return "ui_exporter::FlowGesture::Short";
  }
}

function flowTimeoutLiteral(trigger: IRFlow["trigger"] | undefined): string {
  if (!trigger || trigger.type !== "timeout") {
    return "0";
  }
  return `${Math.max(0, Math.floor(trigger.durationMs ?? 0))}`;
}

function flowDataLiteral(
  trigger: IRFlow["trigger"] | undefined
): { source: string; condition: string } {
  if (!trigger || trigger.type !== "data") {
    return { source: "nullptr", condition: "nullptr" };
  }
  const sourceLiteral = trigger.source
    ? `"${escapeStringLiteral(trigger.source)}"`
    : "nullptr";
  const conditionLiteral = trigger.condition
    ? `"${escapeStringLiteral(trigger.condition)}"`
    : "nullptr";
  return { source: sourceLiteral, condition: conditionLiteral };
}

function animationKindLiteral(kind: string | undefined): string {
  switch (kind) {
    case "frame-sequence":
      return "ui_exporter::AnimationKind::FrameSequence";
    case "property":
    default:
      return "ui_exporter::AnimationKind::Property";
  }
}

function stringLiteralForStateValue(value: number | string | boolean): string {
  if (typeof value === "boolean") {
    return value ? "\"true\"" : "\"false\"";
  }
  if (typeof value === "number") {
    return `"${value}"`;
  }
  return `"${escapeStringLiteral(value)}"`;
}

function formatFloat(value: number): string {
  if (Number.isInteger(value)) {
    return `${value}.0f`;
  }
  const asString = Number(value).toString();
  return `${asString.endsWith(".") ? `${asString}0` : asString}f`;
}

interface EmitterOutputs {
  header: string;
  source: string;
  metadataJson: string;
}

export function emitCpp(ir: ExportIR): EmitterOutputs {
  const header = headerGuard;
  const sourceLines: string[] = [];
  sourceLines.push('#include "GeneratedUi.h"');
  sourceLines.push("");
  sourceLines.push("namespace ui_exporter {");
  sourceLines.push("");

  const screenRefs: Array<{
    elementArray: string;
    elementCount: string;
    flowArray: string;
    flowCount: string;
    assetArray: string;
    assetCount: string;
    animationArray: string;
    animationCount: string;
    submenuArray: string;
    submenuCount: string;
  }> = [];

  ir.dataset.forEach((screen, screenIndex) => {
    const screenIdSlug = sanitiseIdentifier(screen.id || `screen_${screenIndex}`);
    const screenLabel = camelCase(screenIdSlug);
    const textPayloadLines: string[] = [];
    const elementLines: string[] = [];

    screen.elements.forEach((element, elementIndex) => {
      const elementSlug = sanitiseIdentifier(screen.id, element.id || `element_${elementIndex}`);
      const typeLiteral = elementTypeLiteral(element.kind);

      let textPointer = "nullptr";
      if (element.kind.type === "text" || element.kind.type === "value" || element.kind.type === "badge") {
        const payloadName = `k${screenLabel}_${camelCase(elementSlug)}_Text`;
        const payload = element.kind.payload;
        textPayloadLines.push(
          `static constexpr ui_exporter::TextPayload ${payloadName} = { "${escapeStringLiteral(payload.text)}", ${alignLiteral(element.kind)}, ${emphasisLiteral(element.kind)} };`
        );
        textPointer = `&${payloadName}`;
      }

      const width = element.size?.width ?? (element.kind.type === "box" ? element.kind.payload.width : 0);
      const height = element.size?.height ?? (element.kind.type === "box" ? element.kind.payload.height : 0);
      const assetId =
        element.kind.type === "icon" && element.kind.payload.assetId
          ? `"${escapeStringLiteral(element.kind.payload.assetId)}"`
          : "nullptr";
      const bindingLiteral = element.binding
        ? `"${escapeStringLiteral(element.binding)}"`
        : "nullptr";

      elementLines.push(
        `{ "${escapeStringLiteral(element.id)}", ${typeLiteral}, ${element.position.x}, ${element.position.y}, ${width}, ${height}, ${textPointer}, ${assetId}, ${bindingLiteral} }`
      );
    });

    const elementArrayName = `k${screenLabel}Elements`;
    if (textPayloadLines.length > 0) {
      sourceLines.push(...textPayloadLines);
      sourceLines.push("");
    }
    sourceLines.push(`static constexpr ui_exporter::Element ${elementArrayName}[] = {`);
    sourceLines.push(elementLines.map((line) => `    ${line}`).join(",\n"));
    sourceLines.push("};");
    sourceLines.push("");

    let flowArrayName = "nullptr";
    let flowCountExpr = "0";
    if (screen.flows.length > 0) {
      flowArrayName = `k${screenLabel}Flows`;
      const flowLines: string[] = [];
      screen.flows.forEach((flow, flowIndex) => {
        const guardLiteral = flow.guard ? `"${escapeStringLiteral(flow.guard)}"` : "nullptr";
        const targetLiteral = flow.targetScreenId
          ? `"${escapeStringLiteral(flow.targetScreenId)}"`
          : "nullptr";
        const actionLiteral = flow.actionId ? `"${escapeStringLiteral(flow.actionId)}"` : "nullptr";
        let actionParamsName = "nullptr";
        let actionParamsCount = "0";
        const actionParamEntries = flow.actionParams ? Object.entries(flow.actionParams) : [];
        if (actionParamEntries.length > 0) {
          actionParamsName = `k${screenLabel}_Flow${flowIndex}ActionParams`;
          const kvLines = actionParamEntries.map(
            ([key, value]) =>
              `    { "${escapeStringLiteral(key)}", ${stringLiteralForStateValue(value)} }`
          );
          sourceLines.push("");
          sourceLines.push(`static constexpr ui_exporter::KeyValue ${actionParamsName}[] = {`);
          sourceLines.push(kvLines.join(",\n"));
          sourceLines.push("};");
          actionParamsCount = `sizeof(${actionParamsName}) / sizeof(${actionParamsName}[0])`;
        }
        const timeoutLiteral = flowTimeoutLiteral(flow.trigger);
        const { source: dataSourceLiteral, condition: dataConditionLiteral } = flowDataLiteral(
          flow.trigger
        );
        flowLines.push(
          `    { "${escapeStringLiteral(flow.id)}", "${escapeStringLiteral(flow.label)}", ${targetLiteral}, ${flowTriggerLiteral(flow.trigger)}, ${flowButtonLiteral(flow.trigger)}, ${flowGestureLiteral(flow.trigger)}, ${timeoutLiteral}, ${dataSourceLiteral}, ${dataConditionLiteral}, ${guardLiteral}, ${actionLiteral}, ${actionParamsName}, ${actionParamsCount} }`
        );
      });
      sourceLines.push("");
      sourceLines.push(`static constexpr ui_exporter::Flow ${flowArrayName}[] = {`);
      sourceLines.push(flowLines.join(",\n"));
      sourceLines.push("};");
      flowCountExpr = `sizeof(${flowArrayName}) / sizeof(${flowArrayName}[0])`;
    }

    let submenuArrayName = "nullptr";
    let submenuCountExpr = "0";
    if (screen.submenus.length > 0) {
      submenuArrayName = `k${screenLabel}Submenus`;
      const submenuLines = screen.submenus.map((submenu, submenuIndex) => {
        const iconLiteral = submenu.iconAssetId ? `"${escapeStringLiteral(submenu.iconAssetId)}"` : "nullptr";
        return `    { "${escapeStringLiteral(submenu.id)}", "${escapeStringLiteral(submenu.label)}", "${escapeStringLiteral(submenu.screenId)}", ${iconLiteral} }`;
      });
      sourceLines.push("");
      sourceLines.push(`static constexpr ui_exporter::Submenu ${submenuArrayName}[] = {`);
      sourceLines.push(submenuLines.join(",\n"));
      sourceLines.push("};");
      submenuCountExpr = `sizeof(${submenuArrayName}) / sizeof(${submenuArrayName}[0])`;
    }

    let assetArrayName = "nullptr";
    let assetCountExpr = "0";
    if (screen.assets.length > 0) {
      const assetLines: string[] = [];
      assetArrayName = `k${screenLabel}Assets`;
      screen.assets.forEach((asset, assetIndex) => {
        const assetSlug = sanitiseIdentifier(screen.id, asset.id || `asset_${assetIndex}`);
        let framesName = "nullptr";
        let framesCount = "0";
        if (asset.frames && asset.frames.length > 0) {
          framesName = `k${screenLabel}_${camelCase(assetSlug)}Frames`;
          const frameLines = asset.frames.map((frame) => `    "${escapeStringLiteral(frame)}"`);
          sourceLines.push("");
          sourceLines.push(`static constexpr const char* ${framesName}[] = {`);
          sourceLines.push(frameLines.join(",\n"));
          sourceLines.push("};");
          framesCount = `sizeof(${framesName}) / sizeof(${framesName}[0])`;
        }

        let paletteName = "nullptr";
        let paletteCount = "0";
        if (asset.palette && asset.palette.length > 0) {
          paletteName = `k${screenLabel}_${camelCase(assetSlug)}Palette`;
          const paletteLines = asset.palette.map((entry) => `    "${escapeStringLiteral(entry)}"`);
          sourceLines.push("");
          sourceLines.push(`static constexpr const char* ${paletteName}[] = {`);
          sourceLines.push(paletteLines.join(",\n"));
          sourceLines.push("};");
          paletteCount = `sizeof(${paletteName}) / sizeof(${paletteName}[0])`;
        }

        const fps = asset.fps ?? -1;
        assetLines.push(
          `    { "${escapeStringLiteral(asset.id)}", "${escapeStringLiteral(asset.type)}", "${escapeStringLiteral(asset.source)}", ${framesName}, ${framesCount}, ${fps}, ${paletteName}, ${paletteCount} }`
        );
      });
      sourceLines.push("");
      sourceLines.push(`static constexpr ui_exporter::GraphicAsset ${assetArrayName}[] = {`);
      sourceLines.push(assetLines.join(",\n"));
      sourceLines.push("};");
      assetCountExpr = `sizeof(${assetArrayName}) / sizeof(${assetArrayName}[0])`;
    }

    let animationArrayName = "nullptr";
    let animationCountExpr = "0";
    if (screen.animations.length > 0) {
      const animationLines: string[] = [];
      animationArrayName = `k${screenLabel}Animations`;
      screen.animations.forEach((animation, animationIndex) => {
        const animationSlug = sanitiseIdentifier(screen.id, animation.id || `animation_${animationIndex}`);
        const frameArrayName = `k${screenLabel}_${camelCase(animationSlug)}Frames`;
        const frameLines: string[] = [];

        animation.frames.forEach((frame, frameIndex) => {
          const stateEntries = Object.entries(frame.state ?? {});
          let stateArrayName = "nullptr";
          let stateCountExpr = "0";
          if (stateEntries.length > 0) {
            stateArrayName = `k${screenLabel}_${camelCase(animationSlug)}_Frame${frameIndex}State`;
            const kvLines = stateEntries.map(
              ([key, value]) => `    { "${escapeStringLiteral(key)}", ${stringLiteralForStateValue(value)} }`
            );
            sourceLines.push("");
            sourceLines.push(`static constexpr ui_exporter::KeyValue ${stateArrayName}[] = {`);
            sourceLines.push(kvLines.join(",\n"));
            sourceLines.push("};");
            stateCountExpr = `sizeof(${stateArrayName}) / sizeof(${stateArrayName}[0])`;
          }
          const assetFrameIndex = frame.assetFrameIndex ?? -1;
          frameLines.push(
            `    { ${formatFloat(frame.at)}, ${stateArrayName}, ${stateCountExpr}, ${assetFrameIndex} }`
          );
        });

        sourceLines.push("");
        sourceLines.push(`static constexpr ui_exporter::AnimationKeyframe ${frameArrayName}[] = {`);
        sourceLines.push(frameLines.join(",\n"));
        sourceLines.push("};");

        const easingLiteral = animation.easing ? `"${escapeStringLiteral(animation.easing)}"` : "nullptr";
        const loopLiteral = animation.loop ? "true" : "false";
        animationLines.push(
          `    { "${escapeStringLiteral(animation.id)}", "${escapeStringLiteral(animation.targetElementId)}", ${animationKindLiteral(animation.kind)}, ${loopLiteral}, ${easingLiteral}, ${frameArrayName}, sizeof(${frameArrayName}) / sizeof(${frameArrayName}[0]) }`
        );
      });

      sourceLines.push("");
      sourceLines.push(`static constexpr ui_exporter::Animation ${animationArrayName}[] = {`);
      sourceLines.push(animationLines.join(",\n"));
      sourceLines.push("};");
      animationCountExpr = `sizeof(${animationArrayName}) / sizeof(${animationArrayName}[0])`;
    }

    sourceLines.push("");

    screenRefs.push({
      elementArray: elementArrayName,
      elementCount: `sizeof(${elementArrayName}) / sizeof(${elementArrayName}[0])`,
      flowArray: flowArrayName,
      flowCount: flowCountExpr,
      assetArray: assetArrayName,
      assetCount: assetCountExpr,
      animationArray: animationArrayName,
      animationCount: animationCountExpr,
      submenuArray: submenuArrayName,
      submenuCount: submenuCountExpr
    });
  });

  sourceLines.push("const ui_exporter::Screen kGeneratedScreens[] = {");
  ir.dataset.forEach((screen, index) => {
    const refs = screenRefs[index];
    const parts = [
      `"${escapeStringLiteral(screen.id)}"`,
      `"${escapeStringLiteral(screen.name)}"`,
      refs.elementArray,
      refs.elementCount,
      refs.flowArray,
      refs.flowCount,
      refs.assetArray,
      refs.assetCount,
      refs.animationArray,
      refs.animationCount,
      refs.submenuArray,
      refs.submenuCount
    ];
    sourceLines.push(
      `    { ${parts.join(", ")} }${index === ir.dataset.length - 1 ? "" : ","}`
    );
  });
  sourceLines.push("};");
  sourceLines.push("");
  sourceLines.push("static constexpr ui_exporter::ThemeColor kThemeColors[] = {");
  const themeEntries = Object.entries(ir.theme.colors);
  themeEntries.forEach(([key, value], index) => {
    sourceLines.push(
      `    { "${escapeStringLiteral(key)}", ${formatUint32Hex(value.argb8888)} }${index === themeEntries.length - 1 ? "" : ","}`
    );
  });
  sourceLines.push("};");
  sourceLines.push("");
  sourceLines.push(
    "const std::size_t kGeneratedScreenCount = sizeof(kGeneratedScreens) / sizeof(kGeneratedScreens[0]);"
  );
  sourceLines.push("");
  sourceLines.push("const ui_exporter::Theme kGeneratedTheme = {");
  sourceLines.push(
    `    "${escapeStringLiteral(ir.theme.name)}", kThemeColors, sizeof(kThemeColors) / sizeof(kThemeColors[0]), ${ir.theme.typography.base}, ${ir.theme.typography.value}, ${ir.theme.typography.badge}, "${escapeStringLiteral(ir.theme.animation.easing)}"`
  );
  sourceLines.push("};");
  sourceLines.push("");
  sourceLines.push("const ui_exporter::Metadata kGeneratedMetadata = {");
  sourceLines.push(
    `    "${escapeStringLiteral(ir.generatedAt)}", ${ir.screenCount}, ${ir.elementCount}`
  );
  sourceLines.push("};");
  sourceLines.push("");
  sourceLines.push("}  // namespace ui_exporter");
  sourceLines.push("");

  const metadataJson = JSON.stringify(
    {
      generatedAt: ir.generatedAt,
      screenCount: ir.screenCount,
      elementCount: ir.elementCount
    },
    null,
    2
  );

  return {
    header,
    source: sourceLines.join("\n"),
    metadataJson
  };
}

function camelCase(value: string): string {
  return value
    .split(/[_\W]+/)
    .filter(Boolean)
    .map((segment) => segment[0].toUpperCase() + segment.slice(1))
    .join("");
}
