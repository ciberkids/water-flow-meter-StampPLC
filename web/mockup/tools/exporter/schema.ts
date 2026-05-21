import AjvModule from "ajv";
import type { ErrorObject } from "ajv";
import addFormatsModule from "ajv-formats";
import type { ScreenDataset } from "../../src/types.js";
import type { ThemeTokens } from "../../src/theme/types.js";
import { datasetSchema, themeTokensSchema } from "../../shared/schemaDefinitions.js";
import type { ValidationReport } from "./types.js";

type AjvInstance = import("ajv").default;

const AjvConstructor =
  (AjvModule as unknown as { default: new (...args: unknown[]) => AjvInstance }).default ??
  (AjvModule as unknown as new (...args: unknown[]) => AjvInstance);
const addFormats =
  (addFormatsModule as unknown as { default: (ajv: AjvInstance) => AjvInstance }).default ??
  (addFormatsModule as unknown as (ajv: AjvInstance) => AjvInstance);

const ajv = new AjvConstructor({
  allErrors: true,
  strict: true,
  strictSchema: true
}) as AjvInstance;
addFormats(ajv);

const datasetValidator = ajv.compile<ScreenDataset>(datasetSchema);
const themeValidator = ajv.compile<ThemeTokens>(themeTokensSchema);

export class ExportValidationError extends Error {
  constructor(message: string, readonly issues: string[], readonly report?: ValidationReport) {
    super(message);
    this.name = "ExportValidationError";
  }
}

function formatIssues(issues: ErrorObject[] | null | undefined): string[] {
  if (!issues) {
    return [];
  }
  return issues.map((issue) => {
    const path = issue.instancePath || issue.schemaPath || "";
    const keyword = issue.keyword;
    const msg = issue.message ?? "validation error";
    return `${path} (${keyword}): ${msg}`;
  });
}

export function ensureValidDataset(value: unknown): ScreenDataset {
  if (datasetValidator(value)) {
    return value as ScreenDataset;
  }
  throw new ExportValidationError(
    "Screen dataset validation failed",
    formatIssues(datasetValidator.errors)
  );
}

export function ensureValidTheme(value: unknown): ThemeTokens {
  if (themeValidator(value)) {
    return value as ThemeTokens;
  }
  throw new ExportValidationError(
    "Theme token validation failed",
    formatIssues(themeValidator.errors)
  );
}
