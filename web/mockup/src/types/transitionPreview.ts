import type { LayoutReport } from "../utils/layout";

export type TransitionEffect = "slide-up" | "slide-down" | "fade" | "scale";

export interface TransitionPreviewState {
  screenId: string;
  screenName?: string;
  actionId?: string;
  actionLabel?: string;
  triggerLabel: string;
  effect: TransitionEffect;
  expiresAt: number;
  previewLayout?: LayoutReport;
}
