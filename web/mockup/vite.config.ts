import { defineConfig, Plugin } from "vite";
import react from "@vitejs/plugin-react";
import { fileURLToPath } from "node:url";
import path from "node:path";
import { spawn } from "node:child_process";
import * as fs from "node:fs/promises";
import * as os from "node:os";
import type { IncomingMessage, ServerResponse } from "node:http";

const projectRoot = fileURLToPath(new URL(".", import.meta.url));
const tscBin = path.resolve(projectRoot, "node_modules/typescript/bin/tsc");

interface ExportResult {
  status: string;
  summary?: unknown;
  message?: string;
  issues?: unknown;
}

async function runCommand(command: string, args: string[], cwd: string) {
  const child = spawn(command, args, {
    cwd,
    stdio: ["ignore", "pipe", "pipe"],
    env: process.env
  });

  const stdoutChunks: Buffer[] = [];
  const stderrChunks: Buffer[] = [];

  if (child.stdout) {
    child.stdout.on("data", (chunk) => stdoutChunks.push(Buffer.from(chunk)));
  }

  if (child.stderr) {
    child.stderr.on("data", (chunk) => stderrChunks.push(Buffer.from(chunk)));
  }

  const exitCode: number = await new Promise((resolve, reject) => {
    child.on("error", reject);
    child.on("close", (code) => resolve(code ?? 0));
  });

  return {
    code: exitCode,
    stdout: Buffer.concat(stdoutChunks).toString("utf-8"),
    stderr: Buffer.concat(stderrChunks).toString("utf-8")
  };
}

/** Bodies are small (the whole dataset is ~200 KB); the cap stops a runaway request. */
const kMaxBodyBytes = 8 * 1024 * 1024;

async function readBody(req: IncomingMessage): Promise<string> {
  const chunks: Buffer[] = [];
  let total = 0;
  for await (const chunk of req) {
    total += (chunk as Buffer).length;
    if (total > kMaxBodyBytes) {
      throw new Error(`request body exceeds ${kMaxBodyBytes} bytes`);
    }
    chunks.push(Buffer.from(chunk as Buffer));
  }
  return Buffer.concat(chunks).toString("utf-8");
}

/**
 * Runs an export of the dataset the browser POSTed.
 *
 * Decision D4. This endpoint used to ignore the request entirely and let the CLI re-read
 * src/data/screens.json from disk, so clicking Export exported whatever was last SAVED rather
 * than what was on screen — and every validation gate therefore validated the on-disk copy.
 *
 * The dataset is written to a temp file and passed via --screens, rather than adding a stdin
 * path to the CLI: the CLI is also the command CI and the manifest tooling invoke, and it
 * should keep exactly one way in.
 */
async function handleExport(req: IncomingMessage, res: ServerResponse): Promise<void> {
  const respond = (code: number, payload: unknown) => {
    res.statusCode = code;
    res.setHeader("Content-Type", "application/json");
    res.end(JSON.stringify(payload));
  };

  let datasetPath: string | null = null;
  try {
    const raw = await readBody(req);

    if (raw.trim().length > 0) {
      let parsed: unknown;
      try {
        parsed = JSON.parse(raw);
      } catch (error) {
        // Reject rather than silently falling back to the on-disk dataset: exporting
        // something other than what was asked for is the bug this endpoint just stopped
        // having, and a quiet fallback would reintroduce it.
        return respond(400, {
          status: "error",
          message: `posted dataset is not valid JSON: ${error instanceof Error ? error.message : String(error)}`
        });
      }
      const screens = (parsed as { screens?: unknown }).screens;
      if (!Array.isArray(screens)) {
        return respond(400, {
          status: "error",
          message: "posted dataset has no screens array"
        });
      }
      // Structural check only. The exporter validates the dataset properly against the Ajv
      // schema (ensureValidDataset) and reports each failure with its screen and element, so
      // duplicating that here would mean two schemas to keep in step.
      datasetPath = path.join(os.tmpdir(), `stampplc-export-${process.pid}-${Date.now()}.json`);
      await fs.writeFile(datasetPath, raw, "utf-8");
    }

    const build = await runCommand("node", [tscBin, "--project", "tsconfig.exporter.json"], projectRoot);
    if (build.code !== 0) {
      return respond(500, { status: "error", message: build.stderr || build.stdout });
    }

    const args = ["dist-exporter/tools/exporter/cli.js"];
    if (datasetPath) {
      args.push("--screens", datasetPath);
    }
    const exportRun = await runCommand("node", args, projectRoot);
    if (exportRun.code !== 0) {
      res.statusCode = 500;
      res.setHeader("Content-Type", "application/json");
      res.end(
        exportRun.stdout || exportRun.stderr || JSON.stringify({ status: "error", message: "Export failed" })
      );
      return;
    }

    respond(200, JSON.parse(exportRun.stdout || "{}") as ExportResult);
  } catch (error) {
    respond(500, {
      status: "error",
      message: error instanceof Error ? error.message : String(error)
    });
  } finally {
    if (datasetPath) {
      await fs.rm(datasetPath, { force: true }).catch(() => {
        // A leaked temp file is not worth failing an export over.
      });
    }
  }
}

/**
 * One handler, mounted on both the dev and the preview server.
 *
 * These were two duplicated copies of the same forty lines. They had already drifted in their
 * error handling, and a fix applied to one would silently miss the other.
 */
function exporterEndpoint(): Plugin {
  const middleware = async (req: IncomingMessage, res: ServerResponse, next: () => void) => {
    if (req.method !== "POST" || req.url !== "/api/export") {
      return next();
    }
    await handleExport(req, res);
  };

  return {
    name: "stampplc-exporter-endpoint",
    configureServer(server) {
      server.middlewares.use(middleware);
    },
    configurePreviewServer(server) {
      server.middlewares.use(middleware);
    }
  };
}

export default defineConfig({
  plugins: [react(), exporterEndpoint()],
  server: {
    port: 5173
  },
  build: {
    target: "es2019"
  }
});
