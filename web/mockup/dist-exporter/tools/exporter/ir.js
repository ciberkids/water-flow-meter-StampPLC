const hexColorMatcher = /^#([0-9a-fA-F]{6})$/;
const rgbaColorMatcher = /^rgba\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(0|0?\.\d+|1(\.0+)?)\s*\)$/;
function clampByte(value) {
    if (Number.isNaN(value)) {
        return 0;
    }
    return Math.min(255, Math.max(0, Math.round(value)));
}
export function parseColorToArgb8888(color) {
    if (hexColorMatcher.test(color)) {
        const hex = hexColorMatcher.exec(color);
        if (!hex) {
            throw new Error(`Failed to parse hex color: ${color}`);
        }
        const rgb = parseInt(hex[1], 16);
        const argb = (0xff << 24) | rgb;
        return { source: color, argb8888: argb >>> 0 };
    }
    const rgbaMatches = rgbaColorMatcher.exec(color);
    if (rgbaMatches) {
        const r = clampByte(Number.parseInt(rgbaMatches[1], 10));
        const g = clampByte(Number.parseInt(rgbaMatches[2], 10));
        const b = clampByte(Number.parseInt(rgbaMatches[3], 10));
        const aFloat = Number.parseFloat(rgbaMatches[4]);
        const a = clampByte(aFloat >= 0 && aFloat <= 1 ? aFloat * 255 : aFloat);
        const argb = ((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
        return { source: color, argb8888: argb >>> 0 };
    }
    throw new Error(`Unsupported color format: ${color}`);
}
function normaliseTextPayload(element) {
    const emphasis = element.emphasis ?? "normal";
    const align = element.align ?? "left";
    const text = element.content ?? "";
    switch (element.kind) {
        case "text":
            return { type: "text", payload: { text, emphasis, align } };
        case "value":
            return { type: "value", payload: { text, emphasis, align } };
        case "badge":
            return { type: "badge", payload: { text, emphasis, align } };
        case "box":
            return {
                type: "box",
                payload: {
                    width: element.width ?? 0,
                    height: element.height ?? 0
                }
            };
        case "icon":
            return { type: "icon", payload: { assetId: element.metadata?.assetId ?? element.content } };
        case "animation":
            return { type: "animation", payload: { assetId: element.metadata?.assetId } };
        case "scrollbar":
            return {
                type: "scrollbar",
                payload: {
                    label: element.content,
                    autoIndex: Boolean(element.metadata?.autoScrollIndex)
                }
            };
        default: {
            const exhaustive = element.kind;
            return exhaustive;
        }
    }
}
function convertElement(element) {
    const kind = normaliseTextPayload(element);
    const base = {
        id: element.id,
        position: { x: element.x, y: element.y },
        kind
    };
    if (element.width !== undefined || element.height !== undefined) {
        base.size = {
            width: element.width ?? 0,
            height: element.height ?? 0
        };
    }
    if (element.binding) {
        base.binding = element.binding;
    }
    if (element.dataSourceId) {
        base.dataSourceId = element.dataSourceId;
    }
    return base;
}
function convertScreen(screen) {
    const flows = screen.flows ?? [];
    const assets = screen.assets ?? [];
    const animations = screen.animations ?? [];
    const submenus = screen.submenus ?? [];
    return {
        id: screen.id,
        name: screen.name,
        description: screen.description,
        elements: screen.elements.map(convertElement),
        events: screen.events ? [...screen.events] : [],
        flows: [...flows],
        assets: [...assets],
        animations: [...animations],
        submenus: [...submenus]
    };
}
function transformTheme(theme) {
    const colors = {};
    for (const [key, value] of Object.entries(theme.colors)) {
        colors[key] = parseColorToArgb8888(value);
    }
    return {
        name: theme.name,
        colors,
        typography: { ...theme.typography },
        animation: { ...theme.animation }
    };
}
export function buildIntermediateRepresentation(dataset, theme) {
    const generatedAt = new Date().toISOString();
    const screens = dataset.screens.map(convertScreen);
    const elementCount = screens.reduce((acc, screen) => acc + screen.elements.length, 0);
    return {
        generatedAt,
        screenCount: screens.length,
        elementCount,
        dataset: screens,
        theme: transformTheme(theme)
    };
}
export function createValidatedInput(dataset, theme) {
    return { dataset, theme };
}
//# sourceMappingURL=ir.js.map