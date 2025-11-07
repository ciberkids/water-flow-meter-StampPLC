import { defineConfig, Plugin } from "vite";
import react from "@vitejs/plugin-react";
import { fileURLToPath } from "node:url";
import path from "node:path";
import { spawn } from "node:child_process";

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

function exporterEndpoint(): Plugin {
  return {
    name: "stampplc-exporter-endpoint",
    configureServer(server) {
      server.middlewares.use(async (req, res, next) => {
        if (req.method !== "POST" || req.url !== "/api/export") {
          return next();
        }

        try {
          const build = await runCommand("node", [tscBin, "--project", "tsconfig.exporter.json"], projectRoot);
          if (build.code !== 0) {
            res.statusCode = 500;
            res.setHeader("Content-Type", "application/json");
            res.end(JSON.stringify({ status: "error", message: build.stderr || build.stdout }));
            return;
          }

          const exportRun = await runCommand("node", ["dist-exporter/tools/exporter/cli.js"], projectRoot);
          if (exportRun.code !== 0) {
            res.statusCode = 500;
            res.setHeader("Content-Type", "application/json");
            res.end(exportRun.stdout || exportRun.stderr || JSON.stringify({ status: "error", message: "Export failed" }));
            return;
          }

          const payload: ExportResult = JSON.parse(exportRun.stdout || "{}") as ExportResult;
          res.statusCode = 200;
          res.setHeader("Content-Type", "application/json");
          res.end(JSON.stringify(payload));
        } catch (error) {
          res.statusCode = 500;
          res.setHeader("Content-Type", "application/json");
          res.end(
            JSON.stringify({
              status: "error",
              message: error instanceof Error ? error.message : String(error)
            })
          );
        }
      });
    },
    configurePreviewServer(server) {
      server.middlewares.use(async (req, res, next) => {
        if (req.method !== "POST" || req.url !== "/api/export") {
          return next();
        }
        try {
          const build = await runCommand("node", [tscBin, "--project", "tsconfig.exporter.json"], projectRoot);
          if (build.code !== 0) {
            res.statusCode = 500;
            res.setHeader("Content-Type", "application/json");
            res.end(JSON.stringify({ status: "error", message: build.stderr || build.stdout }));
            return;
          }
          const exportRun = await runCommand("node", ["dist-exporter/tools/exporter/cli.js"], projectRoot);
          if (exportRun.code !== 0) {
            res.statusCode = 500;
            res.setHeader("Content-Type", "application/json");
            res.end(exportRun.stdout || exportRun.stderr || JSON.stringify({ status: "error", message: "Export failed" }));
            return;
          }
          const payload: ExportResult = JSON.parse(exportRun.stdout || "{}") as ExportResult;
          res.statusCode = 200;
          res.setHeader("Content-Type", "application/json");
          res.end(JSON.stringify(payload));
        } catch (error) {
          res.statusCode = 500;
          res.setHeader("Content-Type", "application/json");
          res.end(
            JSON.stringify({
              status: "error",
              message: error instanceof Error ? error.message : String(error)
            })
          );
        }
      });
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
