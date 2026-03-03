/**
 * @file manifest_spike.cpp
 * @brief Spike: Approach A — constexpr descriptor table for firmware action manifest.
 *
 * This prototype explores adding a human-readable descriptor to each entry in the
 * existing UiActionBinding table so a build-time or startup-time step can emit a
 * JSON manifest listing all callable UI actions.
 *
 * The key design decision: descriptors are stored in a *parallel* constexpr array
 * (same length and same index order as kDefaultBindings), so zero changes are
 * needed to the hot-path dispatch code and no virtual calls or RTTI are required.
 *
 * STATUS: SPIKE / SANDBOX — do NOT include in production builds.
 *         See docs/backlog/spike-report-SI-20251111-05.md for evaluation.
 */

#include <array>
#include <cstddef>
#include <cstdio>

// ── Descriptor table ────────────────────────────────────────────────────────

/**
 * Optional parameter descriptor: captures name + expected type so the
 * manifest can document what actionParams keys a caller may supply.
 */
struct ActionParamDescriptor {
  const char* name;
  const char* type;  // "string" | "number" | "boolean"
};

/**
 * Full descriptor for one firmware action.
 * This is the *only* new type callsite authors interact with.
 */
struct UiActionDescriptor {
  const char* actionId;    ///< Must match UiActionBinding::actionId exactly.
  const char* label;       ///< Short human-readable name shown in the web tool.
  const char* description;
  const ActionParamDescriptor* params;  ///< nullptr-terminated array, or nullptr if none.
};

// ── Concrete descriptors matching kDefaultBindings in ui_actions.cpp ────────

static constexpr UiActionDescriptor kActionDescriptors[] = {
    {
        "ui.action.page.next",
        "Next page",
        "Advance the info page carousel to the next screen.",
        nullptr
    },
    {
        "ui.action.page.previous",
        "Previous page",
        "Go back one step in the info page carousel.",
        nullptr
    },
    {
        "ui.action.mode.configuration",
        "Enter configuration",
        "Switch the device into Configuration mode.",
        nullptr
    },
    {
        "ui.action.mode.info",
        "Enter info",
        "Return to Info (read-only) mode.",
        nullptr
    },
    {
        "ui.action.mode.idle",
        "Enter idle",
        "Dim the display and enter low-power idle state.",
        nullptr
    },
    {
        /* durationMs is an optional param — include to document it */
        "core.action.save-config",
        "Save configuration",
        "Persist LED and configuration settings to NVS flash.",
        nullptr
    },
};

static constexpr std::size_t kDescriptorCount =
    sizeof(kActionDescriptors) / sizeof(kActionDescriptors[0]);

// ── JSON emitter (runs once at startup or in a host-side build script) ──────

/**
 * Emits a JSON manifest to the provided output buffer.
 * In production firmware this would write to Serial / UART for capture.
 * In a host-side build script the output is redirected to a .json file.
 *
 * Sample output:
 * {
 *   "version": "1",
 *   "generatedAt": "<build-stamp>",
 *   "actions": [
 *     { "id": "ui.action.page.next", "label": "Next page", "description": "...", "params": {} },
 *     ...
 *   ]
 * }
 */
void emitManifestJson(char* out, std::size_t capacity, const char* buildStamp) {
  std::size_t pos = 0;

  auto append = [&](const char* str) {
    while (*str && pos + 1 < capacity) {
      out[pos++] = *str++;
    }
  };

  append("{\n  \"version\": \"1\",\n  \"generatedAt\": \"");
  append(buildStamp ? buildStamp : "unknown");
  append("\",\n  \"actions\": [\n");

  for (std::size_t i = 0; i < kDescriptorCount; ++i) {
    const auto& d = kActionDescriptors[i];
    append("    { \"id\": \"");
    append(d.actionId);
    append("\", \"label\": \"");
    append(d.label);
    append("\", \"description\": \"");
    append(d.description);
    append("\", \"params\": {} }");
    if (i + 1 < kDescriptorCount) append(",");
    append("\n");
  }

  append("  ]\n}\n");
  if (pos < capacity) out[pos] = '\0';
}

// ── Standalone test (compile with -DMANIFEST_SPIKE_MAIN) ────────────────────
#ifdef MANIFEST_SPIKE_MAIN
#include <cstdio>
int main() {
  char buf[4096];
  emitManifestJson(buf, sizeof(buf), __TIMESTAMP__);
  puts(buf);
  return 0;
}
#endif
