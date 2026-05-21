import { defineConfig } from "cypress";
import { fileURLToPath } from "node:url";
import path from "node:path";
import fs from "node:fs";
import { execSync } from "node:child_process";

const projectRoot = fileURLToPath(new URL(".", import.meta.url));
const firmwareRoot = path.resolve(projectRoot, "..", "..", "Water Flow Meter PlatformIO", "src", "ui", "generated");
const fixtureDatasetPath = path.resolve(projectRoot, "tests", "fixtures", "legacy-screens.json");
const tscBin = path.resolve(projectRoot, "node_modules/typescript/bin/tsc");
const exporterCli = path.resolve(projectRoot, "dist-exporter/tools/exporter/cli.js");

function loadJson<T>(relativePath: string): T {
  const absolute = path.resolve(projectRoot, relativePath);
  const raw = fs.readFileSync(absolute, "utf-8");
  return JSON.parse(raw) as T;
}

export default defineConfig({
  e2e: {
    baseUrl: "http://127.0.0.1:4173",
    specPattern: "cypress/e2e/**/*.cy.{ts,tsx,js,jsx}",
    supportFile: false,
    setupNodeEvents(on) {
      on("task", {
        runExportWithFixture() {
          try {
            execSync(`node "${tscBin}" --project tsconfig.exporter.json`, { cwd: projectRoot, stdio: "pipe" });
            execSync(`node "${exporterCli}" --screens "${fixtureDatasetPath}"`, { cwd: projectRoot, stdio: "pipe" });
            return { ok: true };
          } catch (error) {
            return { error: error instanceof Error ? error.message : String(error) };
          }
        },
        compareExportedDataset() {
          try {
            const dataset = loadJson<{ screens: Array<{ id: string; elements: Array<{ id: string; x: number; y: number; width?: number; height?: number; kind: string; content?: string }> }> }>(
              "tests/fixtures/legacy-screens.json"
            );
            const irRaw = fs.readFileSync(path.join(firmwareRoot, "ui_export_ir.json"), "utf-8");
            const ir = JSON.parse(irRaw) as {
              dataset: Array<{ id: string; elements: Array<{ id: string; position: { x: number; y: number }; size?: { width: number; height: number }; kind: { type: string; payload: any } }> }> };

            const mismatches: string[] = [];

            dataset.screens.forEach((screen) => {
              const irScreen = ir.dataset.find((candidate) => candidate.id === screen.id);
              if (!irScreen) {
                mismatches.push(`Missing screen in IR: ${screen.id}`);
                return;
              }
              if (irScreen.elements.length !== screen.elements.length) {
                mismatches.push(`Element count mismatch for ${screen.id}`);
              }
              screen.elements.forEach((element) => {
                const candidate = irScreen.elements.find((item) => item.id === element.id);
                if (!candidate) {
                  mismatches.push(`Missing element ${element.id} in ${screen.id}`);
                  return;
                }
                if (candidate.position.x !== element.x || candidate.position.y !== element.y) {
                  mismatches.push(`Position mismatch for ${screen.id}/${element.id}`);
                }
                if (element.width !== undefined || element.height !== undefined) {
                  const width = element.width ?? 0;
                  const height = element.height ?? 0;
                  if (!candidate.size || candidate.size.width !== width || candidate.size.height !== height) {
                    mismatches.push(`Size mismatch for ${screen.id}/${element.id}`);
                  }
                }
                if ("content" in element && typeof element.content === "string") {
                  if (candidate.kind.type !== "box" && candidate.kind.payload && candidate.kind.payload.text !== undefined) {
                    if (candidate.kind.payload.text !== element.content) {
                      mismatches.push(`Content mismatch for ${screen.id}/${element.id}`);
                    }
                  }
                }
              });
            });

            return { mismatches };
          } catch (error) {
            return { error: error instanceof Error ? error.message : String(error) };
          }
        }
      });
    }
  }
});
