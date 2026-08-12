import fs from "node:fs";
import path from "node:path";
import { describe, expect, it } from "vitest";
import { movePackCursor, packCommitAction, packSelectorLayout } from "../packSelector";

const firmwareSource = (...parts: string[]) =>
  fs.readFileSync(
    path.join(__dirname, "..", "..", "..", "..", "..", "Water-Flow-Meter-PlatformIO", "src", ...parts),
    "utf-8"
  );

describe("the simulated Select Menu matches drawPackSelector", () => {
  /**
   * The Select Menu is the one page the dataset cannot describe — the firmware draws it before every
   * table-driven path — so its geometry is duplicated across a language boundary with nothing but care
   * holding the two together. That is the same situation as the acceleration tiers and the option-list
   * step semantics, and it gets the same treatment: read the numbers out of the C++ rather than trust
   * the copy.
   */
  it("uses the coordinates ui_renderer.cpp draws at", () => {
    const source = firmwareSource("ui", "core", "ui_renderer.cpp");
    const body = source.slice(source.indexOf("void UiRenderer::drawPackSelector"));

    const title = /drawString\("SELECT MENU",\s*(\d+),\s*(\d+)\)/.exec(body);
    expect(title, "the title must be drawn with a literal position").not.toBeNull();
    expect({ x: Number(title![1]), y: Number(title![2]) }).toEqual(packSelectorLayout.title);

    const row = /const int32_t y = static_cast<int32_t>\((\d+) \+ i \* (\d+)\)/.exec(body);
    expect(row, "rows must be at a literal origin and pitch").not.toBeNull();
    expect({ top: Number(row![1]), pitch: Number(row![2]) }).toEqual(packSelectorLayout.rows);

    // The cursor glyph and the label are two separate drawString calls at two x positions.
    const cursorX = /drawString\(onCursor \? ">" : " ",\s*(\d+),/.exec(body);
    const labelX = /drawString\(selector->labelAt\(i\),\s*(\d+),/.exec(body);
    expect(Number(cursorX![1])).toBe(packSelectorLayout.cursorX);
    expect(Number(labelX![1])).toBe(packSelectorLayout.labelX);

    const markerX = /drawString\("\*",\s*(\d+),/.exec(body);
    expect(Number(markerX![1])).toBe(packSelectorLayout.activeMarkerX);

    const footer = /drawString\("UP\/DN choose  ENTER select",\s*(\d+),\s*(\d+)\)/.exec(body);
    expect(footer, "the footer must be drawn with a literal position").not.toBeNull();
    expect({ x: Number(footer![1]), y: Number(footer![2]) }).toEqual(packSelectorLayout.footer);
  });

  it("uses the firmware's exact footer strings", () => {
    const source = firmwareSource("ui", "core", "ui_renderer.cpp");
    const body = source.slice(source.indexOf("void UiRenderer::drawPackSelector"));
    expect(body).toContain(`"${packSelectorLayout.footerText}"`);
    expect(body).toContain(`"${packSelectorLayout.truncatedText}"`);
    expect(body).toContain(`"${packSelectorLayout.unavailableText}"`);
  });

  it("keeps every row inside the panel", () => {
    // Eight entries is PackSelector::kMaxEntries, chosen against the display rather than the
    // filesystem. The last row plus a glyph must still clear the footer, or the page the device falls
    // back on when everything else is broken would overwrite its own instructions.
    const lastRowTop = packSelectorLayout.rows.top + 7 * packSelectorLayout.rows.pitch;
    expect(lastRowTop + 8).toBeLessThanOrEqual(packSelectorLayout.footer.y);
    expect(packSelectorLayout.footer.y).toBeLessThan(135);
    expect(packSelectorLayout.activeMarkerX).toBeLessThan(240);
  });

  it("reads kMaxEntries from the firmware and agrees with it", () => {
    const selector = firmwareSource("ui", "pack", "ui_pack_selector.h");
    const max = /kMaxEntries\s*=\s*(\d+)/.exec(selector);
    expect(max).not.toBeNull();
    expect(Number(max![1])).toBe(packSelectorLayout.maxEntries);
  });
});

describe("the cursor wraps, which is not what it looks like it should do", () => {
  it("wraps at both ends", () => {
    /**
     * I guessed clamp and was wrong. `PackSelector::moveCursor` does modular arithmetic, so DOWN from
     * the last entry lands on the built-in default. Pinned because the guess is the natural one — a
     * three-entry list looks like somewhere clamping would be kinder — and a simulator that clamped
     * would disagree with the device at exactly the moment an operator is trying to reach a working UI.
     */
    expect(movePackCursor(0, 1, 3)).toBe(1);
    expect(movePackCursor(1, -1, 3)).toBe(0);
    expect(movePackCursor(2, 1, 3)).toBe(0);
    expect(movePackCursor(0, -1, 3)).toBe(2);
  });

  it("survives an empty list without going negative", () => {
    expect(movePackCursor(0, 1, 0)).toBe(0);
    expect(movePackCursor(0, -1, 0)).toBe(0);
  });

  it("wraps the way the firmware's own arithmetic does, read from its source", () => {
    const source = firmwareSource("ui", "pack", "ui_pack_selector.cpp");
    const body = source.slice(source.indexOf("void PackSelector::moveCursor"));
    // The three lines that make it a wrap rather than a clamp.
    expect(body).toMatch(/delta % span/);
    expect(body).toMatch(/if \(next < 0\) next \+= span;/);
    expect(body).toMatch(/if \(next >= span\) next -= span;/);
  });
});

describe("committing the cursor", () => {
  it("does nothing when the chosen pack is already running", () => {
    // `commitAction`: writing the same pointer and rebooting into an identical UI "would look like the
    // device ignoring the press".
    expect(packCommitAction(0, 0)).toBe("nothing");
    expect(packCommitAction(2, 2)).toBe("nothing");
  });

  it("deletes the pointer for the built-in default and writes one otherwise", () => {
    expect(packCommitAction(0, 2)).toBe("delete-pointer");
    expect(packCommitAction(1, 0)).toBe("write-pointer");
  });

  it("agrees with commitAction, read from the firmware", () => {
    const source = firmwareSource("ui", "pack", "ui_pack_selector.cpp");
    const body = source.slice(source.indexOf("PackSelector::Commit PackSelector::commitAction"));
    expect(body).toMatch(/cursor_ == activeIndex_[\s\S]*Commit::Nothing/);
    expect(body).toMatch(/cursor_ == kBuiltInIndex \? Commit::DeletePointer : Commit::WritePointer/);
  });
});
