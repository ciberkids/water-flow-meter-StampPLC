export type ThemeAnimationPreset =
  | "linear"
  | "ease-in"
  | "ease-out"
  | "ease-in-out"
  | "cubic-bezier(0.4, 0, 0.2, 1)";

export interface ThemeColorTokens {
  displayBackground: string;
  textPrimary: string;
  textMuted: string;
  textStrong: string;
  value: string;
  badgeBackground: string;
  badgeBorder: string;
  icon: string;
  legend: string;
  gridMinor: string;
  gridMajor: string;
}

export interface ThemeTypographyTokens {
  base: number;
  value: number;
  badge: number;
}

export interface ThemeAnimationTokens {
  easing: ThemeAnimationPreset;
}

export interface ThemeTokens {
  name: string;
  colors: ThemeColorTokens;
  typography: ThemeTypographyTokens;
  animation: ThemeAnimationTokens;
}

export function cloneTheme(base: ThemeTokens): ThemeTokens {
  return {
    name: base.name,
    colors: { ...base.colors },
    typography: { ...base.typography },
    animation: { ...base.animation }
  };
}
