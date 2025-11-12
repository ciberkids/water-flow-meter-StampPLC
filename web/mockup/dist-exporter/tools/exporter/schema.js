import AjvModule from "ajv";
import addFormatsModule from "ajv-formats";
import { datasetSchema, themeTokensSchema } from "../../shared/schemaDefinitions.js";
const AjvConstructor = AjvModule.default ??
    AjvModule;
const addFormats = addFormatsModule.default ??
    addFormatsModule;
const ajv = new AjvConstructor({
    allErrors: true,
    strict: true,
    strictSchema: true
});
addFormats(ajv);
const datasetValidator = ajv.compile(datasetSchema);
const themeValidator = ajv.compile(themeTokensSchema);
export class ExportValidationError extends Error {
    constructor(message, issues, report) {
        super(message);
        this.issues = issues;
        this.report = report;
        this.name = "ExportValidationError";
    }
}
function formatIssues(issues) {
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
export function ensureValidDataset(value) {
    if (datasetValidator(value)) {
        return value;
    }
    throw new ExportValidationError("Screen dataset validation failed", formatIssues(datasetValidator.errors));
}
export function ensureValidTheme(value) {
    if (themeValidator(value)) {
        return value;
    }
    throw new ExportValidationError("Theme token validation failed", formatIssues(themeValidator.errors));
}
//# sourceMappingURL=schema.js.map