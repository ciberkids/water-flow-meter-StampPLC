import themeTokens from "../data/themeTokens.json" with { type: "json" };
import { ThemeTokens } from "./types.js";

export const defaultTheme: ThemeTokens = themeTokens as ThemeTokens;
