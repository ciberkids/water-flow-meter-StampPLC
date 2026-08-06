import { ChangeEvent, ReactNode } from "react";
import { DisplayViewport } from "./DisplayViewport";
import type { LayoutReport } from "../utils/layout";
import type { ScreenDefinition } from "../types";
import { useTheme } from "../theme/ThemeProvider";
import { ThemeColorTokens } from "../theme/types";

function normalizeColorToHex(color: string): string {
  if (color.startsWith("#")) {
    return color;
  }
  const match = color.match(/rgba?\((\d+),\s*(\d+),\s*(\d+)(?:,\s*([\d.]+))?\)/i);
  if (!match) {
    return "#000000";
  }
  const r = Number(match[1]).toString(16).padStart(2, "0");
  const g = Number(match[2]).toString(16).padStart(2, "0");
  const b = Number(match[3]).toString(16).padStart(2, "0");
  return `#${r}${g}${b}`.toUpperCase();
}

function hexToRgb(hex: string): { r: number; g: number; b: number } {
  let normalized = hex.replace("#", "");
  if (normalized.length === 3) {
    normalized = normalized
      .split("")
      .map((char) => char + char)
      .join("");
  }
  const value = parseInt(normalized, 16);
  const r = (value >> 16) & 255;
  const g = (value >> 8) & 255;
  const b = value & 255;
  return { r, g, b };
}

function mergeColorWithHex(original: string, hex: string): string {
  if (original.startsWith("rgba")) {
    const alphaMatch = original.match(/rgba\([^,]+,[^,]+,[^,]+,\s*([\d.]+)\)/i);
    const alpha = alphaMatch ? Math.min(1, Math.max(0, parseFloat(alphaMatch[1]))) : 1;
    const { r, g, b } = hexToRgb(hex);
    if (alpha >= 1) {
      return `rgb(${r}, ${g}, ${b})`;
    }
    return `rgba(${r}, ${g}, ${b}, ${Math.round(alpha * 1000) / 1000})`;
  }
  return hex.toUpperCase();
}

function sanitizeUserHex(raw: string): string | null {
  const trimmed = raw.trim();
  if (!/^#?[0-9a-fA-F]{3,6}$/.test(trimmed)) {
    return null;
  }
  let normalized = trimmed.startsWith("#") ? trimmed.slice(1) : trimmed;
  if (normalized.length === 3) {
    normalized = normalized
      .split("")
      .map((char) => char + char)
      .join("");
  }
  if (normalized.length !== 6) {
    return null;
  }
  return `#${normalized.toUpperCase()}`;
}

const colorFields: Array<{ key: keyof ThemeColorTokens; label: string; name: string }> = [
  { key: "displayBackground", label: "Display background", name: "theme-background" },
  { key: "textPrimary", label: "Primary text", name: "theme-primary" },
  { key: "textMuted", label: "Muted text", name: "theme-muted" },
  { key: "textStrong", label: "Strong text", name: "theme-strong" },
  { key: "value", label: "Accent value", name: "theme-accent" },
  { key: "badgeBackground", label: "Badge background", name: "theme-badge-background" },
  { key: "badgeBorder", label: "Badge border", name: "theme-badge-border" },
  { key: "icon", label: "Icon colour", name: "theme-icon" },
  { key: "legend", label: "Legend text", name: "theme-legend" },
  { key: "gridMajor", label: "Grid major line", name: "theme-grid-major" },
  { key: "gridMinor", label: "Grid minor line", name: "theme-grid-minor" }
];

const warmPresetColors: Partial<ThemeColorTokens> = {
  displayBackground: "#1a0b11",
  textPrimary: "#ffece6",
  textMuted: "#f5b5a5",
  textStrong: "#ffffff",
  value: "#ff6b5a",
  badgeBackground: "#2f141b",
  badgeBorder: "#ff9c8c",
  icon: "#ff6b5a",
  legend: "#ffb49f",
  gridMinor: "rgba(255, 126, 104, 0.22)",
  gridMajor: "rgba(255, 126, 104, 0.48)"
};

interface ThemeEditorProps {
  layout?: LayoutReport;
  zoomPercent: number;
  showGrid: boolean;
  screen?: ScreenDefinition;
  previewFooter?: ReactNode;
  stackControls?: boolean;
  sidebarContent?: ReactNode;
  firmwareValues?: import("../types/firmwareActions").FirmwareValueDefinition[];
  /**
   * Values resolved from the simulated device memory.
   *
   * Passed through so this preview cannot contradict the Simulation tab. Without it the theme preview
   * drew the sample strings: disconnect sensor 1 in Simulation, switch to Design, and the same screen
   * showed `1:   2.34 L/s` with an `OK` badge for the channel you had just switched off.
   */
  boundValues?: Record<string, string>;
}

