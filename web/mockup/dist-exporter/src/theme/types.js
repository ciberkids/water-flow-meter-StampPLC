export function cloneTheme(base) {
    return {
        name: base.name,
        colors: { ...base.colors },
        typography: { ...base.typography },
        animation: { ...base.animation }
    };
}
//# sourceMappingURL=types.js.map