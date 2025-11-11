import type {
  ScreenDataset,
  ScreenDefinition,
  ScreenElement,
  ScreenFlow,
  ScreenGraphicAsset,
  ScreenAnimation,
  ScreenSubmenu
} from "../../src/types.js";
import type { ThemeTokens } from "../../src/theme/types.js";
import {
  type ExportIR,
  type IRElementKind,
  type IRNumberColor,
  type IRScreen,
  type IRScreenElement,
  type ValidatedInput
} from "./types.js";

const hexColorMatcher = /^#([0-9a-fA-F]{6})$/;
const rgbaColorMatcher =
  /^rgba\(\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(\d{1,3})\s*,\s*(0|0?\.\d+|1(\.0+)?)\s*\)$/;

function clampByte(value: number): number {
  if (Number.isNaN(value)) {
    return 0;
  }
  return Math.min(255, Math.max(0, Math.round(value)));
}

export function parseColorToArgb8888(color: string): IRNumberColor {
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
    const argb =
      ((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
    return { source: color, argb8888: argb >>> 0 };
  }

  throw new Error(`Unsupported color format: ${color}`);
}

function normaliseTextPayload(element: ScreenElement): IRElementKind {
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
      return { type: "icon", payload: { assetId: element.content } };
    default: {
      const exhaustive: never = element.kind;
      return exhaustive;
    }
  }
}

function convertElement(element: ScreenElement): IRScreenElement {
  const kind = normaliseTextPayload(element);
  const base: IRScreenElement = {
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
  return base;
}

function convertScreen(
  screen: ScreenDefinition
): IRScreen {
  const flows: ScreenFlow[] = screen.flows ?? [];
  const assets: ScreenGraphicAsset[] = screen.assets ?? [];
  const animations: ScreenAnimation[] = screen.animations ?? [];
  const submenus: ScreenSubmenu[] = screen.submenus ?? [];

  return {
    id: screen.id,
    name: screen.name,
    description: screen.description,
    elements: screen.elements.map(convertElement),
    flows: [...flows],
    assets: [...assets],
    animations: [...animations],
    submenus: [...submenus]
  };
}

function transformTheme(theme: ThemeTokens) {
  const colors: Record<string, IRNumberColor> = {};
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

export function buildIntermediateRepresentation(dataset: ScreenDataset, theme: ThemeTokens): ExportIR {
  const generatedAt = new Date().toISOString();
  const screens = dataset.screens.map(convertScreen);
  const elementCount = screens.reduce(
    (acc, screen) => acc + screen.elements.length,
    0
  );
  return {
    generatedAt,
    screenCount: screens.length,
    elementCount,
    dataset: screens,
    theme: transformTheme(theme)
  };
}

export function createValidatedInput(dataset: ScreenDataset, theme: ThemeTokens): ValidatedInput {
  return { dataset, theme };
}
