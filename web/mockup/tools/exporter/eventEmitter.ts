import { ExportIR, IRScreenEvent } from "./types.js";

function escapeStringLiteral(value: string): string {
    return value
        .replace(/\\/g, "\\\\")
        .replace(/"/g, '\\"')
        .replace(/\n/g, "\\n");
}

function sanitiseIdentifier(...parts: string[]): string {
    const raw = parts.join("_");
    const cleaned = raw
        .replace(/[^A-Za-z0-9_]+/g, "_")
        .replace(/_{2,}/g, "_")
        .replace(/^_+/, "")
        .replace(/_+$/, "")
        .toLowerCase();
    return cleaned || "unnamed";
}

function camelCase(value: string): string {
    return value
        .split(/[_\W]+/)
        .filter(Boolean)
        .map((segment) => segment[0].toUpperCase() + segment.slice(1))
        .join("");
}

export function emitUiEvents(ir: ExportIR): string {
    const lines: string[] = [];

    lines.push('#include "GeneratedUi.h"');
    lines.push('#include "FirmwareAction.h" // User must implement this');
    lines.push('#include "ScreenManager.h" // User must implement this');
    lines.push("");
    lines.push("namespace ui_exporter {");
    lines.push("");

    // Helper to dispatch actions
    lines.push("void DispatchAction(const char* actionId) {");
    lines.push("    if (actionId == nullptr) return;");
    lines.push("    // Firmware implementation hook");
    lines.push("    ManifestAction(actionId);");
    lines.push("}");
    lines.push("");

    // Helper to load screens
    lines.push("void RequestScreenLoad(const char* screenId) {");
    lines.push("    if (screenId == nullptr) return;");
    lines.push("    ScreenManager::LoadScreen(screenId);");
    lines.push("}");
    lines.push("");

    // Generate event registration for each screen
    ir.dataset.forEach((screen, screenIndex) => {
        const screenIdSlug = sanitiseIdentifier(screen.id || `screen_${screenIndex}`);
        const screenLabel = camelCase(screenIdSlug);
        const functionName = `RegisterEvents_${screenLabel}`;

        lines.push(`void ${functionName}() {`);

        if (screen.events.length === 0) {
            lines.push("    // No events defined for this screen");
        } else {
            screen.events.forEach((event) => {
                const trigger = event.trigger; // e.g. "Button A Click" -> needs mapping to firmware concept
                // For now, let's assume raw string matching or a simple mapping.
                // In a real scenario, this trigger string needs to map to a specific hardware set up.
                // Let's generate a comment for now and a hypothetical registration.

                const actionLiteral = event.actionId ? `"${escapeStringLiteral(event.actionId)}"` : "nullptr";
                const targetLiteral = event.targetScreenId
                    ? `"${escapeStringLiteral(event.targetScreenId)}"`
                    : "nullptr";

                lines.push(`    // Event: ${trigger}`);
                lines.push(`    ScreenManager::RegisterEvent("${escapeStringLiteral(trigger)}", []() {`);
                if (event.actionId) {
                    lines.push(`        DispatchAction(${actionLiteral});`);
                }
                if (event.targetScreenId) {
                    lines.push(`        RequestScreenLoad(${targetLiteral});`);
                }
                lines.push("    });");
            });
        }
        lines.push("}");
        lines.push("");
    });

    // Master registration function
    lines.push("void RegisterAllEvents(const char* screenId) {");
    ir.dataset.forEach((screen, screenIndex) => {
        const screenIdSlug = sanitiseIdentifier(screen.id || `screen_${screenIndex}`);
        const screenLabel = camelCase(screenIdSlug);
        lines.push(`    if (strcmp(screenId, "${escapeStringLiteral(screen.id)}") == 0) {`);
        lines.push(`        RegisterEvents_${screenLabel}();`);
        lines.push("        return;");
        lines.push("    }");
    });
    lines.push("}");

    lines.push("");
    lines.push("} // namespace ui_exporter");

    return lines.join("\n");
}
