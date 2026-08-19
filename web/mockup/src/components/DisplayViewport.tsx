import { CSSProperties, Fragment, useCallback, useMemo } from "react";
import { sampleValueFor } from "../utils/sampleValues";
import { ScreenElement } from "../types";
import { LayoutReport } from "../utils/layout";
import { packSelectorLayout } from "../utils/packSelector";
import { useTheme } from "../theme/ThemeProvider";
import { DeviceGrid } from "./DeviceGrid";

/** One row of the Select Menu. Index 0 is always the built-in default and never comes from the card. */
export interface PackSelectorEntry {
  label: string;
  /** The menu currently running, marked so the operator can see what they are leaving (§3.4). */
  active: boolean;
}

export interface PackSelectorState {
  entries: PackSelectorEntry[];
  cursor: number;
  /** The card held more than the page can show. Said out loud rather than silently dropped. */
  truncated: boolean;
}

interface DisplayViewportProps {
  layout: LayoutReport;
  zoomPercent: number;
  showGrid: boolean;
  /**
   * Every binding's value as resolved from the simulated device memory, keyed by binding id.
   *
   * Replaces `globalValues`, which was the loop's flat string map and never reached the renderer at all:
   * the only path from it to pixels was a fan-out into `valueOverrides`, which then shadowed it.
   */
  boundValues?: Record<string, string>;
  scrollIndicator?: string;
  firmwareValues?: import("../types/firmwareActions").FirmwareValueDefinition[];
  /**
   * What a bound value renders as.
   *
   * `sample` shows plausible device output — the mode for judging layout and legibility, and the
   * default because that is the question the viewport exists to answer. `id` shows the short binding
   * id in braces, for a caller that needs to know WHICH value is bound rather than how it looks.
   *
   * NO CALL SITE PASSES `id` TODAY. Both render sites — the simulation viewport (App.tsx) and the theme
   * editor's live preview — use the default. This doc used to assert the design tab relied on it, which
   * was simply untrue; the mode is kept because it costs one branch, not because something uses it.
   *
   * Neither shows the catalogue DESCRIPTION any more. Rendering `{{Sensor 3 instantaneous flow}}`
   * across 137 value elements is what made the panel unreadable.
   */
  bindingDisplay?: "sample" | "id";
  /**
   * Backlight state. `false` renders an empty surface, because that is what the device shows: the idle
   * branch blanks and draws nothing at all.
   *
   * Defaults to true so the design tab and the transition preview, which have no notion of device mode,
   * keep rendering.
   */
  powered?: boolean;
  /**
   * The firmware-drawn Select Menu, when it is open (Loadable_UI_Menu_Packs §3.4).
   *
   * The one page the dataset cannot describe. `UiRenderer::drawPackSelector` short-circuits before every
   * table-driven path, which is the requirement rather than an optimisation — §3.4.1 says the gesture
   * must work "even if the active pack draws nothing at all", and that is only true if the firmware owns
   * this page. So the simulator has to own it too.
   *
   * Until this existed the app tracked `selectorOpen`, wrote it into the status line and the loop badge,
   * and left the 240x135 area showing whatever screen the operator had been on — the chrome claiming a
   * page was open while the panel showed a different one, which is the one thing a viewport must not do.
   */
  packSelector?: PackSelectorState | null;
  /**
   * Aggregate flow in L/MIN (§2a), which decides whether the flow-dot chase runs at all.
   *
   * Undefined or zero leaves the dots unlit, which is what the device shows with no flow.
   */
  aggregateFlowLpm?: number;
  /**
   * Whether the firmware loop is advancing.
   *
   * The chase only animates while it is, because a repaint is what advances it — on the device and
   * here. `drawFlowDots` steps `flowDotPhase_` once per PAINTED FRAME and deliberately does not read
   * `millis()`: the comment there explains that a millis-derived period aliases past recognition when
   * an info page repaints at 1 Hz. A stopped simulated loop is a panel with no counted repaints, so
   * animating one would show motion nothing had drawn. (This said "driven by `millis()` on the device",
   * which is the one thing that function exists not to do.)
   */
  animating?: boolean;
  /**
   * Repaints so far. The flow-dot chase advances one position per repaint — the only rate the panel
   * can actually show — rather than on a timer of its own.
   */
  repaintCount?: number;
}

