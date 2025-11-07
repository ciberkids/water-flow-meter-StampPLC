import { CSSProperties, useMemo } from "react";
import { ScreenElement } from "../types";
import { LayoutReport } from "../utils/layout";
import { useTheme } from "../theme/ThemeProvider";
import { DeviceGrid } from "./DeviceGrid";

interface DisplayViewportProps {
  layout: LayoutReport;
  zoomPercent: number;
  showGrid: boolean;
}

const FRAME_PADDING = 8;
const DEVICE_FONT = "\"StampPLC-Pixel\", \"Press Start 2P\", monospace";

export function DisplayViewport({ layout, zoomPercent, showGrid }: DisplayViewportProps) {
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
          {layout.elements.map((item) => {
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

            const className = [
              "display-element",
              `kind-${element.kind}`,
              item.outOfBounds ? "overflow" : ""
            ]
              .filter(Boolean)
              .join(" ");

            switch (element.kind) {
              case "text":
              case "value":
              case "badge":
                return (
                  <div key={element.id} className={className} style={style}>
                    {element.content}
                  </div>
                );
              case "box":
              case "icon":
                return <div key={element.id} className={className} style={style} />;
              default:
                return null;
            }
          })}
          <div className="legend" style={{ color: theme.colors.legend }}>
            <span>Red pulses per X L</span>
            <span>Green = Ready</span>
            <span>Blue = Flow</span>
          </div>
        </div>
      </div>
    </div>
  );
}
