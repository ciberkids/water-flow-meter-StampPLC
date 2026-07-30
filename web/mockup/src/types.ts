import type { ThemeTokens } from "./theme/types.js";

/**
 * Element kinds the design tool can author.
 *
 * "scrollbar" renders a level-position indicator: the firmware derives the step
 * count and current step from the active navigation level, so it carries no
 * binding.
 *
 * "animation" (SVG frame sequences) was dropped for now — it was half-removed
 * before, breaking the build while leaving the feature in place. It is recoverable
 * from git history when we come back to it.
 */
export type ElementKind = "text" | "value" | "badge" | "box" | "icon" | "scrollbar";

/** Element kinds ui_exporter::ElementType and UiRenderer can actually draw. */
export const FIRMWARE_RENDERABLE_KINDS = [
  "text",
  "value",
  "badge",
  "box",
  "icon",
  "scrollbar"
] as const;
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
  /**
   * Catalogue value this element displays. The single binding field: there used
   * to also be `dataSourceId`, which the design tool wrote and the mockup read,
   * while the exporter emitted firmware bindings from `binding` only — so
   * anything bound through the UI rendered in the mockup and was silently
   * dropped from the firmware output.
   */
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

/**
 * Two distinct behaviours share this trigger, distinguished by `holdButton`:
 *  - `holdButton: "enter"` — a hold countdown. Requires ENTER held for the whole
 *    duration and aborts on release. Used by confirm screens.
 *  - `holdButton: null`/absent — an auto timeout. Fires regardless of buttons.
 *    Used by acknowledgement toasts.
 * Without the discriminator the firmware treats every timeout as a hold, so a 2 s
 * toast would need ENTER held to dismiss — the opposite of intended.
 */
export interface TimeoutFlowTrigger {
  type: "timeout";
  durationMs: number;
  /** "enter" = hold countdown. Absent = auto timeout that fires regardless. */
  holdButton?: ButtonName;
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
