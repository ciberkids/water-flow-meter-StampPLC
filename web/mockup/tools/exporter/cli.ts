#!/usr/bin/env node

import { fileURLToPath } from "node:url";
import path from "node:path";
import fs from "node:fs/promises";
import { parseArgs } from "node:util";
import { spawn } from "node:child_process";
import { ExportValidationError } from "./schema.js";
import { emitCpp } from "./cppEmitter.js";
import { runDatasetGates } from "./exportGates.js";
import { buildPack } from "./packExport.js";
import { readFirmwarePackLimits } from "./packRules.js";
import type {
  AutomationCheck,
  BackupSummary,
  ExportIR,
  FirmwareManifest,
  ManifestSummary,
  ValidationReport
} from "./types.js";
import type { ScreenDataset } from "../../src/types.js";

interface CliOptions {
  screens: string;
  theme?: string;
  out: string;
  dryRun: boolean;
  manifest?: string;
  allowMissingToolchain: boolean;
  /**
   * `--emit-pack <path>`: build a `.uipack` for the SD card INSTEAD of the C++ tables (N-g,
   * Loadable_UI_Menu_Packs §5.6). Nothing under the firmware tree is touched and nothing is compiled.
   */
  emitPack?: string;
  /**
   * `--pack-label <text>`: the header label, refused when it does not fit. Defaults to the file name
   * minus `.uipack`, cut to the header field.
   */
  packLabel?: string;
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

  // NOTE: emitUiEvents/emitUiBindings are deliberately NOT written into the
  // firmware tree. They emit per-screen UpdateValues_*/RegisterEvents_* functions
  // against headers that do not exist (FirmwareAction.h, ScreenManager.h,
  // FirmwareValues.h) and duplicate the runtime mechanism the firmware actually
  // uses (ui/core/ui_bindings.cpp resolving bindingId against UiRenderContext,
  // ui/core/ui_actions.cpp dispatching actionId). Writing them here put
  // uncompilable translation units into src/ui/generated/.

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
  // The manifest is the firmware's declared action/value vocabulary. It defaults
  // to the checked-in copy so binding coverage is always enforced; passing an
  // empty --manifest explicitly is the only way to skip it.
  const manifestDefaultPath = path.join(workspaceRoot, "src", "data", "actionManifest.json");
  const manifestDefault = path.relative(projectRoot, manifestDefaultPath);

  const { values } = parseArgs({
    options: {
      screens: { type: "string", default: screensDefault },
      theme: { type: "string", default: themeDefault },
      out: { type: "string", default: outDefault },
      "dry-run": { type: "boolean", default: false },
      manifest: { type: "string", default: manifestDefault },
      "allow-missing-toolchain": { type: "boolean", default: false },
      // No default: its presence is what selects the pack path. `npm run export:pack` passes
      // web/mockup/dist-pack/default.uipack, and a second --emit-pack after `--` replaces it, because
      // parseArgs keeps the last value of a repeated option.
      "emit-pack": { type: "string" },
      "pack-label": { type: "string" }
    }
  });

