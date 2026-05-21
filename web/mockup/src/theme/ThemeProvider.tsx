import { createContext, ReactNode, useCallback, useContext, useEffect, useMemo, useState } from "react";
import { defaultTheme } from "./defaultTheme";
import { cloneTheme, ThemeTokens } from "./types";
import { SchemaValidationError, validateTheme } from "../schema/validation";

interface ThemeContextValue {
  theme: ThemeTokens;
  updateTheme: (updater: (current: ThemeTokens) => ThemeTokens) => void;
  resetTheme: () => void;
}

const ThemeContext = createContext<ThemeContextValue | undefined>(undefined);
const STORAGE_KEY = "stampplc-theme";

function ensureThemeSafety(theme: ThemeTokens): ThemeTokens {
  try {
    return validateTheme(theme);
  } catch (error) {
    const issues = error instanceof SchemaValidationError ? error.issues : [String(error)];
    console.error("Theme validation failed; falling back to defaults", issues);
    return cloneTheme(defaultTheme);
  }
}

function createThemeState(base: ThemeTokens = defaultTheme): ThemeTokens {
  return cloneTheme(ensureThemeSafety(base));
}

function resolveStoredTheme(): ThemeTokens | null {
  if (typeof window === "undefined") {
    return null;
  }
  try {
    const raw = window.localStorage.getItem(STORAGE_KEY);
    if (!raw) {
      return null;
    }
    const parsed = JSON.parse(raw);
    if (!parsed || typeof parsed !== "object") {
      return null;
    }
    const merged: ThemeTokens = {
      name: typeof parsed.name === "string" ? parsed.name : defaultTheme.name,
      colors: {
        ...defaultTheme.colors,
        ...(typeof parsed.colors === "object" ? parsed.colors : {})
      },
      typography: {
        ...defaultTheme.typography,
        ...(typeof parsed.typography === "object" ? parsed.typography : {})
      },
      animation: {
        ...defaultTheme.animation,
        ...(typeof parsed.animation === "object" ? parsed.animation : {})
      }
    };
    return createThemeState(merged);
  } catch {
    return null;
  }
}

export function ThemeProvider({ children }: { children: ReactNode }) {
  const [theme, setTheme] = useState<ThemeTokens>(() => resolveStoredTheme() ?? createThemeState());

  const updateTheme = useCallback((updater: (current: ThemeTokens) => ThemeTokens) => {
    setTheme((previous) => {
      const draft = createThemeState(previous);
      const next = updater(draft);
      return createThemeState(next);
    });
  }, []);

  const resetTheme = useCallback(() => {
    setTheme(createThemeState());
    if (typeof window !== "undefined") {
      window.localStorage.removeItem(STORAGE_KEY);
    }
  }, []);

  const value = useMemo<ThemeContextValue>(
    () => ({
      theme,
      updateTheme,
      resetTheme
    }),
    [theme, updateTheme, resetTheme]
  );

  const themeVariables = useMemo(() => {
    const vars: Record<string, string> = {
      "--theme-display-background": theme.colors.displayBackground,
      "--theme-text-primary": theme.colors.textPrimary,
      "--theme-text-muted": theme.colors.textMuted,
      "--theme-text-strong": theme.colors.textStrong,
      "--theme-value-color": theme.colors.value,
      "--theme-badge-background": theme.colors.badgeBackground,
      "--theme-badge-border": theme.colors.badgeBorder,
      "--theme-icon-color": theme.colors.icon,
      "--theme-legend-color": theme.colors.legend,
      "--theme-grid-minor": theme.colors.gridMinor,
      "--theme-grid-major": theme.colors.gridMajor,
      "--theme-typography-base": `${theme.typography.base}px`,
      "--theme-typography-value": `${theme.typography.value}px`,
      "--theme-typography-badge": `${theme.typography.badge}px`,
      "--theme-animation-easing": theme.animation.easing
    };
    return vars;
  }, [theme]);

  useEffect(() => {
    if (typeof window === "undefined") {
      return;
    }
    window.localStorage.setItem(STORAGE_KEY, JSON.stringify(theme));
  }, [theme]);

  return (
    <ThemeContext.Provider value={value}>
      <div className="theme-scope" style={themeVariables}>
        {children}
      </div>
    </ThemeContext.Provider>
  );
}

export function useTheme() {
  const context = useContext(ThemeContext);
  if (!context) {
    throw new Error("useTheme must be used within a ThemeProvider");
  }
  return context;
}
