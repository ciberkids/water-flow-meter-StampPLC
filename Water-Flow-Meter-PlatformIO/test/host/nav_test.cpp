// Host-side exercise of UiNavigator against the REAL generated screen table.
// Neither ui_navigator.cpp nor GeneratedUi.cpp depends on Arduino, so the actual
// navigation graph the device will use can be walked without hardware.
#include "ui/core/ui_navigator.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace ui;
static int failures = 0;
static void check(bool ok, const std::string& what) {
  std::printf("  %-58s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) failures++;
}
static const ui_exporter::Screen* byId(const char* id) {
  for (std::size_t i = 0; i < ui_exporter::kGeneratedScreenCount; ++i)
    if (std::strcmp(ui_exporter::kGeneratedScreens[i].id, id) == 0)
      return &ui_exporter::kGeneratedScreens[i];
  return nullptr;
}
// Resolve a screen's flow target, mirroring what InteractionHandler does.
static const ui_exporter::Screen* follow(const ui_exporter::Screen* s,
                                         ui_exporter::FlowButton btn,
                                         ui_exporter::FlowGesture g) {
  if (!s) return nullptr;
  for (std::size_t i = 0; i < s->flowCount; ++i) {
    const auto& f = s->flows[i];
    if (f.trigger != ui_exporter::FlowTrigger::Button) continue;
    if (f.button != btn || f.gesture != g) continue;
    return f.targetScreenId ? byId(f.targetScreenId) : nullptr;
  }
  return nullptr;
}
static const char* idOf(const ui_exporter::Screen* s) { return s && s->id ? s->id : "(null)"; }

int main() {
  UiNavigator nav;
  const auto* p0 = byId("info-p0-global-status");
  std::printf("screens in table: %zu\n\n", ui_exporter::kGeneratedScreenCount);

  std::printf("[info ring]\n");
  nav.reset(p0);
  check(nav.current() == p0 && nav.depth() == 0, "reset lands on P0 at depth 0");
  const char* expect[] = {"info-p1-instant-flow","info-p2-cumulative-liters",
    "info-p3-cumulative-m3","info-p4-session-liters","info-p5-session-m3",
    "info-p6-max-flow","info-p7-enter-config","info-p8-factory-reset",
    "info-p0-global-status"};
  bool ok = true;
  for (int i = 0; i < 9; ++i) {
    nav.goToSibling(follow(nav.current(), ui_exporter::FlowButton::Down,
                           ui_exporter::FlowGesture::Short));
    if (std::strcmp(idOf(nav.current()), expect[i]) != 0) {
      std::printf("      step %d: got %s want %s\n", i, idOf(nav.current()), expect[i]);
      ok = false;
    }
  }
  check(ok, "9 DOWN presses traverse P0..P8 and wrap to P0");
  check(nav.depth() == 0, "sibling moves never change depth");

  uint8_t ri = 0, rc = 0;
  check(nav.ringPosition(&ri, &rc) && rc == 9, "info ring reports 9 members");

  std::printf("\n[descend to the deepest documented path]\n");
  nav.reset(p0);
  for (int i = 0; i < 7; ++i)
    nav.goToSibling(follow(nav.current(), ui_exporter::FlowButton::Down,
                           ui_exporter::FlowGesture::Short));
  check(std::strcmp(idOf(nav.current()), "info-p7-enter-config") == 0, "reached P7");

  nav.descend(follow(nav.current(), ui_exporter::FlowButton::Enter,
                     ui_exporter::FlowGesture::Short));
  check(std::strcmp(idOf(nav.current()), "config-c1-modbus-id") == 0 && nav.depth() == 1,
        "P7 ENTER descends to config root, depth 1");
  check(nav.ringPosition(&ri, &rc) && rc == 8, "config ring reports 8 members (C1-C7 + BACK)");

  for (int i = 0; i < 6; ++i)
    nav.goToSibling(follow(nav.current(), ui_exporter::FlowButton::Down,
                           ui_exporter::FlowGesture::Short));
  check(std::strcmp(idOf(nav.current()), "config-c7-sensor-select") == 0, "paged to C7");

  nav.descend(follow(nav.current(), ui_exporter::FlowButton::Enter,
                     ui_exporter::FlowGesture::Short));
  check(std::strcmp(idOf(nav.current()), "config-sensor-1") == 0 && nav.depth() == 2,
        "C7 ENTER descends to the sensor list, depth 2");
  check(nav.sensorIndex() == 0, "sensor index still 0 before choosing a sensor");

  for (int i = 0; i < 2; ++i)
    nav.goToSibling(follow(nav.current(), ui_exporter::FlowButton::Down,
                           ui_exporter::FlowGesture::Short));
  check(std::strcmp(idOf(nav.current()), "config-sensor-3") == 0, "paged to Sensor 3");

  nav.descend(follow(nav.current(), ui_exporter::FlowButton::Enter,
                     ui_exporter::FlowGesture::Short));
  check(nav.depth() == 3, "Sensor 3 ENTER descends to sensor settings, depth 3");
  check(nav.sensorIndex() == 3, "sensor index is 3, taken from the level we came from");

  nav.descend(follow(nav.current(), ui_exporter::FlowButton::Enter,
                     ui_exporter::FlowGesture::Short));
  check(nav.depth() == 4, "S1 ENTER descends to its editor, depth 4");
  check(std::strstr(idOf(nav.current()), "-edit") != nullptr, "editor screen id ends -edit");

  std::printf("\n[ascend and escape]\n");
  check(nav.ascend() && nav.depth() == 3, "ascend from editor -> depth 3");
  check(nav.sensorIndex() == 3, "sensor index survives an ascend within the sub-tree");
  nav.escape();
  check(nav.current() == p0 && nav.depth() == 0, "escape from depth 3 lands on P0");
  check(nav.sensorIndex() == 0, "escape clears the sensor index");
  check(!nav.ascend(), "ascend at the root is a no-op, not an underflow");

  std::printf("\n[depth cap]\n");
  nav.reset(p0);
  const auto* any = byId("config-c1-modbus-id");
  for (int i = 0; i < UiNavigator::kMaxDepth; ++i) nav.descend(any);
  check(nav.depth() == UiNavigator::kMaxDepth, "fills to kMaxDepth");
  check(!nav.descend(any), "descend past kMaxDepth is refused");

  std::printf("\n%s (%d failures)\n", failures ? "FAILED" : "ALL PASSED", failures);
  return failures ? 1 : 0;
}
