import { describe, expect, it } from "vitest";
import {
  createSensorTable,
  kSensorCount,
  setSensor,
  uncalibratedSensorNumbers,
  warningSensorNumbers,
  warningSummaryText
} from "../sensorConfig";
import {
  warningBannerLayout,
  warningBannerRightEdge,
  warningBannerText,
  warningBannerVisible
} from "../warningBanner";

/** A channel in use with no calibration — the `SET?` row the commissioning gap counts. */
const uncalibrate = (table: ReturnType<typeof createSensorTable>, number: number) =>
  // DF16: `ready` is derived from the calibration now, so an uncalibrated channel is made by clearing
  // q_max — the field the device's own predicate checks first — not by clearing a bit.
  setSensor(table, number, { qMaxLpm: 0 });

/** Every channel connected, ready, and flagged: the sampling-only branch at k=8. */
const allFlagged = () => {
  let table = createSensorTable();
  for (let number = 1; number <= kSensorCount; number += 1) {
    table = setSensor(table, number, { undersampling: true });
  }
  return table;
};

/** Nothing wired at all — the state a device ships in (connected bitmap defaults to 0 in NVS). */
const nothingConnected = () => {
  let table = createSensorTable();
  for (let number = 1; number <= kSensorCount; number += 1) {
    table = setSensor(table, number, { connected: false });
  }
  return table;
};

describe("the banner's geometry is the firmware's", () => {
  it("pins the band, the two draw positions, and the two colours", () => {
    // `bannerY = 116`, `bannerH = 18`, so the band is y 116..133 and 134 is the first row below it.
    expect(warningBannerLayout.y).toBe(116);
    expect(warningBannerLayout.height).toBe(18);
    // Flush with the number both audits hold as `kBannerBottom`. If these ever disagree, one of the two
    // tools is measuring elements against a band the panel does not have.
    expect(warningBannerLayout.y + warningBannerLayout.height).toBe(134);
    // The panel's LAST band: the firmware makes this a static_assert rather than arithmetic.
    expect(warningBannerLayout.y + warningBannerLayout.height).toBeLessThanOrEqual(135);

    // Two separate draws, both at bannerY + 4 = y 120. Swapping them would put the summary under the
    // marker and lose four columns silently.
    expect(warningBannerLayout.markerX).toBe(4);
    expect(warningBannerLayout.marker).toBe("!");
    expect(warningBannerLayout.textX).toBe(16);
    expect(warningBannerLayout.y + warningBannerLayout.textDy).toBe(120);

    // THE PARITY FACT A SIMULATOR INVENTS. `warningColor_` is the badgeBorder token —
    // `toRgb565(palette.color("badgeBorder", warningColor_))` in ui_renderer.cpp — and the text is a
    // WHITE literal, not a token. The mockup's ThemeColorTokens has no `warning` key, and a banner drawn
    // from one would paint a colour no palette on the device can produce.
    expect(warningBannerLayout.fillToken).toBe("badgeBorder");
    expect(warningBannerLayout.textColor).toBe("#FFFFFF");

    // 37 is the panel's 40 columns at 6 px less what the marker at x=4 and the gap before x=16 spend —
    // i.e. whole glyphs between the summary's x and the panel edge, and nothing else. Stated as that
    // division rather than as a right-hand margin: `drawWarningBanner` fills `kPanelWidth` edge to edge
    // and reserves no margin at all, so a number derived from one would be pinning an accident.
    expect(warningBannerLayout.columns).toBe(37);
    expect(warningBannerLayout.columns).toBe(
      Math.floor(
        (warningBannerLayout.panelWidth - warningBannerLayout.textX) / warningBannerLayout.glyphWidth
      )
    );
  });
});

describe("the widened gate", () => {
  it("stays closed on an all-ready device and on a factory-fresh one", () => {
    // The mockup's landing state: eight channels connected AND ready, nothing flagged. A banner here
    // would be a red band over every screenshot in the repo.
    const nominal = createSensorTable();
    expect(uncalibratedSensorNumbers(nominal)).toEqual([]);
    expect(warningSensorNumbers(nominal)).toEqual([]);
    expect(warningBannerVisible(nominal)).toBe(false);

    // A DEVICE WITH NOTHING WIRED RAISES NOTHING, which is the arithmetic that answers half of the
    // objection §2c's relocation retired: the connected bitmap comes out of NVS as 0, `uncalibratedCount`
    // counts `connected && !ready`, so both counts are zero on a factory-fresh device.
    const fresh = nothingConnected();
    expect(uncalibratedSensorNumbers(fresh)).toEqual([]);
    expect(warningSensorNumbers(fresh)).toEqual([]);
    expect(warningBannerVisible(fresh)).toBe(false);
  });

  it("opens on a commissioning gap alone — the half §2c widened", () => {
    // Before the widening the gate was `hasWarnings` alone, so this case was FALSE and the two
    // uncalibrated phrasings the firmware composed every pass painted nowhere.
    const gapOnly = uncalibrate(createSensorTable(), 3);
    expect(warningSensorNumbers(gapOnly)).toEqual([]);
    expect(warningBannerVisible(gapOnly)).toBe(true);
    expect(warningBannerText(gapOnly)).toBe("1 channel not calibrated");
  });

  it("opens on a sampling fault alone, and on both at once", () => {
    const samplingOnly = setSensor(createSensorTable(), 5, { undersampling: true });
    expect(uncalibratedSensorNumbers(samplingOnly)).toEqual([]);
    expect(warningBannerVisible(samplingOnly)).toBe(true);

    const both = setSensor(uncalibrate(createSensorTable(), 1), 5, { undersampling: true });
    expect(warningBannerVisible(both)).toBe(true);
  });

  it("suppresses the commissioning gap while a value editor is open, but never a sampling fault", () => {
    // THE ASYMMETRY IS THE POINT. All thirteen `config-*-edit` screens carry a footer hint ending in
    // `hold=cancel` at y=124 — the only place the abort gesture is documented — and the band is exactly
    // that row. On a factory-fresh device `uncalibratedCount` is 8, so without this term the banner would
    // hide `hold=cancel` permanently on the screens whose use clears the condition.
    let gap = createSensorTable();
    for (let number = 1; number <= kSensorCount; number += 1) {
      gap = uncalibrate(gap, number);
    }
    expect(uncalibratedSensorNumbers(gap).length).toBe(kSensorCount);
    expect(warningBannerVisible(gap, false)).toBe(true);
    expect(warningBannerVisible(gap, true)).toBe(false);
    expect(warningBannerText(gap, true)).toBeNull();

    // A sampling fault is EXEMPT: `hasWarnings` says the number on the panel is not the flow, which
    // outranks a gesture reminder on every screen including an editor.
    const flagged = setSensor(createSensorTable(), 5, { undersampling: true });
    expect(warningBannerVisible(flagged, true)).toBe(true);

    // AND THE SUPPRESSION IS OF THE GATE, NOT THE WORDING. With both kinds present and an editor open the
    // line still names both, because `UiController::update` composes `warningSummary` with no knowledge of
    // the editor at all. This looks like a bug and is the faithful mirror.
    const both = setSensor(uncalibrate(createSensorTable(), 1), 5, { undersampling: true });
    expect(warningBannerText(both, true)).toBe("1 not calibrated, 1 undersampling");
  });
});

