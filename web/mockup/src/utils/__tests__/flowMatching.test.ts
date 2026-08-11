import fs from "node:fs";
import path from "node:path";
import { describe, expect, it } from "vitest";
import { findMatchingButtonFlows } from "../flowMatching";
import type { ScreenDefinition } from "../../types";
import screensJson from "../../data/screens.json";

const screen = (flows: unknown[]): ScreenDefinition => ({ flows } as unknown as ScreenDefinition);

const settingPage = screen([
  { id: "f-next", trigger: { type: "button", button: "down" }, actionId: "ui.action.page.next", targetScreenId: "next-setting" },
  { id: "f-prev", trigger: { type: "button", button: "up" }, actionId: "ui.action.page.previous", targetScreenId: "prev-setting" },
  { id: "f-enter", trigger: { type: "button", button: "enter" }, actionId: "ui.action.nav.descend", targetScreenId: "editor" }
]);

const editorPage = screen([
  { id: "f-inc", trigger: { type: "button", button: "up" }, actionId: "config.action.value.increment" },
  { id: "f-dec", trigger: { type: "button", button: "down" }, actionId: "config.action.value.decrement" },
  { id: "f-commit", trigger: { type: "button", button: "enter" }, actionId: "config.action.value.commit", targetScreenId: "setting" },
  { id: "f-discard", trigger: { type: "button", button: "enter", gesture: "long" }, actionId: "config.action.value.discard", targetScreenId: "setting" }
]);

const event = (button: "up" | "down" | "enter", kind: "short" | "long" | "repeat") =>
  ({ button, kind, timestamp: 0 }) as never;

describe("findMatchingButtonFlows is matchFlow, with no fallback of its own", () => {
  it("answers a repeat with hold flows only — never with the short flow", () => {
    /**
     * The bug this pins. A repeat used to fall back to the screen's SHORT flow whenever that flow named a
     * target screen, so holding UP on a setting page paged the ring every 250 ms and carried the operator
     * away from the setting they were reading. `InteractionHandler::matchFlow` has no such fallback: it
     * maps a repeat to `FlowGesture::Hold` and requires an exact match.
     */
    expect(findMatchingButtonFlows(settingPage, event("down", "repeat"))).toEqual([]);
    expect(findMatchingButtonFlows(settingPage, event("up", "repeat"))).toEqual([]);
    // And a target-less flow was never the discriminator — the GESTURE is.
    expect(findMatchingButtonFlows(editorPage, event("up", "repeat"))).toEqual([]);
  });

  it("answers a repeat when the screen really does declare a hold flow", () => {
    const withHold = screen([
      { id: "f-next", trigger: { type: "button", button: "down" }, actionId: "ui.action.page.next", targetScreenId: "b" },
      { id: "f-hold", trigger: { type: "button", button: "down", gesture: "hold" }, actionId: "ui.action.page.next", targetScreenId: "b" }
    ]);
    const matched = findMatchingButtonFlows(withHold, event("down", "repeat"));
    expect(matched.map((flow) => flow.id)).toEqual(["f-hold"]);
  });

  it("still matches short and long exactly", () => {
    expect(findMatchingButtonFlows(settingPage, event("down", "short")).map((f) => f.id)).toEqual(["f-next"]);
    expect(findMatchingButtonFlows(editorPage, event("enter", "long")).map((f) => f.id)).toEqual(["f-discard"]);
    // A long press on a screen with no long flow matches nothing rather than degrading to the short one.
    expect(findMatchingButtonFlows(settingPage, event("down", "long"))).toEqual([]);
  });

  it("records that the shipped dataset declares no hold flow at all", () => {
    /**
     * Which is why the previous test is hypothetical and why a held UP/DOWN navigates nowhere on the
     * device: §3.1's repeating navigation step is emitted by button_input.cpp and answered by nothing.
     * If this count ever moves, the expectations above stop being theoretical and the panel's button
     * legend needs re-reading — so it is asserted rather than assumed.
     */
    const gestures: Record<string, number> = {};
    for (const definition of screensJson.screens as { flows?: { trigger?: { type?: string; gesture?: string } }[] }[]) {
      for (const flow of definition.flows ?? []) {
        if (flow.trigger?.type !== "button") continue;
        const gesture = flow.trigger.gesture ?? "short";
        gestures[gesture] = (gestures[gesture] ?? 0) + 1;
      }
    }
    expect(gestures.short).toBeGreaterThan(100);
    expect(gestures.hold ?? 0).toBe(0);
  });

  it("agrees with the firmware's mapGesture, read from its source", () => {
    const source = fs.readFileSync(
      path.join(
        __dirname, "..", "..", "..", "..", "..",
        "Water-Flow-Meter-PlatformIO", "src", "input", "interaction_handler.cpp"
      ),
      "utf-8"
    );
    const mapGesture = source.slice(source.indexOf("InteractionHandler::mapGesture"));
    // !isLongPress -> Short; isRepeat -> Hold; otherwise -> Long. A change here changes this file.
    expect(/!event\.isLongPress[\s\S]*?FlowGesture::Short/.test(mapGesture)).toBe(true);
    expect(/isRepeat\s*\?\s*ui_exporter::FlowGesture::Hold\s*:\s*ui_exporter::FlowGesture::Long/.test(mapGesture)).toBe(true);

    // And matchFlow compares the gesture with equality, which is what forbids a fallback.
    const matchFlow = source.slice(source.indexOf("InteractionHandler::matchFlow"));
    expect(/flow\.gesture\s*!=\s*gesture/.test(matchFlow)).toBe(true);
  });
});
