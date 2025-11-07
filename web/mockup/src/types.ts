import type { ThemeTokens } from "./theme/types.js";

export type ElementKind = "text" | "value" | "badge" | "box" | "icon";
export type DisplayOrientation = "portrait" | "landscape";

export interface ScreenElement {
  id: string;
  kind: ElementKind;
  x: number;
  y: number;
  width?: number;
  height?: number;
  content?: string;
  align?: "left" | "center" | "right";
  emphasis?: "normal" | "strong" | "muted";
}

export interface ScreenFlow {
  id: string;
  label: string;
  targetScreenId: string;
  trigger: "button" | "timeout" | "data";
  guard?: string;
}

export interface ScreenGraphicAsset {
  id: string;
  type: "svg-sequence" | "bitmap" | "icon";
  source: string;
  frames?: string[];
  fps?: number;
  palette?: string[];
}

export interface AnimationKeyframe {
  at: number;
  state: Record<string, number | string | boolean>;
  assetFrameIndex?: number;
}

export interface ScreenAnimation {
  id: string;
  targetElementId: string;
  kind: "frame-sequence" | "property";
  frames: AnimationKeyframe[];
  loop?: boolean;
  easing?: "linear" | "step" | "ease-in" | "ease-out" | "ease-in-out";
}

export interface ScreenSubmenu {
  id: string;
  label: string;
  screenId: string;
  iconAssetId?: string;
}

export interface ScreenDefinition {
  id: string;
  name: string;
  description?: string;
  elements: ScreenElement[];
  flows?: ScreenFlow[];
  assets?: ScreenGraphicAsset[];
  animations?: ScreenAnimation[];
  submenus?: ScreenSubmenu[];
}

export interface ScreenDataset {
  screens: ScreenDefinition[];
  theme: ThemeTokens;
}
