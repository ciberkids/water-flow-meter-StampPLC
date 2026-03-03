import type {
  ScreenAnimation,
  ScreenDataset,
  ScreenElement,
  ScreenFlow,
  ScreenSubmenu
} from "../../src/types.js";
import type { ThemeTokens } from "../../src/theme/types.js";

export type { FirmwareManifest, FirmwareAction } from "../../shared/schemaDefinitions.js";

export interface ValidatedInput {
  dataset: ScreenDataset;
  theme: ThemeTokens;
}

export interface IRNumberColor {
  source: string;
  argb8888: number;
}

export interface IRElementTextContent {
  text: string;
  emphasis: ScreenElement["emphasis"];
  align: Exclude<ScreenElement["align"], undefined> | "left";
}

export type IRElementKind =
  | { type: "text"; payload: IRElementTextContent }
  | { type: "value"; payload: IRElementTextContent }
  | { type: "badge"; payload: IRElementTextContent }
  | { type: "box"; payload: { width: number; height: number } }
  | { type: "icon"; payload: { assetId?: string } }
  | { type: "animation"; payload: { assetId?: string } }
  | { type: "scrollbar"; payload: { label?: string; autoIndex?: boolean } };

export interface IRScreenElement {
  id: string;
  position: { x: number; y: number };
  kind: IRElementKind;
  size?: { width: number; height: number };
  binding?: string;
  dataSourceId?: string;
}

export interface IRFlow extends ScreenFlow { }

export interface IRAsset {
  id: string;
  type: "svg-sequence" | "bitmap" | "icon";
  source: string;
  frames?: string[];
  fps?: number;
  palette?: string[];
  embeddedFrames?: string[];
}

export interface IRAnimation extends ScreenAnimation { }

export interface IRSubmenu extends ScreenSubmenu { }

export interface IRScreenEvent {
  trigger: string;
  actionId?: string;
  targetScreenId?: string;
}

export interface IRScreen {
  id: string;
  name: string;
  description?: string;
  elements: IRScreenElement[];
  events: IRScreenEvent[];
  flows: IRFlow[];
  assets: IRAsset[];
  animations: IRAnimation[];
  submenus: IRSubmenu[];
}

export interface IRTheme {
  name: string;
  colors: Record<string, IRNumberColor>;
  typography: ThemeTokens["typography"];
  animation: ThemeTokens["animation"];
}

export interface ExportIR {
  generatedAt: string;
  screenCount: number;
  elementCount: number;
  dataset: IRScreen[];
  theme: IRTheme;
}

export interface ExportContext extends ValidatedInput {
  projectRoot: string;
  generatedDir: string;
  backupDir: string;
  ir: ExportIR;
}

export type ValidationStatus = "pass" | "warning" | "fail";

export interface ValidationCheck {
  id: string;
  title: string;
  status: ValidationStatus;
  message: string;
  screenId?: string;
  elementId?: string;
  recommendation?: string;
}

export interface ValidationReport {
  status: ValidationStatus;
  checks: ValidationCheck[];
  issues: string[];
  log: string;
}

export interface AutomationCheck {
  id: string;
  title: string;
  status: ValidationStatus;
  message: string;
  details?: string;
  durationMs?: number;
  command?: string;
  log?: string;
}

export interface BackupSummary {
  attempted: boolean;
  created: boolean;
  location?: string | null;
  restored?: boolean;
  reason?: string;
}

/** Summary of the firmware manifest load result included in CLI JSON output. */
export interface ManifestSummary {
  status: "loaded" | "missing" | "invalid";
  path?: string;
  actionCount?: number;
  error?: string;
}
