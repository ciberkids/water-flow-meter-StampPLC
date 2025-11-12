#!/usr/bin/env node

import { fileURLToPath } from "node:url";
import path from "node:path";
import fs from "node:fs/promises";
import { parseArgs } from "node:util";
import { spawn } from "node:child_process";
import { ensureValidDataset, ensureValidTheme, ExportValidationError } from "./schema.js";
import { buildIntermediateRepresentation } from "./ir.js";
import { emitCpp } from "./cppEmitter.js";
import { runExportValidations } from "./validation.js";
import type {
  AutomationCheck,
  BackupSummary,
  ExportIR,
  ValidationReport
} from "./types.js";

interface CliOptions {
  screens: string;
  theme?: string;
  out: string;
  dryRun: boolean;
}

interface CommandResult {
  code: number;
  stdout: string;
  stderr: string;
  command: string;
}

async function resolveWorkspaceRoot(startDir: string): Promise<string> {
  const candidates = [
    path.resolve(startDir, "..", ".."),
    path.resolve(startDir, "..", "..", "..")
  ];

  for (const candidate of candidates) {
    try {
      const packagePath = path.join(candidate, "package.json");
      const raw = await fs.readFile(packagePath, "utf-8");
      const pkg = JSON.parse(raw) as { name?: string };
      if (pkg?.name === "stampplc-ui-mockup") {
        return candidate;
      }
    } catch {
      // Ignore lookup failures and continue probing.
    }
  }

  return path.resolve(startDir, "..", "..");
}

function resolvePath(base: string, candidate: string): string {
  if (path.isAbsolute(candidate)) {
    return candidate;
  }
  return path.resolve(base, candidate);
}

async function readJsonFile<T>(filePath: string): Promise<T> {
  const raw = await fs.readFile(filePath, "utf-8");
  return JSON.parse(raw) as T;
}

async function directoryExists(dirPath: string): Promise<boolean> {
  try {
    const stat = await fs.stat(dirPath);
    return stat.isDirectory();
  } catch (error: unknown) {
    if ((error as NodeJS.ErrnoException).code === "ENOENT") {
      return false;
    }
    throw error;
  }
}

