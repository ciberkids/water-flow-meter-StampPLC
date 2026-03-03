import { CSSProperties, useCallback, useMemo } from "react";
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
  pendingTransition?: TransitionPreviewState | null;
  scrollIndicator?: string;
  firmwareValues?: import("../types/firmwareActions").FirmwareValueDefinition[];
}

const FRAME_PADDING = 8;
const MINI_PREVIEW_SCALE = 0.45;
const DEVICE_FONT = "\"StampPLC-Pixel\", \"Press Start 2P\", monospace";

export function DisplayViewport({
  layout,
  zoomPercent,
  showGrid,
  valueOverrides,
  pendingTransition,
  scrollIndicator,
  firmwareValues
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
      animation: {
        border: `1px dashed ${theme.colors.legend}`,
        backgroundColor: "rgba(133, 187, 232, 0.08)",
        color: theme.colors.legend,
        textTransform: "uppercase",
        letterSpacing: "0.08em",
        fontSize: "0.6rem",
        display: "flex",
        alignItems: "center",
        justifyContent: "center"
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

        if (element.dataSourceId && firmwareValues) {
          const boundValue = firmwareValues.find((v) => v.id === element.dataSourceId);
          if (boundValue) {
            displayContent = `{{${boundValue.name}}}`;
          }
        }

        if (element.kind === "value" && overrideValue !== undefined) {
          // Overrides take precedence (e.g. simulation values)
          displayContent = overrideValue;
        }

        const isOverridden =
          element.kind === "value" && overrideValue !== undefined && displayContent !== element.content;

        const className = [
          "display-element",
          `kind-${element.kind}`,
          item.outOfBounds ? "overflow" : "",
          isOverridden ? "value-overridden" : ""
        ]
          .filter(Boolean)
          .join(" ");

        switch (element.kind) {
          case "text":
          case "value":
          case "badge":
            return (
              <div key={element.id} className={className} style={style}>
                {displayContent}
              </div>
            );
          case "box":
          case "icon":
            return <div key={element.id} className={className} style={style} />;
          case "animation":
            return (
              <div key={element.id} className={`${className} animation-block`} style={style}>
                <span>{element.metadata?.assetId ?? "Animation"}</span>
              </div>
            );
          case "scrollbar":
            return (
              <div key={element.id} className={`${className} scrollbar-block`} style={style}>
                <span className="scrollbar-indicator">{options?.scrollIndicator ?? "—"}</span>
                <span className="scrollbar-label">{element.content ?? "Screen"}</span>
              </div>
            );
          default:
            return null;
        }
      }),
    [baseStyle, elementStyles, emphasisStyles]
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
        <div className="display-surface" style={surfaceStyle}>
          <DeviceGrid
            width={baseWidth}
            height={baseHeight}
            visible={showGrid}
            minorColor={theme.colors.gridMinor}
            majorColor={theme.colors.gridMajor}
          />
          {renderElementsForLayout(layout, valueOverrides, { scrollIndicator })}
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
