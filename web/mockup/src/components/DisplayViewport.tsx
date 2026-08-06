import { CSSProperties, useCallback, useMemo } from "react";
import { sampleValueFor } from "../utils/sampleValues";
import { ScreenElement } from "../types";
import { LayoutReport } from "../utils/layout";
import { useTheme } from "../theme/ThemeProvider";
import { DeviceGrid } from "./DeviceGrid";
import { TransitionPreviewState } from "../types/transitionPreview";

interface DisplayViewportProps {
  layout: LayoutReport;
  zoomPercent: number;
  showGrid: boolean;
  valueOverrides?: Record<string, string>;
  /**
   * Every binding's value as resolved from the simulated device memory, keyed by binding id.
   *
   * Replaces `globalValues`, which was the loop's flat string map and never reached the renderer at all:
   * the only path from it to pixels was a fan-out into `valueOverrides`, which then shadowed it.
   */
  boundValues?: Record<string, string>;
  pendingTransition?: TransitionPreviewState | null;
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

export function DisplayViewport({
  layout,
  zoomPercent,
  showGrid,
  valueOverrides,
  boundValues,
  pendingTransition,
  scrollIndicator,
  firmwareValues,
  bindingDisplay = "sample",
  powered = true
}: DisplayViewportProps) {
  const { theme } = useTheme();
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
        lineHeight: `${theme.typography.value}px`,
        fontWeight: 700
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
      overrides?: Record<string, string>,
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

        const overrideValue = overrides ? overrides[element.id] : undefined;
        let displayContent = element.content;

        if (element.binding) {
          // Resolved from the simulated device memory when we have it, so what the panel draws is what
          // the device's state says — including `3: --` for a disconnected sensor and `3: WAIT` for one
          // that is enabled but not ready. `sampleValueFor` remains the fallback for bindings memory
          // does not model, and is the whole story in `id` mode, which the design tab uses.
          const definition = firmwareValues?.find((value) => value.id === element.binding);
          displayContent =
            bindingDisplay === "id"
              ? sampleValueFor(element.binding, definition, "id")
              : boundValues?.[element.binding] ??
                sampleValueFor(element.binding, definition, "sample");
        }

        /* A per-element pin, for elements MEMORY DOES NOT ANSWER.
         *
         * Two changes here, and the second reverses a documented decision on purpose:
         *
         *  - Values only, because ValuePlaceholderPanel.tsx:13 filters its list to `kind === "value"`:
         *    a badge can never HAVE a pin, so widening this gate to badges was dead code, and the comment
         *    that came with it — claiming the widening is what let P1's status badges render `--` — was
         *    wrong about its own mechanism. Memory resolution above is what drives those badges, and it
         *    runs for every kind.
         *  - Memory now WINS over a pin. The pin used to take precedence so a specific case could be
         *    held for inspection, which was reasonable when the loop was a flat string map — but it
         *    meant typing one character into a sensor row, then disconnecting that sensor, left the old
         *    number on screen while the badge beside it read `--`. Two homes for one fact, override
         *    winning: exactly what this round removes. To vary a modelled value now, edit memory.
         *
         * An EMPTY string is treated as no pin, so clearing the field restores the resolved value rather
         * than blanking the cell with no way back (Revert is disabled when the element has no `content`).
         */
        const hasPin = overrideValue !== undefined && overrideValue !== "";
        const memoryAnswered = Boolean(element.binding) && boundValues?.[element.binding!] !== undefined;
        if (element.kind === "value" && hasPin && !memoryAnswered) {
          displayContent = overrideValue as string;
        }

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

        const isOverridden =
          element.kind === "value" &&
          hasPin &&
          !memoryAnswered &&
          displayContent !== element.content;

        const className = [
          "display-element",
          `kind-${element.kind}`,
          item.outOfBounds ? "overflow" : "",
          isOverridden ? "value-overridden" : "",
          isWithheld ? "value-disabled" : ""
        ]
          .filter(Boolean)
          .join(" ");

        switch (element.kind) {
          case "text":
          case "badge":
            return (
              <div key={element.id} className={className} style={style}>
                {displayContent}
              </div>
            );
          case "value":
            // No added marker. A little "x" used to be appended for a withheld reading, INSIDE the
            // emulated 240x135 area — a glyph the device never draws, in the one place that has to show
            // only what the device shows. The `value-disabled` class dims it instead.
            return (
              <div key={element.id} className={className} style={style}>
                {displayContent}
              </div>
            );
          case "box":
            return <div key={element.id} className={className} style={style} />;
          case "icon":
            // An OUTLINE with its asset id, not a filled block. The theme's `icon` colour is
            // #56d2ff, so an empty styled div painted P0's 55x55 flow-dots element as a solid cyan
            // rectangle in the middle of the panel — which reads as a rendering fault rather than as
            // "the firmware animates eight dots here". The footprint is what matters for judging
            // layout; the fill was actively misleading.
            return (
              <div
                key={element.id}
                className={`${className} icon-outline`}
                style={{ ...style, background: "transparent" }}
              >
                <span className="icon-outline__label">{element.metadata?.assetId ?? "icon"}</span>
              </div>
            );
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
          {powered ? renderElementsForLayout(layout, valueOverrides, { scrollIndicator }) : null}
          {pendingTransition ? (
            <div
              className={`transition-overlay transition-overlay--${pendingTransition.effect}`}
              style={
                {
                  ["--transition-ease" as const]: theme.animation.easing ?? "ease-in-out"
                } as CSSProperties
              }
            >
              <div className="transition-overlay__glass">
                <div className="transition-overlay__screen">
                  {pendingTransition.previewLayout ? (
                    <div
                      className="transition-overlay__screen-zoom"
                      style={{
                        width: `${pendingTransition.previewLayout.bounds.width}px`,
                        height: `${pendingTransition.previewLayout.bounds.height}px`,
                        transform: `scale(${MINI_PREVIEW_SCALE})`
                      }}
                    >
                      <div
                        className="transition-overlay__mini-surface"
                        style={{
                          width: `${pendingTransition.previewLayout.bounds.width}px`,
                          height: `${pendingTransition.previewLayout.bounds.height}px`,
                          backgroundColor: theme.colors.displayBackground,
                          borderColor: theme.colors.badgeBorder
                        }}
                      >
                        {renderElementsForLayout(pendingTransition.previewLayout)}
                      </div>
                    </div>
                  ) : (
                    <div className="transition-overlay__placeholder">
                      {pendingTransition.screenName ?? pendingTransition.screenId}
                    </div>
                  )}
                </div>
                <div className="transition-overlay__meta">
                  <span className="transition-overlay__target">
                    {pendingTransition.screenName ?? pendingTransition.screenId}
                  </span>
                  {pendingTransition.actionLabel ? (
                    <span className="transition-overlay__action">{pendingTransition.actionLabel}</span>
                  ) : null}
                  <span className="transition-overlay__trigger">{pendingTransition.triggerLabel}</span>
                </div>
              </div>
            </div>
          ) : null}
        </div>
      </div>
    </div>
  );
}