async function backupGeneratedAssets(generatedDir: string, projectRoot: string): Promise<string | null> {
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

async function restoreGeneratedAssets(sourceDir: string, destinationDir: string) {
  await fs.rm(destinationDir, { recursive: true, force: true });
  await fs.mkdir(destinationDir, { recursive: true });
  await fs.cp(sourceDir, destinationDir, { recursive: true });
}

async function prepareOutputDirectory(outDir: string) {
  await fs.rm(outDir, { recursive: true, force: true });
  await fs.mkdir(outDir, { recursive: true });
}

async function writeOutputs(ir: ExportIR, outDir: string, dryRun: boolean) {
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

function parseCliArgs(projectRoot: string, workspaceRoot: string): CliOptions {
  const screensDefaultPath = path.join(workspaceRoot, "src", "data", "screens.json");
  const screensDefault = path.relative(projectRoot, screensDefaultPath);
  const themeDefault = "";
  const outDefaultPath = path.join(
    projectRoot,
    "Water-Flow-Meter-PlatformIO",
    "src",
    "ui",
    "generated"
  );
  const outDefault = path.relative(projectRoot, outDefaultPath);

  const { values } = parseArgs({
    options: {
      screens: { type: "string", default: screensDefault },
      theme: { type: "string", default: themeDefault },
      out: { type: "string", default: outDefault },
      "dry-run": { type: "boolean", default: false }
    }
  });

  return {
    screens: resolvePath(projectRoot, values.screens as string),
    theme: values.theme ? resolvePath(projectRoot, values.theme as string) : undefined,
    out: resolvePath(projectRoot, values.out as string),
    dryRun: Boolean(values["dry-run"])
  };
}

function collectOutput(stdout: string, stderr: string): string {
  return [stdout.trim(), stderr.trim()].filter(Boolean).join("\n");
}

async function runCommand(command: string, args: string[], cwd: string): Promise<CommandResult> {
  return await new Promise<CommandResult>((resolve, reject) => {
    const child = spawn(command, args, {
      cwd,
      stdio: ["ignore", "pipe", "pipe"],
      env: process.env
    });

    const stdoutChunks: Buffer[] = [];
    const stderrChunks: Buffer[] = [];

    child.stdout?.on("data", (chunk) => stdoutChunks.push(Buffer.from(chunk)));
    child.stderr?.on("data", (chunk) => stderrChunks.push(Buffer.from(chunk)));

    child.once("error", reject);
    child.once("close", (code) => {
      resolve({
        code: code ?? 0,
        stdout: Buffer.concat(stdoutChunks).toString("utf-8"),
        stderr: Buffer.concat(stderrChunks).toString("utf-8"),
        command: [command, ...args].join(" ").trim()
      });
    });
  });
}

async function runPlatformioCheck(projectRoot: string): Promise<AutomationCheck> {
  const firmwareDir = path.join(projectRoot, "Water-Flow-Meter-PlatformIO");
  const candidates = ["platformio", "pio"];
  let missingExecutables = 0;

  for (const executable of candidates) {
    try {
      const start = process.hrtime.bigint();
      const result = await runCommand(executable, ["run", "-e", "m5stack-stamplc"], firmwareDir);
      const durationMs = Number(process.hrtime.bigint() - start) / 1_000_000;
      const log = collectOutput(result.stdout, result.stderr);
      if (result.code === 0) {
        return {
          id: "platformio-compile",
          title: "PlatformIO compile check",
          status: "pass",
          message: "Generated UI assets compile with the firmware target.",
          command: result.command,
          durationMs,
          log
        };
      }
      return {
        id: "platformio-compile",
        title: "PlatformIO compile check",
        status: "fail",
        message: "PlatformIO compile failed. See log for details.",
        command: result.command,
        durationMs,
        log: log || "(no compiler output)"
      };
    } catch (error) {
      if ((error as NodeJS.ErrnoException).code === "ENOENT") {
        missingExecutables += 1;
        continue;
      }
      throw error;
    }
  }

  const message =
    missingExecutables === candidates.length
      ? "PlatformIO CLI not found in PATH. Install platformio to enable compile checks."
      : "PlatformIO command was not available.";

  return {
    id: "platformio-compile",
    title: "PlatformIO compile check",
    status: "warning",
    message,
    command: "platformio run -e m5stack-stamplc",
    details: message
  };
}

function validationFailurePayload(report?: ValidationReport) {
  return (
    report ?? {
      status: "fail",
      checks: [],
      issues: ["Validation failed"],
      log: ""
    }
  );
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
    const validationReport = runExportValidations(dataset, ir);

    if (validationReport.status === "fail") {
      throw new ExportValidationError(
        "Post-export validation failed",
        validationReport.issues,
        validationReport
      );
    }

    let backupLocation: string | null = null;
    let backupSummary: BackupSummary = {
      attempted: !options.dryRun,
      created: false,
      location: null
    };

    if (!options.dryRun) {
      backupLocation = await backupGeneratedAssets(options.out, projectRoot);
      backupSummary = {
        attempted: true,
        created: Boolean(backupLocation),
        location: backupLocation
      };
    }

    await writeOutputs(ir, options.out, options.dryRun);

    let compilationReport: AutomationCheck;
    if (!options.dryRun) {
      compilationReport = await runPlatformioCheck(projectRoot);
      if (compilationReport.status === "fail") {
        if (backupLocation) {
          await restoreGeneratedAssets(backupLocation, options.out);
          backupSummary.restored = true;
          backupSummary.reason = "Compilation failed; restored previous export.";
        } else {
          await fs.rm(options.out, { recursive: true, force: true });
          backupSummary.reason = "Compilation failed; removed incomplete assets because no backup was available.";
        }
        console.error(
          JSON.stringify(
            {
              status: "automation-error",
              message: compilationReport.message,
              validation: validationReport,
              compilation: compilationReport,
              backup: backupSummary
            },
            null,
            2
          )
        );
        process.exitCode = 3;
        return;
      }
    } else {
      compilationReport = {
        id: "platformio-compile",
        title: "PlatformIO compile check",
        status: "warning",
        message: "Skipping compilation because --dry-run was provided.",
        command: "platformio run -e m5stack-stamplc",
        details: "Re-run without --dry-run to ensure the firmware compiles with generated assets."
      };
    }

    const summary = {
      generatedAt: ir.generatedAt,
      screens: ir.screenCount,
      elements: ir.elementCount,
      output: options.dryRun ? "(dry-run)" : options.out,
      backup: options.dryRun ? null : backupLocation
    };

    console.log(
      JSON.stringify(
        {
          status: "ok",
          summary,
          validation: validationReport,
          backup: backupSummary,
          compilation: compilationReport
        },
        null,
        2
      )
    );
  } catch (error: unknown) {
    if (error instanceof ExportValidationError) {
      console.error(
        JSON.stringify(
          {
            status: "validation-error",
            message: error.message,
            issues: error.issues,
            validation: validationFailurePayload(error.report)
          },
          null,
          2
        )
      );
      process.exitCode = 2;
      return;
    }
    const message = error instanceof Error ? error.message : String(error);
    console.error(JSON.stringify({ status: "error", message }, null, 2));
    process.exitCode = 1;
  }
}

run().catch((error) => {
  const message = error instanceof Error ? error.message : String(error);
  console.error(JSON.stringify({ status: "error", message }, null, 2));
  process.exitCode = 1;
});