describe("the banner can only ever carry a fault", () => {
  it("never shows either reassurance string, for any table", () => {
    // The gate is true only when one of the counts is non-zero, so "All sensors nominal" and "No channels
    // in use" are unreachable THROUGH THE BANNER by construction — they stay reachable only through
    // `telemetry.status` and `legend.warning`, which no dataset element binds. That is why the exporter's
    // `diagnostics-banner` check still emits its standing WARNING, and why it should.
    const tables = [
      createSensorTable(),
      nothingConnected(),
      uncalibrate(createSensorTable(), 2),
      allFlagged(),
      setSensor(uncalibrate(createSensorTable(), 1), 5, { undersampling: true })
    ];
    for (const table of tables) {
      for (const editorOpen of [false, true]) {
        const text = warningBannerText(table, editorOpen);
        expect(text).not.toBe("All sensors nominal");
        expect(text).not.toBe("No channels in use");
        // And whenever the gate is closed there is no band at all, not an empty one.
        expect(text === null).toBe(!warningBannerVisible(table, editorOpen));
      }
    }
    // The two reassurance strings are genuinely what the shared composer returns for those tables — this
    // is the gate filtering them out, not the composer having stopped producing them.
    expect(warningSummaryText(createSensorTable())).toBe("All sensors nominal");
    expect(warningSummaryText(nothingConnected())).toBe("No channels in use");
  });
});

describe("the widest summary fits the band, with four columns spare", () => {
  it("measures every branch against the 37 columns and pins the widest", () => {
    /**
     * §2c claimed 37 held "the widest summary the device can produce (`! S1,2,3,4,5,6,7,8` is 18)" — a
     * format the firmware had stopped composing. The sampling-only branch then read
     * `Sampling warning on sensors %s` with the channel list: a 28-character prefix plus a 3k-2 list,
     * which passed 37 at FOUR flagged channels and reached 50 at eight (right edge x=316, 76 px off a
     * 240 px panel). It overflowed SILENTLY, because `drawWarningBanner` prints straight to the panel and
     * gets none of `drawTextElement`'s `~` truncation, and no test on either side had reached the branch
     * above two channels.
     *
     * Every branch is a count now. This test is the bound, and it must be updated DELIBERATELY if any
     * phrasing changes — including its mirror in `ui_controller.cpp`.
     */
    let combined = createSensorTable();
    for (let number = 1; number <= kSensorCount; number += 1) {
      combined = setSensor(combined, number, { qMaxLpm: 0, undersampling: true });
    }
    // The widest of the five branches, and it is the combined one — not the sampling-only one that used
    // to name the channels.
    expect(warningBannerText(combined)).toBe("8 not calibrated, 8 undersampling");
    expect(warningBannerText(combined)?.length).toBe(33);
    expect(warningBannerRightEdge("8 not calibrated, 8 undersampling")).toBe(214);

    const sampling = allFlagged();
    expect(warningSensorNumbers(sampling).length).toBe(kSensorCount);
    expect(warningBannerText(sampling)).toBe("Sampling warning on 8 sensors");
    expect(warningBannerText(sampling)?.length).toBe(29);

    let gap = createSensorTable();
    for (let number = 1; number <= kSensorCount; number += 1) {
      gap = uncalibrate(gap, number);
    }
    expect(warningBannerText(gap)).toBe("8 channels not calibrated");

    // The bound itself, across every branch the gate can open on, and the slack stated as a number so a
    // future phrasing cannot quietly spend it.
    for (const table of [combined, sampling, gap]) {
      const text = warningBannerText(table);
      expect(text).not.toBeNull();
      expect(text!.length).toBeLessThanOrEqual(warningBannerLayout.columns);
      expect(warningBannerRightEdge(text!)).toBeLessThanOrEqual(warningBannerLayout.panelWidth);
    }
    expect(warningBannerLayout.columns - 33).toBe(4);

    // The old format, kept as the measurement that made the trade rather than as a claim about today:
    // this is what the band cannot hold, and the renderer would still clip it without saying so.
    expect(warningBannerRightEdge("Sampling warning on sensors 1, 2, 3, 4, 5, 6, 7, 8")).toBe(316);
  });
});
