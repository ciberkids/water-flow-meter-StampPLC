import type { ThemeTokens } from "./theme/types.js";

export type ElementKind = "text" | "value" | "badge" | "box" | "icon" | "animation" | "scrollbar";
export type DisplayOrientation = "portrait" | "landscape";

export type ElementMetadataValue = string | number | boolean;

export interface ScreenElementMetadata {
  assetId?: string;
  autoScrollIndex?: boolean;
  [key: string]: ElementMetadataValue | undefined;
}

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
  binding?: string;
  metadata?: ScreenElementMetadata;
}

export type ButtonName = "up" | "down" | "enter";
export type ButtonGesture = "short" | "long" | "hold";

export interface ButtonFlowTrigger {
  type: "button";
  button: ButtonName;
  gesture?: ButtonGesture;
}

export interface TimeoutFlowTrigger {
  type: "timeout";
  durationMs: number;
}

export interface DataFlowTrigger {
  type: "data";
  source: string;
  condition: string;
}

export type FlowTrigger = ButtonFlowTrigger | TimeoutFlowTrigger | DataFlowTrigger;

export interface ScreenFlow {
  id: string;
  label: string;
  targetScreenId?: string;
  trigger: FlowTrigger;
  guard?: string;
  actionId?: string;
  actionParams?: Record<string, string | number | boolean>;
}

export interface ScreenGraphicAsset {
  id: string;
  type: "svg-sequence" | "bitmap" | "icon";
  source: string;
  frames?: string[];
  fps?: number;
  palette?: string[];
  embeddedFrames?: string[];
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