export function ThemeEditor({
  layout,
  zoomPercent,
  showGrid,
  screen,
  previewFooter,
  stackControls = false,
  sidebarContent,
  firmwareValues,
  boundValues
}: ThemeEditorProps) {
  const { theme, updateTheme, resetTheme } = useTheme();
  const previewZoom =
    layout?.bounds.orientation === "landscape" ? Math.min(zoomPercent, 160) : Math.min(zoomPercent, 200);
  const zoomLabel = previewZoom !== zoomPercent ? `${previewZoom}% (clamped for preview)` : `${previewZoom}%`;

  const applyColorChange = (key: keyof ThemeColorTokens, hexValue: string) => {
    updateTheme((current) => ({
      ...current,
      colors: {
        ...current.colors,
        [key]: mergeColorWithHex(current.colors[key], hexValue)
      }
    }));
  };

  const handleBaseTypographyChange = (event: ChangeEvent<HTMLInputElement>) => {
    const baseValue = Number(event.target.value);
    updateTheme((current) => ({
      ...current,
      typography: {
        ...current.typography,
        base: Math.max(6, Math.min(14, baseValue))
      }
    }));
  };

  const handleValueTypographyChange = (event: ChangeEvent<HTMLInputElement>) => {
    const raw = Number(event.target.value);
    updateTheme((current) => ({
      ...current,
      typography: {
        ...current.typography,
        value: Math.max(6, Math.min(20, raw))
      }
    }));
  };

  const handleBadgeTypographyChange = (event: ChangeEvent<HTMLInputElement>) => {
    const raw = Number(event.target.value);
    updateTheme((current) => {
      const nextBadge = Math.max(6, Math.min(14, raw));
      return {
        ...current,
        typography: {
          ...current.typography,
          badge: nextBadge
        }
      };
    });
  };

  const applyWarmPreset = () => {
    updateTheme((current) => ({
      ...current,
      colors: {
        ...current.colors,
        ...warmPresetColors
      }
    }));
  };

  const previewLabel = screen?.name ?? screen?.id ?? "Select a screen";
  const viewportLabel = layout
    ? `${layout.bounds.width} × ${layout.bounds.height}px • ${layout.bounds.orientation}`
    : "Configure dataset to view preview";

  const renderColorSection = () => (
    <section className="theme-editor__section">
      <h4>Colours</h4>
      <div className="theme-editor__grid">
        {colorFields.map((field) => (
          <label key={field.key} className="theme-editor__field">
            <span>{field.label}</span>
            <div className="color-input-row">
              <input
                type="color"
                name={field.name}
                value={normalizeColorToHex(theme.colors[field.key])}
                onInput={(event) =>
                  applyColorChange(field.key, (event.target as HTMLInputElement).value)
                }
                onChange={(event) => applyColorChange(field.key, event.target.value)}
              />
              <input
                type="text"
                aria-label={`${field.label} hex value`}
                value={normalizeColorToHex(theme.colors[field.key])}
                onChange={(event) => {
                  const sanitized = sanitizeUserHex(event.target.value);
                  if (sanitized) {
                    applyColorChange(field.key, sanitized);
                  }
                }}
              />
            </div>
          </label>
        ))}
      </div>
    </section>
  );

  const renderTypographySection = () => (
    <section className="theme-editor__section">
      <h4>Typography</h4>
      <div className="theme-editor__grid theme-editor__grid--compact">
        <label className="theme-editor__field">
          <span>Base font size ({theme.typography.base}px)</span>
          <input
            type="range"
            min={6}
            max={14}
            value={theme.typography.base}
            onChange={handleBaseTypographyChange}
          />
        </label>
        <label className="theme-editor__field">
          <span>Value font size ({theme.typography.value}px)</span>
          <input
            type="range"
            min={6}
            max={20}
            value={theme.typography.value}
            onChange={handleValueTypographyChange}
          />
        </label>
        <label className="theme-editor__field">
          <span>Badge font size ({theme.typography.badge}px)</span>
          <input
            type="range"
            min={6}
            max={14}
            value={theme.typography.badge}
            onChange={handleBadgeTypographyChange}
          />
        </label>
      </div>
    </section>
  );

  const inlineSections = stackControls ? (
    <div className="theme-editor__inline-sections">
      {renderColorSection()}
      {renderTypographySection()}
    </div>
  ) : null;

  const sidebarSections = stackControls ? sidebarContent : sidebarContent ?? (
    <>
      {renderColorSection()}
      {renderTypographySection()}
    </>
  );

  return (
    <section className={`theme-editor${stackControls ? " theme-editor--stacked" : ""}`} aria-label="Design controls">
      <header className="theme-editor__header">
        <div>
          <h3>Design system</h3>
          <p>
            Adjust palette, typography, and easing tokens. Updates sync instantly to the live preview and
            persist in <code>screens.json</code> under <code>theme</code> for the translator.
          </p>
        </div>
        <dl className="theme-editor__meta">
          <div>
            <dt>Previewing</dt>
            <dd>{previewLabel}</dd>
          </div>
          <div>
            <dt>Viewport</dt>
            <dd>{viewportLabel}</dd>
          </div>
          <div>
            <dt>Zoom</dt>
            <dd>{zoomLabel}</dd>
          </div>
        </dl>
      </header>

      <div className="theme-editor__layout">
        <aside className="theme-editor__preview">
          <h4>Live preview</h4>
          {layout ? (
            <div className="theme-editor__viewport">
              <DisplayViewport
                layout={layout}
                zoomPercent={previewZoom}
                showGrid={showGrid}
                firmwareValues={firmwareValues}
                boundValues={boundValues}
              />
            </div>
          ) : (
            <p className="theme-editor__empty">Select a screen to preview design changes.</p>
          )}
          {previewFooter ? (
            <div className="theme-editor__preview-footer">{previewFooter}</div>
          ) : null}
          {inlineSections}
        </aside>

        {sidebarSections ? <div className="theme-editor__sections">{sidebarSections}</div> : null}
      </div>

      <div className="theme-editor__actions">
        <button type="button" onClick={applyWarmPreset}>
          Apply warm preset
        </button>
        <button type="button" onClick={resetTheme}>
          Reset design
        </button>
      </div>
    </section>
  );
}
