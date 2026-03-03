import { ExportIR } from "./types.js";

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

export function emitUiBindings(ir: ExportIR): { header: string; source: string } {
    const headerLines: string[] = [];
    const sourceLines: string[] = [];

    // HEADER GENERATION
    headerLines.push('#pragma once');
    headerLines.push('');
    headerLines.push('namespace ui_exporter {');
    headerLines.push('');
    headerLines.push('// Updates all bound values for the currently active screen');
    headerLines.push('void UpdateScreenValues(const char* screenId);');
    headerLines.push('');
    headerLines.push('} // namespace ui_exporter');

    // SOURCE GENERATION
    sourceLines.push('#include "ui_bindings.h"');
    sourceLines.push('#include "GeneratedUi.h"');
    sourceLines.push('#include "FirmwareValues.h" // User must implement GetFirmwareValue(id)');
    sourceLines.push('#include "ScreenManager.h" // User must implement UpdateElementText(id, value)');
    sourceLines.push('#include <cstring>');
    sourceLines.push('#include <cstdio>');
    sourceLines.push('');
    sourceLines.push('namespace ui_exporter {');
    sourceLines.push('');

    ir.dataset.forEach((screen, screenIndex) => {
        const screenIdSlug = sanitiseIdentifier(screen.id || `screen_${screenIndex}`);
        const screenLabel = camelCase(screenIdSlug);
        const functionName = `UpdateValues_${screenLabel}`;

        // Find elements with bindings
        const boundElements = screen.elements.filter(e => e.dataSourceId);

        if (boundElements.length > 0) {
            sourceLines.push(`void ${functionName}() {`);
            sourceLines.push('    char buffer[64];');

            boundElements.forEach(element => {
                const dataSource = element.dataSourceId || "";
                // We assume GetFirmwareValue returns a string or we format it here.
                // For simplicity, let's assume a generic GetFirmwareValueString(id, buffer, len)
                sourceLines.push(`    // Element: ${element.id} -> ${dataSource}`);
                sourceLines.push(`    if (GetFirmwareValueString("${escapeStringLiteral(dataSource)}", buffer, sizeof(buffer))) {`);
                sourceLines.push(`        ScreenManager::UpdateElementText("${escapeStringLiteral(element.id)}", buffer);`);
                sourceLines.push('    }');
            });
            sourceLines.push('}');
        } else {
            sourceLines.push(`void ${functionName}() {`);
            sourceLines.push('    // No bound values on this screen');
            sourceLines.push('}');
        }
        sourceLines.push('');
    });

    sourceLines.push('void UpdateScreenValues(const char* screenId) {');
    ir.dataset.forEach((screen, screenIndex) => {
        const screenIdSlug = sanitiseIdentifier(screen.id || `screen_${screenIndex}`);
        const screenLabel = camelCase(screenIdSlug);
        sourceLines.push(`    if (strcmp(screenId, "${escapeStringLiteral(screen.id)}") == 0) {`);
        sourceLines.push(`        UpdateValues_${screenLabel}();`);
        sourceLines.push('        return;');
        sourceLines.push('    }');
    });
    sourceLines.push('}');

    sourceLines.push('');
    sourceLines.push('} // namespace ui_exporter');

    return {
        header: headerLines.join('\n'),
        source: sourceLines.join('\n')
    };
}
