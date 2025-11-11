#!/usr/bin/env node
import { fileURLToPath } from "node:url";
import path from "node:path";
import fs from "node:fs/promises";
import { parseArgs } from "node:util";
import { ensureValidDataset, ensureValidTheme, ExportValidationError } from "./schema.js";
import { buildIntermediateRepresentation } from "./ir.js";
import { emitCpp } from "./cppEmitter.js";
async function resolveWorkspaceRoot(startDir) {
    const candidates = [
        path.resolve(startDir, "..", ".."),
        path.resolve(startDir, "..", "..", "..")
    ];
    for (const candidate of candidates) {
        try {
            const packagePath = path.join(candidate, "package.json");
            const raw = await fs.readFile(packagePath, "utf-8");
            const pkg = JSON.parse(raw);
            if (pkg?.name === "stampplc-ui-mockup") {
                return candidate;
            }
        }
        catch {
            // Ignore lookup failures and continue probing.
        }
    }
    return path.resolve(startDir, "..", "..");
}
function resolvePath(base, candidate) {
    if (path.isAbsolute(candidate)) {
        return candidate;
    }
    return path.resolve(base, candidate);
}
async function readJsonFile(filePath) {
    const raw = await fs.readFile(filePath, "utf-8");
    return JSON.parse(raw);
}
async function directoryExists(dirPath) {
    try {
        const stat = await fs.stat(dirPath);
        return stat.isDirectory();
    }
    catch (error) {
        if (error.code === "ENOENT") {
            return false;
        }
        throw error;
    }
}
async function backupGeneratedAssets(generatedDir, projectRoot) {
    if (!(await directoryExists(generatedDir))) {
        return null;
    }
    const timestamp = new Date()
        .toISOString()
        .replace(/[-:]/g, "")
        .replace(/\..+$/, "")
        .replace("T", "_");
    const backupRoot = path.resolve(projectRoot, "backups/ui");
    const backupDir = path.resolve(backupRoot, timestamp);
    await fs.mkdir(backupDir, { recursive: true });
    await fs.cp(generatedDir, backupDir, { recursive: true });
    return backupDir;
}
async function prepareOutputDirectory(outDir) {
    await fs.rm(outDir, { recursive: true, force: true });
    await fs.mkdir(outDir, { recursive: true });
}
async function writeOutputs(ir, outDir, dryRun) {
    if (dryRun) {
        return;
    }
    await prepareOutputDirectory(outDir);
    const { header, source, metadataJson } = emitCpp(ir);
    const irJson = JSON.stringify(ir, null, 2);
    await Promise.all([
        fs.writeFile(path.join(outDir, "GeneratedUi.h"), header, "utf-8"),
        fs.writeFile(path.join(outDir, "GeneratedUi.cpp"), source, "utf-8"),
        fs.writeFile(path.join(outDir, "ui_export_metadata.json"), metadataJson, "utf-8"),
        fs.writeFile(path.join(outDir, "ui_export_ir.json"), irJson, "utf-8")
    ]);
}
function parseCliArgs(projectRoot, workspaceRoot) {
    const screensDefaultPath = path.join(workspaceRoot, "src", "data", "screens.json");
    const screensDefault = path.relative(projectRoot, screensDefaultPath);
    const themeDefault = "";
    const outDefaultPath = path.join(projectRoot, "Water-Flow-Meter-PlatformIO", "src", "ui", "generated");
    const outDefault = path.relative(projectRoot, outDefaultPath);
    const { values } = parseArgs({
        options: {
            screens: { type: "string", default: screensDefault },
            theme: { type: "string", default: themeDefault },
            out: { type: "string", default: outDefault },
            "dry-run": { type: "boolean", default: false }
        }
    });
    const baseDir = projectRoot;
    return {
        screens: resolvePath(projectRoot, values.screens),
        theme: values.theme ? resolvePath(projectRoot, values.theme) : undefined,
        out: resolvePath(projectRoot, values.out),
        dryRun: Boolean(values["dry-run"])
    };
}
async function run() {
    const currentDir = path.dirname(fileURLToPath(import.meta.url));
    const workspaceRoot = await resolveWorkspaceRoot(currentDir);
    const projectRoot = path.resolve(workspaceRoot, "..", "..");
    const options = parseCliArgs(projectRoot, workspaceRoot);
    try {
        const dataset = ensureValidDataset(await readJsonFile(options.screens));
        const themeTokens = options.theme
            ? ensureValidTheme(await readJsonFile(options.theme))
            : dataset.theme;
        const ir = buildIntermediateRepresentation(dataset, themeTokens);
        let backupLocation = null;
        if (!options.dryRun) {
            backupLocation = await backupGeneratedAssets(options.out, projectRoot);
        }
        await writeOutputs(ir, options.out, options.dryRun);
        const summary = {
            generatedAt: ir.generatedAt,
            screens: ir.screenCount,
            elements: ir.elementCount,
            output: options.dryRun ? "(dry-run)" : options.out,
            backup: options.dryRun ? null : backupLocation
        };
        // eslint-disable-next-line no-console
        console.log(JSON.stringify({ status: "ok", summary }, null, 2));
    }
    catch (error) {
        if (error instanceof ExportValidationError) {
            // eslint-disable-next-line no-console
            console.error(JSON.stringify({ status: "validation-error", message: error.message, issues: error.issues }, null, 2));
            process.exitCode = 2;
            return;
        }
        const message = error instanceof Error ? error.message : String(error);
        // eslint-disable-next-line no-console
        console.error(JSON.stringify({ status: "error", message }, null, 2));
        process.exitCode = 1;
    }
}
run().catch((error) => {
    const message = error instanceof Error ? error.message : String(error);
    // eslint-disable-next-line no-console
    console.error(JSON.stringify({ status: "error", message }, null, 2));
    process.exitCode = 1;
});
//# sourceMappingURL=cli.js.map