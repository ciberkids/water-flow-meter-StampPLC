#pragma once

#include <cstddef>
#include <cstdint>

#include "ui/generated/GeneratedUi.h"

namespace ui {

/**
 * The "Select Menu" page the FIRMWARE appends to the end of the root level
 * (`docs/Requirements/feature addition/Loadable_UI_Menu_Packs.md` §3.4).
 *
 * Four things about where this lives, each of them a decision rather than an accident:
 *
 * (1) IT IS A REAL `ui_exporter::Screen`, so no new drawing or dispatch path exists for it.
 *     `UiNavigator::current_` is a bare `const Screen*` — nothing requires it to point INTO a screen
 *     table — and `UiRenderer::update` paints `context.currentScreen` directly, while
 *     `InteractionHandler::matchFlow` iterates `screen->flows`. So the ordinary table-driven render
 *     and flow-match paths handle this screen unchanged. That is strictly less machinery than the
 *     `drawPackSelector` short-circuit, which had to be a short-circuit because the SELECTOR is a live
 *     list of what is on the card and cannot be a constant. This page is the ENTRY, not the list.
 *
 * (2) IT IS DELIBERATELY NOT IN `kRequiredScreens` (ui/core/ui_pages.h). Everything listed there is a
 *     screen a PACK must supply: `web/mockup/tools/exporter/validation.ts` fails an export whose
 *     dataset omits any of them. Listing this id would therefore force it into every pack — and a pack
 *     that then dropped the row would take the only discoverable route to pack selection with it, which
 *     is the exact trap §3.4 exists to prevent.
 *
 * (3) IT IS DELIBERATELY NOT IN `src/ui/generated/`. The exporter regenerates that directory from
 *     `web/mockup/src/data/screens.json` and the CI export gate fails on any difference, so an entry
 *     added there would be erased — and, being a dataset row, would be shadowable by a pack.
 *
 * (4) THE FLOW TARGETS ARE ASYMMETRIC ON PURPOSE. `f-next` declares the literal root id, which is safe
 *     because `ui_pages.h`'s `kRequiredScreens` makes that id a contract every pack must satisfy.
 *     `f-prev` and `f-enter` declare no target: UP's destination is COMPUTED (the preceding root
 *     member, whatever the active pack's root ring turns out to be), and ENTER's destination is not a
 *     screen at all — it is the firmware-drawn selector.
 *
 * `inline constexpr` throughout so every translation unit shares one address; `UiNavigator` compares
 * pointers against `kSelectMenuScreen` to recognise the tail.
 *
 * The layout is copied in shape from `net-mqtt-root`, the §7 root-page template: title at (2,2),
 * `nav.position` at (168,2), two body lines at y=28 and y=40, a footer hint at y=124 and the level
 * scrollbar at x=232. Widths are inside the 40-column / 6 px-glyph row budget. Note that the footer
 * hint is NOT always visible: the warning banner paints at y=116 over the footer row, AFTER the
 * screen, so a live warning covers it — the same trade `Display_Per_Screen_Spec` §2c accepted for
 * every other screen that carries a hint there.
 *
 * The mockup cannot preview this page, for the same reason it cannot preview the selector: it reads
 * `screens.json`, and this screen is not in it. So are the geometry and spec audits — the host test
 * `rootEntryTests` in `test/host/interaction_test.cpp` is the only gate this screen's geometry has.
 */
inline constexpr const char* kSelectMenuScreenId = "ui-select-menu";

inline constexpr ui_exporter::TextPayload kSelectMenuTitle{
    "SELECT MENU", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Strong};
/** Empty on purpose: the `nav.position` binding supplies the text. */
inline constexpr ui_exporter::TextPayload kSelectMenuNavPos{
    "", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted};
inline constexpr ui_exporter::TextPayload kSelectMenuLine1{
    "Choose which menu the display", ui_exporter::TextAlign::Left,
    ui_exporter::TextEmphasis::Normal};
inline constexpr ui_exporter::TextPayload kSelectMenuLine2{
    "runs. Built-in is always here.", ui_exporter::TextAlign::Left,
    ui_exporter::TextEmphasis::Normal};
inline constexpr ui_exporter::TextPayload kSelectMenuFooter{
    "UP/DN pages  ENTER open", ui_exporter::TextAlign::Left, ui_exporter::TextEmphasis::Muted};

inline constexpr ui_exporter::Element kSelectMenuElements[] = {
    {"hdr-title", ui_exporter::ElementType::Text, 2, 2, 0, 0, &kSelectMenuTitle, nullptr, nullptr},
    {"nav-position", ui_exporter::ElementType::Text, 168, 2, 0, 0, &kSelectMenuNavPos, nullptr,
     "nav.position"},
    {"line-1", ui_exporter::ElementType::Text, 2, 28, 0, 0, &kSelectMenuLine1, nullptr, nullptr},
    {"line-2", ui_exporter::ElementType::Text, 2, 40, 0, 0, &kSelectMenuLine2, nullptr, nullptr},
    {"footer-hint", ui_exporter::ElementType::Text, 2, 124, 0, 0, &kSelectMenuFooter, nullptr,
     nullptr},
    {"level-position", ui_exporter::ElementType::Scrollbar, 232, 14, 5, 100, nullptr, nullptr,
     nullptr}};

inline constexpr std::size_t kSelectMenuElementCount =
    sizeof(kSelectMenuElements) / sizeof(kSelectMenuElements[0]);

/**
 * Three flows, matching what `net-wifi-root` and `net-mqtt-root` declare: UP/DOWN short and ENTER
 * short. No ENTER-long — neither of those root pages declares one either.
 */
inline constexpr ui_exporter::Flow kSelectMenuFlows[] = {
    {"f-next", "Next page", "info-p0-global-status", ui_exporter::FlowTrigger::Button,
     ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr,
     "ui.action.page.next", nullptr, 0},
    {"f-prev", "Previous page", nullptr, ui_exporter::FlowTrigger::Button,
     ui_exporter::FlowButton::Up, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr,
     "ui.action.page.previous", nullptr, 0},
    {"f-enter", "Open the Select Menu", nullptr, ui_exporter::FlowTrigger::Button,
     ui_exporter::FlowButton::Enter, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr,
     "ui.action.pack.select-menu", nullptr, 0}};

inline constexpr ui_exporter::Screen kSelectMenuScreen{kSelectMenuScreenId,
                                                       "SELECT MENU",
                                                       kSelectMenuElements,
                                                       sizeof(kSelectMenuElements) /
                                                           sizeof(kSelectMenuElements[0]),
                                                       kSelectMenuFlows,
                                                       sizeof(kSelectMenuFlows) /
                                                           sizeof(kSelectMenuFlows[0]),
                                                       // assets, submenus, visibleWhen{Binding,Equals}.
                                                       // ONE PAIR SHORTER SINCE J2: the generated `Screen`
                                                       // no longer carries `animations` / `animationCount`,
                                                       // because decision C1 dropped the feature and nothing
                                                       // in the firmware ever read the emitted array. A
                                                       // POSITIONAL initializer over a generated struct
                                                       // breaks on any field change — this is the file that
                                                       // caught it, via `too many initializers`.
                                                       nullptr,
                                                       0,
                                                       nullptr,
                                                       0,
                                                       nullptr,
                                                       0};

}  // namespace ui
