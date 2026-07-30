import { defineConfig } from "vitest/config";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  test: {
    environment: "node",
    // tools/exporter/__tests__ are written against node:test and are run by
    // `npm run test:exporter` (node --test over the compiled dist-exporter
    // output, where their fixture paths resolve). Globbing them here made
    // vitest report six "0 test" file failures, which hid the genuine
    // assertion failures in src/.
    include: ["src/**/*.test.ts", "src/**/*.test.tsx", "src/**/*.test.mts", "src/**/*.test.mjs"],
    globals: false
  }
});
