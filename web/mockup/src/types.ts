import type { ThemeTokens } from "./theme/types.js";

/**
 * Element kinds the design tool can author.
 *
 * "animation" and "scrollbar" are authorable but not yet renderable by the
 * firmware (Improvement_of_the_web_ui.md asks for both). They were previously
 * removed from this union while the editor, viewport and layout code that
 * implements them was left in place, which broke the build without removing the
 * feature. The exporter blocks any dataset that uses them — see
 * FIRMWARE_RENDERABLE_KINDS in tools/exporter/validation.ts.
 */
export type ElementKind = "text" | "value" | "badge" | "box" | "icon" | "animation" | "scrollbar";

/** Element kinds ui_exporter::ElementType and UiRenderer can actually draw. */
export const FIRMWARE_RENDERABLE_KINDS = ["text", "value", "badge", "box", "icon"] as const;
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
  dataSourceId?: string;
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

export interface ScreenEvent {
  trigger: string;
  actionId?: string;
  targetScreenId?: string;
}

export interface ScreenDefinition {
  id: string;
  name: string;
  description?: string;
  elements: ScreenElement[];
  events?: ScreenEvent[];
  flows?: ScreenFlow[];
  assets?: ScreenGraphicAsset[];
  animations?: ScreenAnimation[];
  submenus?: ScreenSubmenu[];
}

export interface ScreenDataset {
  screens: ScreenDefinition[];
  theme: ThemeTokens;
}
