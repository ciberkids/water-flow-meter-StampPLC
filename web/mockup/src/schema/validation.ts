import Ajv from "ajv";
import type { ErrorObject } from "ajv";
import addFormats from "ajv-formats";
import type { ScreenDataset } from "../types";
import type { ThemeTokens } from "../theme/types";
import { datasetSchema, themeTokensSchema } from "../../shared/schemaDefinitions";

const ajv = new Ajv({
  allErrors: true,
  strict: true,
  strictSchema: true
});
addFormats(ajv);

const datasetValidator = ajv.compile<ScreenDataset>(datasetSchema);
const themeValidator = ajv.compile<ThemeTokens>(themeTokensSchema);

export class SchemaValidationError extends Error {
  constructor(message: string, readonly issues: string[]) {
    super(message);
    this.name = "SchemaValidationError";
  }
}

function toMessages(errors: ErrorObject[] | null | undefined): string[] {
  if (!errors) {
    return [];
  }
  return errors.map((error) => {
    const path = error.instancePath || error.schemaPath || "";
    const msg = error.message ?? "schema validation error";
    return `${path}: ${msg}`;
  });
}

export function validateDataset(dataset: unknown): ScreenDataset {
  if (datasetValidator(dataset)) {
    return dataset as ScreenDataset;
  }
  throw new SchemaValidationError(
    "Screen dataset failed validation",
    toMessages(datasetValidator.errors)
  );
}

export function validateTheme(theme: unknown): ThemeTokens {
  if (themeValidator(theme)) {
    return theme as ThemeTokens;
  }
  throw new SchemaValidationError(
    "Theme tokens failed validation",
    toMessages(themeValidator.errors)
  );
}