const FRAME_PADDING = 8;
const MINI_PREVIEW_SCALE = 0.45;
const DEVICE_FONT = "\"StampPLC-Pixel\", \"Press Start 2P\", monospace";

/**
 * Where the thumb sits, from a `"index / count"` indicator.
 *
 * Falls back to a full-height thumb when the position is unknown, because a scrollbar with no thumb
 * looks broken while a full one reads as "one level, no scrolling" — which is the truth at depth 0.
 */
function scrollThumbStyle(indicator: string | undefined): CSSProperties {
  const match = indicator ? /(\d+)\s*\/\s*(\d+)/.exec(indicator) : null;
  if (!match) {
    return { top: "0%", height: "100%" };
  }
  const index = Number(match[1]);
  const count = Math.max(1, Number(match[2]));
  const height = Math.max(12, 100 / count);
  const top = Math.min(100 - height, ((index - 1) / count) * 100);
  return { top: `${top}%`, height: `${height}%` };
}

/**
 * `UiRenderer::drawPackSelector`, in the browser.
 *
 * Every coordinate here is the firmware's, read off ui_renderer.cpp rather than chosen: the title at
 * (4, 4), rows at `20 + i * 12`, the cursor glyph at x=4 and the label at x=16, the active marker at
 * x=228, and the footer at y=116. A page that looked right but sat two pixels off would be the mockup
 * disagreeing with the device about the one screen that exists to be dependable.
 *
 * The cursor is a leading `>` and not an inverted row, for the firmware's own reason: Font0 has no bold,
 * and inverting a row would mean a fillRect per row — more bus traffic on the page that most needs to
 * work when other things do not.
 *
 * The two colours are the firmware's mapping, not a guess: `ui_renderer.cpp` resolves `highlightColor_`
 * from the palette's `value` and `warningColor_` from `badgeBorder`, so a theme edit moves both surfaces
 * together instead of the panel and the preview drifting apart on a recoloured pack.
 */
function renderPackSelector(
  selector: PackSelectorState,
  baseStyle: CSSProperties,
  theme: ReturnType<typeof useTheme>["theme"]
) {
  const at = (left: number, top: number, extra?: CSSProperties): CSSProperties => ({
    ...baseStyle,
    left: `${left}px`,
    top: `${top}px`,
    ...extra
  });

  // A selector with no entries is a wiring bug, not an operator-visible state, and the firmware says so
  // on screen rather than painting an empty page nobody can escape. Mirrored, including the colour.
  if (selector.entries.length === 0) {
    return (
      <>
        <span style={at(packSelectorLayout.title.x, packSelectorLayout.title.y)}>SELECT MENU</span>
        <span
          style={at(packSelectorLayout.cursorX, packSelectorLayout.rows.top, {
            color: theme.colors.badgeBorder
          })}
        >
          {packSelectorLayout.unavailableText}
        </span>
      </>
    );
  }

  return (
    <>
      <span style={at(packSelectorLayout.title.x, packSelectorLayout.title.y)}>SELECT MENU</span>
      {selector.entries.map((entry, index) => {
        const top = packSelectorLayout.rows.top + index * packSelectorLayout.rows.pitch;
        const onCursor = index === selector.cursor;
        return (
          <Fragment key={`${index}-${entry.label}`}>
            <span
              style={at(packSelectorLayout.cursorX, top, onCursor ? { color: theme.colors.value } : undefined)}
            >
              {onCursor ? ">" : " "}
            </span>
            <span
              style={at(packSelectorLayout.labelX, top, onCursor ? { color: theme.colors.value } : undefined)}
            >
              {entry.label}
            </span>
            {entry.active ? (
              <span style={at(packSelectorLayout.activeMarkerX, top)}>*</span>
            ) : null}
          </Fragment>
        );
      })}
      <span style={at(packSelectorLayout.footer.x, packSelectorLayout.footer.y)}>
        {selector.truncated ? packSelectorLayout.truncatedText : packSelectorLayout.footerText}
      </span>
    </>
  );
}

