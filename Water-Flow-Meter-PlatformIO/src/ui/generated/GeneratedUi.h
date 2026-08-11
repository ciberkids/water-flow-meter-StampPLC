#pragma once

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
  /**
   * Screen-level visibility: the SETTING binding that gates this screen, or nullptr when it is
   * unconditional.
   *
   * The navigator skips a screen whose gate does not hold, so a level's ring is shorter than its
   * member count when a branch is inactive. Relaxes R7.3, which is safe here because the gate is
   * itself a setting with an unguarded editor — the completeness rule becomes "reachable under some
   * value of the gate", still statically decidable by enumerating that setting's options.
   */
  const char* visibleWhenBinding;
  std::int32_t visibleWhenEquals;
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