  return {
    screens: resolvePath(projectRoot, values.screens as string),
    theme: values.theme ? resolvePath(projectRoot, values.theme as string) : undefined,
    out: resolvePath(projectRoot, values.out as string),
    dryRun: Boolean(values["dry-run"]),
    manifest: (values.manifest as string) ? resolvePath(projectRoot, values.manifest as string) : undefined,
    allowMissingToolchain: Boolean(values["allow-missing-toolchain"]),
    // Resolved against the repository root like every other path here, so `--emit-pack` and
    // `--screens` mean the same thing by a relative path.
    // PRESENCE selects the pack path, not truthiness: an empty `--emit-pack ""` must not fall through
    // to export:firmware, which rewrites the firmware tree and compiles it. An empty path resolves to
    // the repository root, whose name fails the file name rule, so it is refused like any other.
    emitPack:
      values["emit-pack"] !== undefined
        ? resolvePath(projectRoot, values["emit-pack"] as string)
        : undefined,
    packLabel: values["pack-label"] as string | undefined
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

/** Firmware build image produced by `podman build -t stampplc-fw .`. */
const kFirmwareImage = "stampplc-fw";

/**
 * Probes whether a container runtime has the firmware image locally.
 *
 * This replaces matching the runtime's error prose. That approach failed on a GitHub
 * runner: podman had no local image, so it tried to PULL one and returned "requested
 * access to the resource is denied" / "StatusCode: 404" — wording the regex did not
 * anticipate. The export therefore reported a compile FAILURE for a runner that simply
 * was not available, and restored the previous assets from backup.
 *
 * An explicit existence check is deterministic and needs no knowledge of how each
 * runtime phrases a miss.
 */
async function hasLocalImage(runtime: string, image: string, cwd: string): Promise<boolean> {
  const probes: Record<string, string[]> = {
    podman: ["image", "exists", image],
    docker: ["image", "inspect", image]
  };
  const args = probes[runtime];
  if (!args) {
    return true;  // Not a container runtime; nothing to probe.
  }
  try {
    const result = await runCommand(runtime, args, cwd);
    return result.code === 0;
  } catch {
    return false;  // The runtime itself is not installed.
  }
}

/**
 * Ways to compile-check the generated assets, in preference order: a local
 * PlatformIO install first, then the containerised toolchain. The container
 * fallback matters because without it a machine with no local `pio` can only
 * ever waive this check — which is how unverified output shipped before.
 */
function compileRunners(firmwareDir: string): Array<{ command: string; args: string[] }> {
  const pioArgs = ["run", "-e", "m5stack-stamplc"];
  const mount = `${firmwareDir}:/workspace:Z`;
  return [
    { command: "platformio", args: pioArgs },
    { command: "pio", args: pioArgs },
    {
      command: "podman",
      args: ["run", "--rm", "-v", mount, "-w", "/workspace", kFirmwareImage, "pio", ...pioArgs]
    },
    {
      command: "docker",
      args: ["run", "--rm", "-v", mount, "-w", "/workspace", kFirmwareImage, "pio", ...pioArgs]
    }
  ];
}

async function runPlatformioCheck(
  projectRoot: string,
  allowMissingToolchain: boolean
): Promise<AutomationCheck> {
  const firmwareDir = path.join(projectRoot, "Water-Flow-Meter-PlatformIO");
  const candidates = compileRunners(firmwareDir);
  let missingExecutables = 0;

  for (const runner of candidates) {
    try {
      // A container runtime with no local image is an unavailable runner, not a compile
      // failure. Established before running anything, so the distinction never depends
      // on how the runtime words its error.
      if (!(await hasLocalImage(runner.command, kFirmwareImage, firmwareDir))) {
        missingExecutables += 1;
        continue;
      }

      const start = process.hrtime.bigint();
      const result = await runCommand(runner.command, runner.args, firmwareDir);
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

  // A missing toolchain used to degrade to "warning" while the overall export
  // still reported status "ok" — that is how an uncompilable GeneratedUi.h
  // shipped. Unverifiable output is now a failure unless explicitly waived.
  const baseMessage =
    "No usable firmware toolchain found, so the generated assets could not be compile-checked. " +
    "Tried: platformio, pio, and the containerised build " +
    `(podman/docker run ${kFirmwareImage}).`;
  const remedy =
    "Install PlatformIO, or build the firmware image once with: " +
    `cd Water-Flow-Meter-PlatformIO && podman build -t ${kFirmwareImage} .`;

  if (allowMissingToolchain) {
    return {
      id: "platformio-compile",
      title: "PlatformIO compile check",
      status: "warning",
      message: `${baseMessage} Waived via --allow-missing-toolchain.`,
      command: "platformio run -e m5stack-stamplc",
      details: `The exported C++ has NOT been verified to compile. ${remedy}`
    };
  }

  return {
    id: "platformio-compile",
    title: "PlatformIO compile check",
    status: "fail",
    message: `${baseMessage} Pass --allow-missing-toolchain to export without verification.`,
    command: "platformio run -e m5stack-stamplc",
    details: remedy
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

/**
 * `--emit-pack`: the dataset has passed every gate `export:firmware` runs; now build the pack, refuse it
 * if the device would not load it, and only then write it.
 *
 * The limits come from the firmware headers on every run (packRules.ts). A header that cannot be read
 * throws, which `run()` reports as an error with exit code 1 — the export stops rather than falling back
 * to a remembered number.
 */
async function writePack(
  options: CliOptions,
  projectRoot: string,
  dataset: ScreenDataset,
  ir: ExportIR,
  manifest: FirmwareManifest | null,
  manifestSummary: ManifestSummary,
  validationReport: ValidationReport
) {
  const outPath = options.emitPack as string;
  const limits = await readFirmwarePackLimits(projectRoot);
  const fileName = path.basename(outPath);
  // The theme comes from the IR, which the gates built from `--theme` when one was given, so this
  // command and export:firmware draw one menu from one set of inputs (§5.2). It used to come from the
  // raw dataset: `--theme` was accepted and ignored, and the pack's bytes were identical without it.
  const outcome = buildPack({
    dataset,
    theme: ir.theme,
    manifest,
    fileName,
    label: options.packLabel,
    limits
  });

  for (const check of outcome.checks) {
    validationReport.checks.push(check);
    if (check.status === "fail") {
      validationReport.status = "fail";
      validationReport.issues.push(check.message);
    }
    validationReport.log += `\n[${check.status.toUpperCase()}] ${check.title} — ${check.message}`;
  }
  if (outcome.status === "fail" || !outcome.pack) {
    throw new ExportValidationError("Pack validation failed", validationReport.issues, validationReport);
  }

  if (!options.dryRun) {
    await fs.mkdir(path.dirname(outPath), { recursive: true });
    await fs.writeFile(outPath, outcome.pack.bytes);
  }

  const warnings = validationReport.checks
    .filter((check) => check.status === "warning")
    .map((check) => `${check.title}: ${check.message}`);

  console.log(
    JSON.stringify(
      {
        status: warnings.length > 0 ? "ok-with-warnings" : "ok",
        summary: {
          pack: options.dryRun ? "(dry-run)" : outPath,
          fileName,
          bytes: outcome.pack.bytes.length,
          maxBytes: limits.maxPackBytes,
          screens: outcome.pack.screenCount,
          label: outcome.label,
          catalogueAbi: outcome.catalogueAbi,
          copyTo: `${limits.cardDirectory}/${fileName}`,
          limitsReadFrom: limits.sources
        },
        warnings,
        validation: validationReport,
        manifest: manifestSummary,
        // Stated, not skipped silently and not reported as a pass: the compile gate proves generated C++
        // builds, and a pack has none. What a pack must satisfy instead is §3.3, which the firmware checks
        // when it loads the file and the host round trip checks against this command's own output.
        compilation: {
          status: "not-applicable",
          message:
            "A menu pack is data the firmware validates when it loads it (Loadable_UI_Menu_Packs §3.3); " +
            "there is no C++ to compile."
        }
      },
      null,
      2
    )
  );
}

async function run() {
  const currentDir = path.dirname(fileURLToPath(import.meta.url));
  const workspaceRoot = await resolveWorkspaceRoot(currentDir);
  const projectRoot = path.resolve(workspaceRoot, "..", "..");
  const options = parseCliArgs(projectRoot, workspaceRoot);

  try {
    const gates = await runDatasetGates({
      projectRoot,
      screensPath: options.screens,
      themePath: options.theme,
      manifestPath: options.manifest
    });

    // Block export if manifest was explicitly provided but failed to load.
    if (gates.kind === "manifest-invalid") {
      console.error(
        JSON.stringify(
          {
            status: "validation-error",
            message: `Manifest is invalid: ${gates.manifestSummary.error}`,
            manifest: gates.manifestSummary
          },
          null,
          2
        )
      );
      process.exitCode = 2;
      return;
    }
    const { ir, manifestSummary, validationReport } = gates;

    if (options.emitPack) {
      await writePack(options, projectRoot, gates.dataset, ir, gates.manifest, manifestSummary, validationReport);
      return;
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
      compilationReport = await runPlatformioCheck(projectRoot, options.allowMissingToolchain);
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

    // "ok" means the output was verified. Anything the pipeline could not verify
    // (unwaived compile check, validation warnings) must not read as a clean pass.
    const warnings = [
      ...validationReport.checks.filter((check) => check.status === "warning"),
      ...(compilationReport.status === "warning" ? [compilationReport] : [])
    ].map((check) => `${check.title}: ${check.message}`);

    console.log(
      JSON.stringify(
        {
          status: warnings.length > 0 ? "ok-with-warnings" : "ok",
          summary,
          warnings,
          validation: validationReport,
          manifest: manifestSummary,
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
