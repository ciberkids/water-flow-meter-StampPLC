/**
 * @file manifest_spike_macro.cpp
 * @brief Spike: Approach B — X-macro / registration-helper for firmware action manifest.
 *
 * This prototype collapses the handler pointer AND the JSON descriptor into a
 * single X-macro expansion so every action is defined exactly once.
 * The dispatch table and the descriptor table are both generated from the
 * same macro list — no duplication, no index drift.
 *
 * Trade-off: callers must use the macro style; mixing hand-written bindings
 * and macro-generated ones requires care.
 *
 * STATUS: SPIKE / SANDBOX — see docs/backlog/spike-report-SI-20251111-05.md.
 */

#include <array>
#include <cstddef>
#include <cstring>

// ── Forward-declare handler functions (defined elsewhere in firmware) ────────
// In production these come from ui_actions.cpp; here we stub them.

struct UiActionContextStub {};
struct FlowStub {};

static void handlePageNext(const UiActionContextStub&, const FlowStub&) {}
static void handlePagePrevious(const UiActionContextStub&, const FlowStub&) {}
static void handleEnterConfiguration(const UiActionContextStub&, const FlowStub&) {}
static void handleEnterInfo(const UiActionContextStub&, const FlowStub&) {}
static void handleEnterIdle(const UiActionContextStub&, const FlowStub&) {}
static void handleSaveConfig(const UiActionContextStub&, const FlowStub&) {}

// ── X-macro action table ─────────────────────────────────────────────────────
/**
 * Each row: ACTION(id, label, description, handler)
 * Adding a new action = one line here. Nothing else to touch.
 */
#define UI_ACTION_TABLE(ACTION)                                                 \
  ACTION("ui.action.page.next",                                                  \
         "Next page",                                                            \
         "Advance the info page carousel to the next screen.",                   \
         handlePageNext)                                                          \
  ACTION("ui.action.page.previous",                                              \
         "Previous page",                                                         \
         "Go back one step in the info page carousel.",                          \
         handlePagePrevious)                                                      \
  ACTION("ui.action.mode.configuration",                                         \
         "Enter configuration",                                                   \
         "Switch the device into Configuration mode.",                            \
         handleEnterConfiguration)                                                \
  ACTION("ui.action.mode.info",                                                   \
         "Enter info",                                                            \
         "Return to Info (read-only) mode.",                                      \
         handleEnterInfo)                                                         \
  ACTION("ui.action.mode.idle",                                                   \
         "Enter idle",                                                            \
         "Dim the display and enter low-power idle state.",                       \
         handleEnterIdle)                                                         \
  ACTION("core.action.save-config",                                               \
         "Save configuration",                                                    \
         "Persist LED and configuration settings to NVS flash.",                  \
         handleSaveConfig)

// ── Dispatch table generated from the macro ──────────────────────────────────
using ActionFn = void (*)(const UiActionContextStub&, const FlowStub&);

struct MacroBinding {
  const char* actionId;
  ActionFn    handler;
};

#define MAKE_BINDING(id, label, desc, fn) { id, fn },
static constexpr MacroBinding kMacroBindings[] = {
  UI_ACTION_TABLE(MAKE_BINDING)
};
#undef MAKE_BINDING

static constexpr std::size_t kMacroBindingCount =
    sizeof(kMacroBindings) / sizeof(kMacroBindings[0]);

// ── Descriptor table generated from the same macro ───────────────────────────
struct MacroDescriptor {
  const char* actionId;
  const char* label;
  const char* description;
};

#define MAKE_DESCRIPTOR(id, label, desc, fn) { id, label, desc },
static constexpr MacroDescriptor kMacroDescriptors[] = {
  UI_ACTION_TABLE(MAKE_DESCRIPTOR)
};
#undef MAKE_DESCRIPTOR

static constexpr std::size_t kMacroDescriptorCount =
    sizeof(kMacroDescriptors) / sizeof(kMacroDescriptors[0]);

// ── Dispatch function (mirrors UiActionRegistry::dispatch) ───────────────────
static bool macroDispatch(const char* actionId,
                          const UiActionContextStub& ctx,
                          const FlowStub& flow) {
  for (std::size_t i = 0; i < kMacroBindingCount; ++i) {
    if (std::strcmp(kMacroBindings[i].actionId, actionId) == 0) {
      if (kMacroBindings[i].handler) {
        kMacroBindings[i].handler(ctx, flow);
        return true;
      }
      return false;
    }
  }
  return false;
}

// ── JSON emitter (same contract as Approach A) ───────────────────────────────
void emitManifestJsonMacro(char* out, std::size_t capacity, const char* buildStamp) {
  std::size_t pos = 0;
  auto append = [&](const char* str) {
    while (*str && pos + 1 < capacity) out[pos++] = *str++;
  };

  append("{\n  \"version\": \"1\",\n  \"generatedAt\": \"");
  append(buildStamp ? buildStamp : "unknown");
  append("\",\n  \"actions\": [\n");

  for (std::size_t i = 0; i < kMacroDescriptorCount; ++i) {
    const auto& d = kMacroDescriptors[i];
    append("    { \"id\": \"");  append(d.actionId);
    append("\", \"label\": \""); append(d.label);
    append("\", \"description\": \""); append(d.description);
    append("\", \"params\": {} }");
    if (i + 1 < kMacroDescriptorCount) append(",");
    append("\n");
  }
  append("  ]\n}\n");
  if (pos < capacity) out[pos] = '\0';
}

// ── Standalone test ───────────────────────────────────────────────────────────
#ifdef MANIFEST_SPIKE_MACRO_MAIN
#include <cstdio>
int main() {
  char buf[4096];
  emitManifestJsonMacro(buf, sizeof(buf), __TIMESTAMP__);
  puts(buf);
  return 0;
}
#endif