export function DisplayViewport({
  layout,
  zoomPercent,
  showGrid,
  boundValues,
  scrollIndicator,
  firmwareValues,
  bindingDisplay = "sample",
  powered = true,
  packSelector = null,
  aggregateFlowLpm = 0,
  animating = false,
  repaintCount = 0
}: DisplayViewportProps) {
  const { theme } = useTheme();

  /**
   * Which dot is lit: nothing without flow, the first with flow but a stopped loop, and one step per
   * repaint while it runs.
   *
   * Derived rather than held in state, so there is no timer to fall out of step with the loop and no
   * second clock in the app. It also means the chase is deterministic: the same tick count always
   * gives the same frame, which is what makes it reproducible in a screenshot.
   */
  // 0.01 L/min, restated from the old 0.001 L/s so the threshold moved with the unit — matching
  // `drawFlowDots`. Left at 0.001 it would have declared flow at a sixtieth of the intended rate.
  const hasFlow = aggregateFlowLpm > 0.01;
  const dotPhase = !hasFlow ? -1 : animating ? repaintCount % 4 : 0;

  const orientation = layout.bounds.orientation;
  const scale = useMemo(() => Math.max(zoomPercent / 100, 1), [zoomPercent]);
  const baseWidth = layout.bounds.width;
  const baseHeight = layout.bounds.height;
  const scaledWidth = baseWidth * scale;
  const scaledHeight = baseHeight * scale;

  const baseStyle: CSSProperties = useMemo(
    () => ({
      position: "absolute",
      color: theme.colors.textPrimary,
      fontFamily: DEVICE_FONT,
      fontSize: `${theme.typography.base}px`,
      lineHeight: `${theme.typography.base}px`,
      fontWeight: 400,
      letterSpacing: "0",
      textTransform: "none",
      whiteSpace: "nowrap",
      imageRendering: "pixelated",
      zIndex: 2
    }),
    [theme]
  );

  const elementStyles = useMemo<Record<ScreenElement["kind"], CSSProperties>>(
    () => ({
      text: {},
      value: {
        color: theme.colors.value,
        fontSize: `${theme.typography.value}px`,
        lineHeight: `${theme.typography.value}px`
        /**
         * NO fontWeight. Font0 has no bold — `ui_renderer.cpp` says so at the one place that already
         * works around it — so the device distinguishes a value by COLOUR alone (highlightColor_).
         *
         * Rendering 700 here made the mockup show a weight the panel cannot produce, and it was
         * visible as an inconsistency rather than a nicety: on the formula row, `f-mult` is a `value`
         * and the other terms are `text`, so the multiplier looked bold on S4 while the emphasised
         * term on S5 and S6 only changed colour. Same emphasis, two different treatments, neither
         * matching the device.
         */
      },
      badge: {
        padding: "1px 3px",
        border: `1px solid ${theme.colors.badgeBorder}`,
        backgroundColor: theme.colors.badgeBackground,
        color: theme.colors.textPrimary,
        fontSize: `${theme.typography.badge}px`,
        lineHeight: `${theme.typography.badge}px`,
        textTransform: "none",
        letterSpacing: "0"
      },
      box: {
        border: `1px solid ${theme.colors.badgeBorder}`,
        backgroundColor: theme.colors.badgeBackground
      },
      icon: {
        backgroundColor: theme.colors.icon
      },
      scrollbar: {
        border: `1px solid ${theme.colors.badgeBorder}`,
        backgroundColor: "rgba(255, 255, 255, 0.04)",
        color: theme.colors.textStrong,
        fontSize: `${theme.typography.badge}px`,
        display: "flex",
        flexDirection: "column",
        alignItems: "center",
        justifyContent: "center",
        gap: "2px",
        letterSpacing: "0.1em"
      }
    }),
    [theme]
  );

  const emphasisStyles = useMemo<Record<string, CSSProperties>>(
    () => ({
      normal: {},
      strong: { color: theme.colors.textStrong },
      muted: { color: theme.colors.textMuted }
    }),
    [theme]
  );

  const renderElementsForLayout = useCallback(
    (
      layoutToRender: LayoutReport,
      options?: { scrollIndicator?: string }
    ) =>
      layoutToRender.elements.map((item) => {
        const { element } = item;
        const style: CSSProperties = {
          ...baseStyle,
          ...(elementStyles[element.kind] ?? {}),
          ...(emphasisStyles[element.emphasis ?? "normal"] ?? {}),
          left: `${item.left}px`,
          top: `${item.top}px`
        };

        if (item.width > 0) {
          style.width = `${item.width}px`;
        }
        if (item.height > 0) {
          style.height = `${item.height}px`;
        }

        if (element.kind === "box" || element.kind === "icon") {
          style.display = "block";
        }

        let displayContent = element.content;

        if (element.binding) {
          // Resolved from the simulated device memory when we have it, so what the panel draws is what
          // the device's state says — including `3: --` for a disconnected sensor and `3: SET?` for one
          // that is enabled but not calibrated. `sampleValueFor` remains the fallback for bindings
          // memory does not model, and is the whole story in `id` mode, which the design tab uses.
          const definition = firmwareValues?.find((value) => value.id === element.binding);
          displayContent =
            bindingDisplay === "id"
              ? sampleValueFor(element.binding, definition, "id")
              : boundValues?.[element.binding] ??
                sampleValueFor(element.binding, definition, "sample");
        }

        /* The per-element PIN is gone, along with the Value placeholders panel that produced it.
         *
         * It was the last of the two-homes-for-one-fact defects in this file: a pin was a second copy
         * of a value memory already answered, keyed by element id instead of by binding, so it could
         * not follow the selected sensor and went stale the moment memory moved. Memory had already
         * been given precedence over it, which left the pin reachable only for values memory does not
         * model — and with the panel removed, nothing can set one at all. Editing memory is now the
         * single way to vary what the panel draws. */

        /* §4.3.19 — a withheld reading, detected from the string the device actually draws.
         *
         * This used to read `globalValues["sensor.<n>.connected"]`, a binding id that is not among the
         * 104 the firmware advertises and that nothing could ever write, so the branch never fired once
         * and every sensor rendered as connected and flowing. Now that memory resolves the binding, the
         * device's own withheld formats — bare `--` for a status, `<n>: --` for a metric — are the
         * signal, and the mockup does not need a parallel notion of "disabled". */
        const isWithheld =
          Boolean(element.binding) &&
          (displayContent === "--" || /:\s+--$/.test(displayContent ?? ""));

        const className = [
          "display-element",
          `kind-${element.kind}`,
          item.outOfBounds ? "overflow" : "",
          isWithheld ? "value-disabled" : ""
        ]
          .filter(Boolean)
          .join(" ");

        /**
         * `data-element-id` is for the visual suite, which used to find an element by the TEXT it
         * renders. That coupled every geometry assertion to content: when the Design toolbox stopped
         * offering a Content box for a bindable kind, the test could no longer label an element and
         * four corner checks died on a fill() timeout. The id is stable, the content is not.
         */
        switch (element.kind) {
          case "text":
          case "badge":
            return (
              <div key={element.id} data-element-id={element.id} className={className} style={style}>
                {displayContent}
              </div>
            );
          case "value":
            // No added marker. A little "x" used to be appended for a withheld reading, INSIDE the
            // emulated 240x135 area — a glyph the device never draws, in the one place that has to show
            // only what the device shows. The `value-disabled` class dims it instead.
            return (
              <div key={element.id} data-element-id={element.id} className={className} style={style}>
                {displayContent}
              </div>
            );
          case "box":
            return <div key={element.id} data-element-id={element.id} className={className} style={style} />;
          case "icon": {
            /**
             * `flow-dots` draws the DOTS, because that is what the device draws.
             *
             * It used to draw a labelled outline reading "flow-dots" — honest about the footprint,
             * but it left the landing screen of the simulator showing a dashed box where the panel
             * shows a chase, and the SVG gallery drew the four dots. Two renderings of one element
             * disagreeing is the same defect class as the sample tables that disagreed about Modbus
             * ID, and this one sat on P0.
             *
             * Geometry from `drawFlowDots`: four dots (the agreed count — the old comment here said
             * eight), radius min(spacing, height) / 3, the leftmost lit. Any OTHER asset id keeps the
             * outline, which stays the honest rendering for something with no implementation.
             */
            const isFlowDots = element.metadata?.assetId === "flow-dots";
            if (!isFlowDots) {
              return (
                <div
                  key={element.id}
                  className={`${className} icon-outline`}
                  style={{ ...style, background: "transparent" }}
                >
                  <span className="icon-outline__label">{element.metadata?.assetId ?? "icon"}</span>
                </div>
              );
            }
            const w = element.width ?? 40;
            const h = element.height ?? 12;
            const count = 4;
            const spacing = w / count;
            const r = Math.min(spacing, h) / 3;
            return (
              <div key={element.id} className={className} style={{ ...style, background: "transparent" }}>
                <svg width="100%" height="100%" viewBox={`0 0 ${w} ${h}`} style={{ display: "block" }}>
                  {Array.from({ length: count }, (_, i) => {
                    const lit = i === dotPhase;
                    return (
                      <circle
                        key={i}
                        cx={spacing / 2 + i * spacing}
                        cy={h / 2}
                        r={r}
                        fill={lit ? theme.colors.value : "none"}
                        stroke={lit ? theme.colors.value : theme.colors.textMuted}
                        strokeWidth={0.75}
                        opacity={lit ? 1 : 0.5}
                      />
                    );
                  })}
                </svg>
              </div>
            );
          }
          case "scrollbar":
            // A TRACK AND A THUMB, sized from the ring position — which is what the firmware draws.
            //
            // It used to render two text spans (the index and the word "Screen") inside an element
            // 5 px wide and 104 px tall, so both overflowed to the right of the panel as a stray
            // floating "1" and a clipped "Scree". That is the "weird un-editable" artifact: not the
            // scrollbar being wrong, but text no 5 px column could ever hold.
            return (
              <div
                key={element.id}
                className={`${className} scrollbar-block`}
                style={style}
                title={`Level position ${options?.scrollIndicator ?? "—"}`}
              >
                <span
                  className="scrollbar-thumb"
                  style={scrollThumbStyle(options?.scrollIndicator)}
                />
              </div>
            );
          default:
            return null;
        }
      }),
    // Stale deps were a real hazard, not a lint nit: `boundValues`, `firmwareValues` and
    // `bindingDisplay` are read from the closure, so without them here a new value map would render the
    // FIRST frame forever — and the whole point of resolving from memory is that it changes.
    [baseStyle, bindingDisplay, boundValues, elementStyles, emphasisStyles, firmwareValues]
  );

  const wrapperStyle: CSSProperties = {
    width: `${scaledWidth + FRAME_PADDING * 2}px`,
    height: `${scaledHeight + FRAME_PADDING * 2}px`,
    padding: `${FRAME_PADDING}px`
  };

  const scaleStyle: CSSProperties = {
    width: `${baseWidth}px`,
    height: `${baseHeight}px`,
    transform: `scale(${scale})`,
    transformOrigin: "top left",
    willChange: "transform"
  };

  const surfaceStyle: CSSProperties = useMemo(
    () => ({
      width: `${baseWidth}px`,
      height: `${baseHeight}px`,
      position: "relative",
      backgroundColor: theme.colors.displayBackground,
      color: theme.colors.textPrimary,
      border: `1px solid ${theme.colors.badgeBorder}`,
      imageRendering: "pixelated",
      overflow: "hidden",
      fontFamily: DEVICE_FONT
    }),
    [baseHeight, baseWidth, theme]
  );

  return (
    <div className={`viewport-wrapper orientation-${orientation}`} style={wrapperStyle}>
      <div className="viewport-scale" style={scaleStyle}>
        <div
          className={powered ? "display-surface" : "display-surface display-surface--off"}
          style={surfaceStyle}
        >
          <DeviceGrid
            width={baseWidth}
            height={baseHeight}
            visible={showGrid && powered}
            minorColor={theme.colors.gridMinor}
            majorColor={theme.colors.gridMajor}
          />
          {/* Backlight off draws NOTHING. ui_renderer.cpp:115-119 is `setBacklight(false)` followed by
              `fillScreen(backgroundColor_)` with no drawString anywhere in the idle branch — so a blank
              surface is the faithful render, and any "display off" wording belongs to the chrome outside
              this frame. (The dataset's state-idle screen carries a "- Display off -" label; that label
              is the dataset being unfaithful, and it is not what we draw here.) */}
          {powered && packSelector
            ? renderPackSelector(packSelector, baseStyle, theme)
            : powered
              ? renderElementsForLayout(layout, { scrollIndicator })
              : null}
          {/* THE TRANSITION OVERLAY IS GONE (J7), not disabled.

              It faded a miniature of the incoming screen over the panel on every UP/DOWN — and on a device
              whose whole navigation IS UP/DOWN, that meant an animation over almost every press, obscuring
              the thing this viewport exists to show. The firmware draws no such transition; it clears and
              repaints. `App.tsx` had been passing `pendingTransition={undefined}` ever since that decision,
              so this branch could not render, and its state, its type, its 1500 ms timer and 23 lines of CSS
              stayed behind it. What the decision did NOT kill is the ring `ScreenSelector` puts on the target
              row, which is a workspace affordance rather than a device simulation and still works. */}
        </div>
      </div>
    </div>
  );
}
