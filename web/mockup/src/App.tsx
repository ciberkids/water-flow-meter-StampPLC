import { ChangeEvent, useCallback, useEffect, useMemo, useRef, useState } from "react";
import "./App.css";
import { DisplayViewport } from "./components/DisplayViewport";
import { ScreenSelector } from "./components/ScreenSelector";
import screensData from "./data/screens.json";
import actionManifestJson from "./data/actionManifest.json";
import {
  DisplayOrientation,
  ScreenDataset,
  ScreenDefinition,
  ScreenElement,
  ScreenFlow,
  ElementKind
} from "./types";
import { ButtonPanel } from "./components/ButtonPanel";
import { ThemeEditor } from "./components/ThemeEditor";
import { HelpPanel } from "./components/HelpPanel";
import { useTheme } from "./theme/ThemeProvider";
import { cloneTheme, type ThemeTokens } from "./theme/types";
import { defaultTheme } from "./theme/defaultTheme";
import { useSimulatedButtons } from "./hooks/useSimulatedButtons";
import { SimulatedButton, SimulatedButtonEvent } from "./types/buttonSimulation";
import { computeLayout, DISPLAY_WIDTH, DISPLAY_HEIGHT } from "./utils/layout";
import {
  clampDatasetToDisplay,
  clampCoordinate,
  clampSize,
  ClampAdjustment,
  ClampCorrection
} from "./utils/datasetClamp";
import { SchemaValidationError, validateDataset } from "./schema/validation";
import { ExporterPanel } from "./components/ExporterPanel";
import { SimulationTracePanel } from "./components/SimulationTracePanel";
import { ValuePlaceholderPanel } from "./components/ValuePlaceholderPanel";
import { SimulationTraceEntry } from "./types/simulationTrace";
import { FirmwareActionManifest, FirmwareActionDefinition } from "./types/firmwareActions";
import { TransitionEffect, TransitionPreviewState } from "./types/transitionPreview";
import { findMatchingButtonFlows } from "./utils/flowMatching";
import { ScreenHierarchyPanel } from "./components/design/ScreenHierarchyPanel";
import { buildScreenHierarchy } from "./utils/screenHierarchy";
import { DesignToolbox } from "./components/design/DesignToolbox";
import { EventBindingPanel } from "./components/design/EventBindingPanel";
import { LiveJsonEditorPanel } from "./components/design/LiveJsonEditorPanel";
import { AnimationInspector } from "./components/design/AnimationInspector";

const clamp = (value: number, min: number, max: number) =>
  Math.min(Math.max(value, min), max);

const ensureDatasetTheme = (source: ScreenDataset): ScreenDataset => ({
  ...source,
  theme: cloneTheme(source.theme)
});

const themesEqual = (a?: ThemeTokens, b?: ThemeTokens): boolean => {
  if (!a || !b) {
    return a === b;
  }
  return JSON.stringify(a) === JSON.stringify(b);
};

const createId = (prefix: string) => `${prefix}-${Math.random().toString(36).slice(2, 8)}`;
const MAX_COORD_X = DISPLAY_WIDTH;
const MAX_COORD_Y = DISPLAY_HEIGHT;
const MAX_SIZE = DISPLAY_HEIGHT;
const MAX_INPUT_LENGTH = 4;
const NUDGE_STEP = 1;

const createScreenTemplate = (name: string): ScreenDefinition => ({
  id: createId("screen"),
  name,
  description: "",
  elements: [
    {
      id: createId("element"),
      kind: "text",
      x: 4,
      y: 4,
      content: name,
      emphasis: "strong"
    }
  ],
  flows: [],
  submenus: []
});

const createElementTemplate = (kind: ElementKind, screen: ScreenDefinition): ScreenElement => {
  const offset = screen.elements.length * 12 + 20;
  const baseElement: ScreenElement = {
    id: createId("element"),
    kind,
    x: 4,
    y: offset,
    content: undefined
  };

  switch (kind) {
    case "text":
      return { ...baseElement, content: "New text", emphasis: "normal" };
    case "value":
      return { ...baseElement, content: "VALUE 00", emphasis: "strong" };
    case "box":
      return { ...baseElement, width: 60, height: 18, content: " ", kind: "box" };
    case "badge":
      return { ...baseElement, content: "Badge" };
    case "icon":
      return {
        ...baseElement,
        width: 12,
        height: 12,
        metadata: { assetId: screen.assets?.[0]?.id }
      };
    case "animation":
      return {
        ...baseElement,
        width: 50,
        height: 24,
        metadata: { assetId: screen.assets?.[0]?.id }
      };
    case "scrollbar":
      return {
        ...baseElement,
        width: 10,
        height: 80,
        metadata: { autoScrollIndex: true },
        content: "Scroll"
      };
    default:
      return baseElement;
  }
};

const fileToDataUrl = (file: File) =>
  new Promise<string>((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve(reader.result as string);
    reader.onerror = () => reject(reader.error ?? new Error("Failed to read file"));
    reader.readAsDataURL(file);
  });

const findParentScreenId = (screens: ScreenDefinition[], childId: string): string | null => {
  for (const screen of screens) {
    if (screen.submenus?.some((submenu) => submenu.screenId === childId)) {
      return screen.id;
    }
  }
  return null;
};

const prepareDataset = (source: ScreenDataset) => {
  const themed = ensureDatasetTheme(source);
  return clampDatasetToDisplay(themed);
};

const normalizeElementUpdate = (
  element: ScreenElement,
  updates: Partial<ScreenElement>
): ScreenElement => {
  const next: Partial<ScreenElement> = { ...updates };
  if (updates.x !== undefined) {
    next.x = clampCoordinate(updates.x, "x");
  }
  if (updates.y !== undefined) {
    next.y = clampCoordinate(updates.y, "y");
  }
  if (updates.width !== undefined) {
    next.width = clampSize(updates.width);
  }
  if (updates.height !== undefined) {
    next.height = clampSize(updates.height);
  }
  return { ...element, ...next };
};

type ValidationFeedback = {
  status: "idle" | "success" | "error";
  message: string;
  issues: string[];
};

type ClampNotice = {
  timestamp: number;
  context: string;
  total: number;
  samples: ClampCorrection[];
};

const formatTriggerLabel = (event: SimulatedButtonEvent): string =>
  `${event.button.toUpperCase()} • ${event.kind}`;

const deriveTransitionEffect = (event: SimulatedButtonEvent): TransitionEffect => {
  if (event.button === "down") {
    return "slide-up";
  }
  if (event.button === "up") {
    return "slide-down";
  }
  if (event.button === "enter" && event.kind === "short") {
    return "scale";
  }
  return "fade";
};

const buildClampNotice = (corrections: ClampCorrection[], context: string): ClampNotice => ({
  timestamp: Date.now(),
  context,
  total: corrections.length,
  samples: corrections.slice(0, 4)
});

export function App() {
  const datasetValidation = useMemo(() => {
    try {
      const validated = validateDataset(screensData);
      return { dataset: validated, errors: null as string[] | null };
    } catch (error) {
      const issues =
        error instanceof SchemaValidationError ? error.issues : [String(error)];
      console.error("Screen dataset validation failed", issues);
      return {
        dataset: { screens: [], theme: cloneTheme(defaultTheme) } as ScreenDataset,
        errors: issues
      };
    }
  }, []);

  const initialDatasetResult = useMemo(() => prepareDataset(datasetValidation.dataset), [datasetValidation.dataset]);
  const initialDataset = initialDatasetResult.dataset;

  const { theme, updateTheme } = useTheme();

  const initialScreens = initialDataset.screens;
  const [dataset, setDataset] = useState<ScreenDataset>(initialDataset);
  const initialManifest = actionManifestJson as FirmwareActionManifest;
  const [firmwareManifest, setFirmwareManifest] = useState<FirmwareActionManifest>(initialManifest);
  const [validationFeedback, setValidationFeedback] = useState<ValidationFeedback>(() => ({
    status: datasetValidation.errors?.length ? "error" : "success",
    message: datasetValidation.errors?.length
      ? "Dataset validation failed. Import or fix the JSON and validate again."
      : "Dataset loaded successfully.",
    issues: datasetValidation.errors ?? []
  }));
  const [clampNotice, setClampNotice] = useState<ClampNotice | null>(() =>
    initialDatasetResult.corrections.length
      ? buildClampNotice(initialDatasetResult.corrections, "loading bundled dataset")
      : null
  );
  const [selectedScreenId, setSelectedScreenId] = useState<string>(initialScreens[0]?.id ?? "");
  const screens: ScreenDefinition[] = useMemo(() => dataset.screens, [dataset]);
  const datasetSummary = useMemo(
    () => ({
      screenCount: dataset.screens.length,
      ids: dataset.screens.map((screen) => screen.id)
    }),
    [dataset]
  );
  const [selectedElementId, setSelectedElementId] = useState<string | null>(
    initialScreens[0]?.elements[0]?.id ?? null
  );
  const announceClampCorrections = useCallback(
    (corrections: ClampCorrection[], context: string) => {
      if (corrections.length === 0) {
        return;
      }
      setClampNotice(buildClampNotice(corrections, context));
    },
    []
  );
  const actionCatalog = useMemo(() => {
    const map = new Map<string, FirmwareActionDefinition>();
    firmwareManifest.actions.forEach((action) => {
      if (action?.id) {
        map.set(action.id, action);
      }
    });
    return map;
  }, [firmwareManifest]);
  const [zoom, setZoom] = useState<number>(200);
  const [showGrid, setShowGrid] = useState<boolean>(true);
  const [orientation, setOrientation] = useState<DisplayOrientation>("landscape");
  const [interactionLog, setInteractionLog] = useState<string[]>([]);
  const [traceEntries, setTraceEntries] = useState<SimulationTraceEntry[]>([]);
  const [traceFilter, setTraceFilter] = useState<string>("");
  const [transitionPreview, setTransitionPreview] = useState<TransitionPreviewState | null>(null);
  const [valueOverrides, setValueOverrides] = useState<Record<string, Record<string, string>>>({});
  const [activePanel, setActivePanel] = useState<"simulation" | "design" | "importExport" | "help">("simulation");
  const fileInputRef = useRef<HTMLInputElement | null>(null);
  const manifestInputRef = useRef<HTMLInputElement | null>(null);
  const [manifestFeedback, setManifestFeedback] = useState<{ status: "idle" | "success" | "error"; message: string }>({
    status: "idle",
    message: "Using bundled manifest."
  });
  const screenJsonDownload = useCallback(() => {
    const blob = new Blob([JSON.stringify(dataset, null, 2)], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement("a");
    anchor.href = url;
    anchor.download = "screens.json";
    anchor.click();
    URL.revokeObjectURL(url);
  }, [dataset]);

  useEffect(() => {
    if (screens.length === 0) {
      if (selectedScreenId !== "") {
        setSelectedScreenId("");
      }
      return;
    }
    if (!screens.some((screen) => screen.id === selectedScreenId)) {
      setSelectedScreenId(screens[0].id);
    }
  }, [screens, selectedScreenId]);

  const selectedScreen = useMemo(
    () => screens.find((screen) => screen.id === selectedScreenId) ?? screens[0],
    [screens, selectedScreenId]
  );
  const totalScreens = screens.length;
  const selectedScreenOverrides = selectedScreen ? valueOverrides[selectedScreen.id] ?? {} : {};
  useEffect(() => {
    if (!selectedScreen) {
      if (selectedElementId !== null) {
        setSelectedElementId(null);
      }
      return;
    }
    if (!selectedScreen.elements.some((element) => element.id === selectedElementId)) {
      setSelectedElementId(selectedScreen.elements[0]?.id ?? null);
    }
  }, [selectedScreen, selectedElementId]);
  const hierarchy = useMemo(() => buildScreenHierarchy(screens), [screens]);
  const breadcrumbs = useMemo(
    () =>
      hierarchy.breadcrumbsMap.get(selectedScreenId) ??
      (selectedScreen ? [selectedScreen.name] : []),
    [hierarchy, selectedScreen, selectedScreenId]
  );
  const scrollIndicator = hierarchy.pathMap.get(selectedScreenId);
  const canDeleteScreen = totalScreens > 1;

  const handleAddScreen = useCallback(
    (mode: "root" | "child") => {
      setDataset((current) => {
        const name =
          mode === "child" ? "Child screen" : `Screen ${current.screens.length + 1}`;
        const newScreen = createScreenTemplate(name);
        const nextScreens = [...current.screens, newScreen];
        if (mode === "child" && selectedScreenId) {
          const parentIndex = nextScreens.findIndex((screen) => screen.id === selectedScreenId);
          if (parentIndex >= 0) {
            const parent = nextScreens[parentIndex];
            const nextSubmenus = [
              ...(parent.submenus ?? []),
              { id: createId("submenu"), label: newScreen.name, screenId: newScreen.id }
            ];
            nextScreens[parentIndex] = { ...parent, submenus: nextSubmenus };
          }
        }
        setSelectedScreenId(newScreen.id);
        setSelectedElementId(newScreen.elements[0]?.id ?? null);
        return { ...current, screens: nextScreens };
      });
    },
    [selectedScreenId]
  );

  const handleDuplicateScreen = useCallback(() => {
    if (!selectedScreen) {
      return;
    }
    setDataset((current) => {
      const index = current.screens.findIndex((screen) => screen.id === selectedScreen.id);
      if (index === -1) {
        return current;
      }
      const clone: ScreenDefinition = {
        ...selectedScreen,
        id: createId("screen"),
        name: `${selectedScreen.name} Copy`,
        elements: selectedScreen.elements.map((element) => ({
          ...element,
          id: createId("element")
        })),
        flows: selectedScreen.flows?.map((flow) => ({
          ...flow,
          id: createId("flow")
        })),
        submenus: []
      };
      const nextScreens = [...current.screens];
      nextScreens.splice(index + 1, 0, clone);
      setSelectedScreenId(clone.id);
      setSelectedElementId(clone.elements[0]?.id ?? null);
      return { ...current, screens: nextScreens };
    });
  }, [selectedScreen]);

  const handleDeleteScreen = useCallback(() => {
    if (!selectedScreenId || !canDeleteScreen) {
      return;
    }
    setDataset((current) => {
      if (current.screens.length <= 1) {
        return current;
      }
      const nextScreens = current.screens
        .filter((screen) => screen.id !== selectedScreenId)
        .map((screen) => ({
          ...screen,
          submenus: screen.submenus?.filter((submenu) => submenu.screenId !== selectedScreenId)
        }));
      const nextId = nextScreens[0]?.id ?? "";
      setSelectedScreenId(nextId);
      const nextElements =
        nextScreens.find((screen) => screen.id === nextId)?.elements ?? [];
      setSelectedElementId(nextElements[0]?.id ?? null);
      return { ...current, screens: nextScreens };
    });
  }, [canDeleteScreen, selectedScreenId]);

  const handleReorderScreen = useCallback(
    (direction: "up" | "down") => {
      if (!selectedScreenId) {
        return;
      }
      setDataset((current) => {
        const parentId = findParentScreenId(current.screens, selectedScreenId);
        if (!parentId) {
          const index = current.screens.findIndex((screen) => screen.id === selectedScreenId);
          const targetIndex = direction === "up" ? index - 1 : index + 1;
          if (index === -1 || targetIndex < 0 || targetIndex >= current.screens.length) {
            return current;
          }
          const nextScreens = [...current.screens];
          const [screen] = nextScreens.splice(index, 1);
          nextScreens.splice(targetIndex, 0, screen);
          return { ...current, screens: nextScreens };
        }
        const parentIndex = current.screens.findIndex((screen) => screen.id === parentId);
        if (parentIndex === -1) {
          return current;
        }
        const parent = current.screens[parentIndex];
        const submenus = parent.submenus ?? [];
        const index = submenus.findIndex((submenu) => submenu.screenId === selectedScreenId);
        const targetIndex = direction === "up" ? index - 1 : index + 1;
        if (index === -1 || targetIndex < 0 || targetIndex >= submenus.length) {
          return current;
        }
        const nextSubmenus = [...submenus];
        const [entry] = nextSubmenus.splice(index, 1);
        nextSubmenus.splice(targetIndex, 0, entry);
        const nextScreens = [...current.screens];
        nextScreens[parentIndex] = { ...parent, submenus: nextSubmenus };
        return { ...current, screens: nextScreens };
      });
    },
    [selectedScreenId]
  );

  const handleAddElement = useCallback(
    (kind: ElementKind) => {
      if (!selectedScreenId) {
        return;
      }
      setDataset((current) => {
        const index = current.screens.findIndex((screen) => screen.id === selectedScreenId);
        if (index === -1) {
          return current;
        }
        const screen = current.screens[index];
        const newElement = createElementTemplate(kind, screen);
        const nextScreen = { ...screen, elements: [...screen.elements, newElement] };
        const nextScreens = [...current.screens];
        nextScreens[index] = nextScreen;
        setSelectedElementId(newElement.id);
        return { ...current, screens: nextScreens };
      });
    },
    [selectedScreenId]
  );

  const handleRemoveElement = useCallback(
    (elementId: string) => {
      if (!selectedScreenId) {
        return;
      }
      setDataset((current) => {
        const index = current.screens.findIndex((screen) => screen.id === selectedScreenId);
        if (index === -1) {
          return current;
        }
        const screen = current.screens[index];
        if (screen.elements.length <= 1) {
          return current;
        }
        const nextScreen = {
          ...screen,
          elements: screen.elements.filter((element) => element.id !== elementId)
        };
        const nextScreens = [...current.screens];
        nextScreens[index] = nextScreen;
        setSelectedElementId(nextScreen.elements[0]?.id ?? null);
        return { ...current, screens: nextScreens };
      });
    },
    [selectedScreenId]
  );

  const handleUpdateElement = useCallback(
    (elementId: string, updates: Partial<ScreenElement>) => {
      if (!selectedScreenId) {
        return;
      }
      setDataset((current) => {
        const index = current.screens.findIndex((screen) => screen.id === selectedScreenId);
        if (index === -1) {
          return current;
        }
        const screen = current.screens[index];
        const elements = screen.elements.map((element) =>
          element.id === elementId ? normalizeElementUpdate(element, updates) : element
        );
        const nextScreens = [...current.screens];
        nextScreens[index] = { ...screen, elements };
        return { ...current, screens: nextScreens };
      });
    },
    [selectedScreenId]
  );

  const handleNudgeSelectedElement = useCallback(
    (direction: "up" | "down" | "left" | "right") => {
      if (!selectedScreenId || !selectedElementId) {
        return;
      }
      const deltaX = direction === "left" ? -NUDGE_STEP : direction === "right" ? NUDGE_STEP : 0;
      const deltaY = direction === "up" ? -NUDGE_STEP : direction === "down" ? NUDGE_STEP : 0;
      setDataset((current) => {
        const index = current.screens.findIndex((screen) => screen.id === selectedScreenId);
        if (index === -1) {
          return current;
        }
        const screen = current.screens[index];
        const elements = screen.elements.map((element) => {
          if (element.id !== selectedElementId) {
            return element;
          }
          return {
            ...element,
            x: clampCoordinate(element.x + deltaX, "x"),
            y: clampCoordinate(element.y + deltaY, "y")
          };
        });
        const nextScreens = [...current.screens];
        nextScreens[index] = { ...screen, elements };
        return { ...current, screens: nextScreens };
      });
    },
    [selectedElementId, selectedScreenId]
  );

  const handleAddFlow = useCallback(() => {
    if (!selectedScreenId) {
      return;
    }
    setDataset((current) => {
      const index = current.screens.findIndex((screen) => screen.id === selectedScreenId);
      if (index === -1) {
        return current;
      }
      const screen = current.screens[index];
      const newFlow: ScreenFlow = {
        id: createId("flow"),
        label: "New button event",
        trigger: { type: "button", button: "up", gesture: "short" }
      };
      const nextScreen = { ...screen, flows: [...(screen.flows ?? []), newFlow] };
      const nextScreens = [...current.screens];
      nextScreens[index] = nextScreen;
      return { ...current, screens: nextScreens };
    });
  }, [selectedScreenId]);

  const handleUpdateFlow = useCallback(
    (flowId: string, updates: Partial<ScreenFlow>) => {
      if (!selectedScreenId) {
        return;
      }
      setDataset((current) => {
        const index = current.screens.findIndex((screen) => screen.id === selectedScreenId);
        if (index === -1) {
          return current;
        }
        const screen = current.screens[index];
        const flows = (screen.flows ?? []).map((flow) =>
          flow.id === flowId ? { ...flow, ...updates } : flow
        );
        const nextScreens = [...current.screens];
        nextScreens[index] = { ...screen, flows };
        return { ...current, screens: nextScreens };
      });
    },
    [selectedScreenId]
  );

  const handleDeleteFlow = useCallback(
    (flowId: string) => {
      if (!selectedScreenId) {
        return;
      }
      setDataset((current) => {
        const index = current.screens.findIndex((screen) => screen.id === selectedScreenId);
        if (index === -1) {
          return current;
        }
        const screen = current.screens[index];
        const nextScreen = {
          ...screen,
          flows: (screen.flows ?? []).filter((flow) => flow.id !== flowId)
        };
        const nextScreens = [...current.screens];
        nextScreens[index] = nextScreen;
        return { ...current, screens: nextScreens };
      });
    },
    [selectedScreenId]
  );

  const handleApplyDatasetFromJson = useCallback(
    (nextDataset: ScreenDataset) => {
      const result = prepareDataset(nextDataset);
      setDataset(result.dataset);
      announceClampCorrections(result.corrections, "applying JSON edits");
      setValidationFeedback({
        status: "success",
        message: "Dataset updated from JSON editor.",
        issues: []
      });
    },
    [announceClampCorrections]
  );

  const handleUploadFrames = useCallback(
    (files: FileList) => {
      if (!selectedScreenId || files.length === 0) {
        return;
      }
      const run = async () => {
        const fileArray = Array.from(files);
        const embeddedFrames = await Promise.all(fileArray.map((file) => fileToDataUrl(file)));
        const frameNames = fileArray.map((file) => file.name);
        setDataset((current) => {
          const index = current.screens.findIndex((screen) => screen.id === selectedScreenId);
          if (index === -1) {
            return current;
          }
          const screen = current.screens[index];
          const newAsset = {
            id: createId("asset"),
            type: "svg-sequence" as const,
            source: frameNames[0],
            frames: frameNames,
            fps: 12,
            embeddedFrames
          };
          const nextScreen = {
            ...screen,
            assets: [...(screen.assets ?? []), newAsset]
          };
          const nextScreens = [...current.screens];
          nextScreens[index] = nextScreen;
          return { ...current, screens: nextScreens };
        });
      };
      run().catch((error) => console.error("Failed to upload frames", error));
    },
    [selectedScreenId]
  );

  const handleReorderFrame = useCallback(
    (assetId: string, frameIndex: number, direction: "up" | "down") => {
      if (!selectedScreenId) {
        return;
      }
      setDataset((current) => {
        const screenIndex = current.screens.findIndex((screen) => screen.id === selectedScreenId);
        if (screenIndex === -1) {
          return current;
        }
        const screen = current.screens[screenIndex];
        const assets = screen.assets ?? [];
        const assetIndex = assets.findIndex((asset) => asset.id === assetId);
        if (assetIndex === -1) {
          return current;
        }
        const asset = assets[assetIndex];
        const frames = asset.frames ?? [];
        const targetIndex = direction === "up" ? frameIndex - 1 : frameIndex + 1;
        if (targetIndex < 0 || targetIndex >= frames.length) {
          return current;
        }
        const nextFrames = [...frames];
        const [moved] = nextFrames.splice(frameIndex, 1);
        nextFrames.splice(targetIndex, 0, moved);
        let nextEmbedded = asset.embeddedFrames;
        if (asset.embeddedFrames && asset.embeddedFrames.length === frames.length) {
          nextEmbedded = [...asset.embeddedFrames];
          const [frameData] = nextEmbedded.splice(frameIndex, 1);
          nextEmbedded.splice(targetIndex, 0, frameData);
        }
        const updatedAsset = { ...asset, frames: nextFrames, embeddedFrames: nextEmbedded };
        const nextAssets = [...assets];
        nextAssets[assetIndex] = updatedAsset;
        const nextScreen = { ...screen, assets: nextAssets };
        const nextScreens = [...current.screens];
        nextScreens[screenIndex] = nextScreen;
        return { ...current, screens: nextScreens };
      });
    },
    [selectedScreenId]
  );

  const validateDatasetSafe = useCallback((raw: unknown) => {
    try {
      const validated = validateDataset(raw);
      return { dataset: validated, issues: [] as string[] };
    } catch (error) {
      if (error instanceof SchemaValidationError) {
        return { dataset: null, issues: error.issues };
      }
      return { dataset: null, issues: [error instanceof Error ? error.message : String(error)] };
    }
  }, []);

  const handleImportClick = useCallback(() => {
    fileInputRef.current?.click();
  }, []);

  const handleDatasetImport = useCallback(
    async (event: ChangeEvent<HTMLInputElement>) => {
      const file = event.target.files?.[0];
      if (!file) {
        return;
      }
      try {
        const text = await file.text();
        const parsed = JSON.parse(text);
        const { dataset: validated, issues } = validateDatasetSafe(parsed);
        if (!validated || issues.length > 0) {
          setValidationFeedback({
            status: "error",
            message: `Failed to import ${file.name}. Resolve the issues below and try again.`,
            issues
          });
        } else {
          const result = prepareDataset(validated);
          setDataset(result.dataset);
          announceClampCorrections(result.corrections, `importing ${file.name}`);
          setSelectedScreenId(result.dataset.screens[0]?.id ?? "");
          setValidationFeedback({
            status: "success",
            message: `Imported ${file.name} (${validated.screens.length} screens).`,
            issues: []
          });
        }
      } catch (error) {
        setValidationFeedback({
          status: "error",
          message: `Unable to parse ${file.name}. Ensure it is valid JSON.`,
          issues: [error instanceof Error ? error.message : String(error)]
        });
      } finally {
        event.target.value = "";
      }
    },
    [announceClampCorrections, validateDatasetSafe]
  );

  const parseManifestSafe = useCallback((raw: unknown): FirmwareActionManifest => {
    if (typeof raw !== "object" || raw === null) {
      throw new Error("Manifest must be an object");
    }
    const candidate = raw as Partial<FirmwareActionManifest>;
    if (!Array.isArray(candidate.actions)) {
      throw new Error("Manifest is missing an actions array");
    }
    candidate.actions.forEach((action, index) => {
      if (!action?.id || !action?.label) {
        throw new Error(`Manifest entry #${index + 1} missing id/label`);
      }
    });
    return {
      updatedAt: candidate.updatedAt ?? new Date().toISOString(),
      actions: candidate.actions as FirmwareActionDefinition[]
    };
  }, []);

  const handleManifestUploadClick = useCallback(() => {
    manifestInputRef.current?.click();
  }, []);

  const handleManifestImport = useCallback(
    async (event: ChangeEvent<HTMLInputElement>) => {
      const file = event.target.files?.[0];
      if (!file) {
        return;
      }
      try {
        const payload = JSON.parse(await file.text());
        const parsed = parseManifestSafe(payload);
        setFirmwareManifest(parsed);
        setManifestFeedback({
          status: "success",
          message: `Loaded ${file.name} (${parsed.actions.length} actions).`
        });
      } catch (error) {
        setManifestFeedback({
          status: "error",
          message: error instanceof Error ? error.message : String(error)
        });
      } finally {
        event.target.value = "";
      }
    },
    [parseManifestSafe]
  );

  const handleValidateClick = useCallback(() => {
    const { dataset: validated, issues } = validateDatasetSafe(dataset);
    if (!validated || issues.length > 0) {
      setValidationFeedback({
        status: "error",
        message: "Validation failed. Resolve the issues below before exporting.",
        issues
      });
      return;
    }
    const result = prepareDataset(validated);
    setDataset(result.dataset);
    announceClampCorrections(result.corrections, "manual validation");
    setValidationFeedback({
      status: "success",
      message: `Dataset validated (${validated.screens.length} screens).`,
      issues: []
    });
  }, [announceClampCorrections, dataset, validateDatasetSafe]);

  const layoutReport = useMemo(
    () => (selectedScreen ? computeLayout(selectedScreen, orientation) : undefined),
    [selectedScreen, orientation]
  );
  const overflow = layoutReport?.overflow ?? [];

  useEffect(() => {
    if (!themesEqual(theme, dataset.theme)) {
      updateTheme(() => cloneTheme(dataset.theme));
    }
  }, [dataset.theme, theme, updateTheme]);

  useEffect(() => {
    if (!transitionPreview) {
      return undefined;
    }
    const remaining = transitionPreview.expiresAt - Date.now();
    if (remaining <= 0) {
      setTransitionPreview(null);
      return undefined;
    }
    const timer = window.setTimeout(() => setTransitionPreview(null), remaining);
    return () => window.clearTimeout(timer);
  }, [transitionPreview]);

  useEffect(() => {
    setDataset((current) => {
      if (themesEqual(current.theme, theme)) {
        return current;
      }
      return {
        ...current,
        theme: cloneTheme(theme)
      };
    });
  }, [theme]);

  const appendLog = useCallback((message: string) => {
    setInteractionLog((current) => {
      const next = [message, ...current];
      return next.slice(0, 6);
    });
  }, []);

  const recordTraceEntry = useCallback(
    (entry: Omit<SimulationTraceEntry, "timestamp"> & { timestamp?: number }) => {
      const actionDefinition = entry.id ? actionCatalog.get(entry.id) : undefined;
      setTraceEntries((current) => {
        const next = [
          {
            ...entry,
            functionName: entry.functionName ?? actionDefinition?.label,
            timestamp: entry.timestamp ?? Date.now()
          },
          ...current
        ];
        return next.slice(0, 25);
      });
    },
    [actionCatalog]
  );

  const handleTraceReplay = useCallback(
    (entry: SimulationTraceEntry) => {
      recordTraceEntry({
        ...entry,
        timestamp: Date.now(),
        notes: entry.notes ? `${entry.notes} • replay` : "replay"
      });
    },
    [recordTraceEntry]
  );

  const handleTraceClear = useCallback(() => {
    setTraceEntries([]);
  }, []);

  const handleValueChange = useCallback(
    (screenId: string, element: ScreenElement, nextValue: string) => {
      const baseValue = element.content ?? "";
      const currentOverride = valueOverrides[screenId]?.[element.id];
      const previousValue = currentOverride ?? baseValue;
      if (previousValue === baseValue && nextValue !== baseValue) {
        recordTraceEntry({
          id: "ui.mock.value-edit",
          label: "Value edited",
          functionName: actionCatalog.get("ui.mock.value-edit")?.label,
          trigger: `value.edit.${element.id}`,
          screenId,
          screenName: selectedScreen?.name,
          actionParams: { elementId: element.id, value: nextValue }
        });
      }
      setValueOverrides((current) => {
        const existing = current[screenId] ?? {};
        return {
          ...current,
          [screenId]: {
            ...existing,
            [element.id]: nextValue
          }
        };
      });
    },
    [actionCatalog, recordTraceEntry, selectedScreen?.name, valueOverrides]
  );

  const handleValueRevert = useCallback((screenId: string, element: ScreenElement) => {
    setValueOverrides((current) => {
      const existing = current[screenId];
      if (!existing) {
        return current;
      }
      const { [element.id]: _removed, ...rest } = existing;
      const next = { ...current };
      if (Object.keys(rest).length === 0) {
        delete next[screenId];
      } else {
        next[screenId] = rest;
      }
      return next;
    });
  }, []);

  const handleValueSave = useCallback(
    (screenId: string, element: ScreenElement, value: string) => {
      recordTraceEntry({
        id: "ui.mock.value-save",
        label: "Value saved",
        functionName: actionCatalog.get("ui.mock.value-save")?.label,
        trigger: `value.save.${element.id}`,
        screenId,
        screenName: selectedScreen?.name,
        actionParams: { elementId: element.id, value }
      });
      recordTraceEntry({
        id: "core.action.save-config",
        label: "Save configuration",
        functionName: actionCatalog.get("core.action.save-config")?.label,
        trigger: `value.save.${element.id}`,
        screenId,
        screenName: selectedScreen?.name,
        actionParams: { elementId: element.id }
      });
    },
    [actionCatalog, recordTraceEntry, selectedScreen?.name]
  );

  const previewTransition = useCallback(
    (payload: {
      targetScreenId?: string;
      actionId?: string;
      actionLabel?: string;
      triggerLabel: string;
      effect: TransitionEffect;
    }) => {
      if (!payload.targetScreenId && !payload.actionLabel) {
        return;
      }

      const targetScreen = payload.targetScreenId
        ? dataset.screens.find((candidate) => candidate.id === payload.targetScreenId)
        : undefined;
      const previewLayout =
        targetScreen && payload.targetScreenId ? computeLayout(targetScreen, orientation) : undefined;

      setTransitionPreview({
        screenId: payload.targetScreenId ?? selectedScreen?.id ?? "—",
        screenName: targetScreen?.name ?? payload.targetScreenId ?? selectedScreen?.name,
        actionId: payload.actionId,
        actionLabel: payload.actionLabel ?? payload.actionId,
        triggerLabel: payload.triggerLabel,
        effect: payload.effect,
        previewLayout,
        expiresAt: Date.now() + 1500
      });
    },
    [dataset.screens, orientation, selectedScreen?.id, selectedScreen?.name]
  );

  const selectByOffset = useCallback(
    (offset: number): string | undefined => {
      if (totalScreens === 0) {
        return undefined;
      }
      const currentIndex = screens.findIndex((screen) => screen.id === selectedScreenId);
      const safeIndex = currentIndex >= 0 ? currentIndex : 0;
      const nextIndex = (safeIndex + offset + totalScreens) % totalScreens;
      const nextId = screens[nextIndex].id;
      setSelectedScreenId(nextId);
      return nextId;
    },
    [screens, selectedScreenId, totalScreens]
  );

  const selectById = useCallback(
    (id: string): string | undefined => {
      const target = screens.find((screen) => screen.id === id);
      if (target) {
        setSelectedScreenId(target.id);
        return target.id;
      }
      return undefined;
    },
    [screens]
  );

  const handleNavigateFromValidation = useCallback(
    (screenId: string) => {
      const resolved = selectById(screenId);
      if (resolved) {
        setActivePanel("design");
      }
    },
    [selectById]
  );

  const handleButtonEvent = useCallback(
    (event: SimulatedButtonEvent) => {
      const triggerLabel = formatTriggerLabel(event);
      const effect = deriveTransitionEffect(event);
      let activeScreenId = selectedScreen?.id ?? screens[0]?.id ?? "—";
      const resolvedFlows = findMatchingButtonFlows(selectedScreen, event);
      if (resolvedFlows.length > 0) {
        resolvedFlows.forEach((flow) => {
          const actionDefinition = flow.actionId ? actionCatalog.get(flow.actionId) : undefined;
          if (flow.targetScreenId) {
            const resolvedId = selectById(flow.targetScreenId);
            if (resolvedId) {
              activeScreenId = resolvedId;
            }
          }
          recordTraceEntry({
            id: flow.actionId ?? "ui.action.unassigned",
            label: flow.label,
            functionName: actionDefinition?.label,
            trigger: `${event.button}.${event.kind}`,
            screenId: selectedScreen?.id ?? "unknown",
            screenName: selectedScreen?.name,
            actionParams: flow.actionParams ?? null,
            targetScreenId: flow.targetScreenId
          });
          previewTransition({
            targetScreenId: flow.targetScreenId,
            actionId: flow.actionId,
            actionLabel: actionDefinition?.label ?? flow.label,
            triggerLabel,
            effect
          });
        });
      } else {
        let fallbackDestination: string | undefined;
        if (event.button === "up" && event.kind !== "long") {
          fallbackDestination = selectByOffset(-1);
        } else if (event.button === "down" && event.kind !== "long") {
          fallbackDestination = selectByOffset(1);
        } else if (event.button === "enter" && event.kind === "short") {
          fallbackDestination = selectById("configuration");
        }
        if (fallbackDestination) {
          activeScreenId = fallbackDestination;
        }
        recordTraceEntry({
          id: "ui.action.unmapped",
          label: "No matching flow",
          trigger: `${event.button}.${event.kind}`,
          screenId: selectedScreen?.id ?? "unknown",
          screenName: selectedScreen?.name
        });
      }

      appendLog(
        `[${new Date(event.timestamp).toLocaleTimeString()}] ${triggerLabel} → ${activeScreenId}`
      );
    },
    [actionCatalog, appendLog, previewTransition, recordTraceEntry, selectById, selectByOffset, selectedScreen, screens]
  );

  const { pressed, press, release, cancelAll } = useSimulatedButtons(handleButtonEvent);

  const mapKeyToButton = useCallback((key: string): SimulatedButton | undefined => {
    switch (key) {
      case "ArrowUp":
        return "up";
      case "ArrowDown":
        return "down";
      case "Enter":
        return "enter";
      default:
        return undefined;
    }
  }, []);

  const mapArrowKeyToDirection = useCallback(
    (key: string): ("up" | "down" | "left" | "right") | null => {
      switch (key) {
        case "ArrowUp":
          return "up";
        case "ArrowDown":
          return "down";
        case "ArrowLeft":
          return "left";
        case "ArrowRight":
          return "right";
        default:
          return null;
      }
    },
    []
  );

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      const arrowDirection = mapArrowKeyToDirection(event.key);
      if (arrowDirection && activePanel === "design") {
        event.preventDefault();
        if (selectedElementId) {
          handleNudgeSelectedElement(arrowDirection);
        }
        return;
      }
      const button = mapKeyToButton(event.key);
      if (!button) {
        return;
      }
      if (event.repeat) {
        event.preventDefault();
        return;
      }
      event.preventDefault();
      press(button);
    };

    const handleKeyUp = (event: KeyboardEvent) => {
      const arrowDirection = mapArrowKeyToDirection(event.key);
      if (arrowDirection && activePanel === "design") {
        event.preventDefault();
        return;
      }
      const button = mapKeyToButton(event.key);
      if (!button) {
        return;
      }
      event.preventDefault();
      release(button);
    };

    const handleWindowBlur = () => {
      cancelAll();
    };

    const handleVisibilityChange = () => {
      if (document.hidden) {
        cancelAll();
      }
    };

    window.addEventListener("keydown", handleKeyDown);
    window.addEventListener("keyup", handleKeyUp);
    window.addEventListener("blur", handleWindowBlur);
    document.addEventListener("visibilitychange", handleVisibilityChange);

    return () => {
      window.removeEventListener("keydown", handleKeyDown);
      window.removeEventListener("keyup", handleKeyUp);
      window.removeEventListener("blur", handleWindowBlur);
      document.removeEventListener("visibilitychange", handleVisibilityChange);
      cancelAll();
    };
  }, [
    activePanel,
    cancelAll,
    handleNudgeSelectedElement,
    mapArrowKeyToDirection,
    mapKeyToButton,
    press,
    release,
    selectedElementId
  ]);

  const themeSnapshotSection = (
    <section className="theme-snapshot">
      <h2>Theme snapshot</h2>
      <div className="screen-details">
        <p><strong>Name:</strong> {dataset.theme.name}</p>
        <p><strong>Value colour:</strong> {dataset.theme.colors.value}</p>
        <p><strong>Orientation:</strong> {orientation}</p>
      </div>
    </section>
  );

  const renderScreenContext = (options?: { showThemeSnapshot?: boolean; hideSelector?: boolean }) => (
    <div className="screen-context">
      {options?.hideSelector ? null : (
        <section>
          <h2>Screens</h2>
          <ScreenSelector
            screens={screens}
            activeId={selectedScreen?.id ?? ""}
            previewId={transitionPreview?.screenId}
            onSelect={setSelectedScreenId}
          />
        </section>
      )}
      <section>
        <h2>Screen details</h2>
        <div className="screen-details">
          <p><strong>ID:</strong> {selectedScreen?.id}</p>
          <p>{selectedScreen?.description}</p>
          <p><strong>Elements:</strong> {selectedScreen?.elements.length}</p>
          <hr />
          <p><strong>Validation:</strong> {validationLabel}</p>
          <p><strong>Active screen:</strong> {selectedScreen?.name ?? "—"}</p>
          <p><strong>Total screens:</strong> {datasetSummary.screenCount}</p>
        </div>
      </section>
      <section className="interaction-log">
        <h2>Button events</h2>
        {interactionLog.length === 0 ? (
          <span>No simulated input yet.</span>
        ) : (
          interactionLog.map((entry, index) => <span key={index}>{entry}</span>)
        )}
      </section>
      {options?.showThemeSnapshot ? themeSnapshotSection : null}
    </div>
  );

  const validationLabel =
    validationFeedback.status === "success"
      ? "Ready"
      : validationFeedback.status === "error"
        ? "Needs fixes"
        : "Idle";

  return (
    <div className="app-shell">
      <main className="workspace">
        <div className="workspace-tabs">
          <button
            type="button"
            className={activePanel === "simulation" ? "active" : ""}
            onClick={() => setActivePanel("simulation")}
          >
            Simulation
          </button>
          <button
            type="button"
            className={activePanel === "design" ? "active" : ""}
            onClick={() => setActivePanel("design")}
          >
            Design
          </button>
          <button
            type="button"
            className={activePanel === "importExport" ? "active" : ""}
            onClick={() => setActivePanel("importExport")}
          >
            Import &amp; Export
          </button>
          <button
            type="button"
            className={activePanel === "help" ? "active" : ""}
            onClick={() => setActivePanel("help")}
          >
            Help &amp; Documentation
          </button>
        </div>

        {activePanel === "simulation" && (
          <section className="panel simulation-view">
            <div className="panel-grid panel-grid--simulation">
              <div className="panel-column panel-column--context">{renderScreenContext()}</div>
              <div className="panel-column panel-column--main">
                <section className="controls">
                  <div className="labelled-control">
                    <label htmlFor="zoom">Zoom ({zoom}%)</label>
                    <input
                      id="zoom"
                      type="range"
                      min={100}
                      max={400}
                      value={zoom}
                      onChange={(event) => setZoom(clamp(Number(event.target.value), 100, 400))}
                    />
                  </div>
                  <div className="labelled-control">
                    <label className="grid-toggle">
                      <input
                        type="checkbox"
                        checked={showGrid}
                        onChange={(event) => setShowGrid(event.target.checked)}
                      />
                      Show grid overlay
                    </label>
                  </div>
                  <div className="labelled-control">
                    <label>Orientation</label>
                    <div className="orientation-group">
                      <button
                        type="button"
                        className={orientation === "landscape" ? "active" : ""}
                        onClick={() => setOrientation("landscape")}
                      >
                        Landscape
                      </button>
                      <button
                        type="button"
                        className={orientation === "portrait" ? "active" : ""}
                        onClick={() => setOrientation("portrait")}
                      >
                        Portrait
                      </button>
                    </div>
                  </div>
                </section>

                <section className="viewport-container">
                  {layoutReport ? (
                    <DisplayViewport
                      layout={layoutReport}
                      zoomPercent={zoom}
                      showGrid={showGrid}
                      valueOverrides={selectedScreenOverrides}
                      pendingTransition={transitionPreview}
                      scrollIndicator={scrollIndicator}
                    />
                  ) : (
                    <p>No screen selected.</p>
                  )}
                </section>

                <ButtonPanel pressed={pressed} onPressStart={press} onPressEnd={release} />

                <ValuePlaceholderPanel
                  screen={selectedScreen}
                  overrides={selectedScreenOverrides}
                  onChange={handleValueChange}
                  onRevert={handleValueRevert}
                  onSave={handleValueSave}
                />

                <section className="layout-warnings">
                  <strong>Layout diagnostics</strong>
                  {clampNotice ? (
                    <div className="layout-correction-alert">
                      <div>
                        <p>
                          {clampNotice.total} element{clampNotice.total === 1 ? "" : "s"} were clamped when{" "}
                          {clampNotice.context}.
                        </p>
                        <ul>
                          {clampNotice.samples.map((sample) => (
                            <li key={`${sample.screenId}:${sample.elementId}`}>
                              <code>{sample.screenId}</code> · <code>{sample.elementId}</code> (
                              {sample.adjustments
                                .map(
                                  (adjustment) => `${adjustment.field}: ${adjustment.from}→${adjustment.to}`
                                )
                                .join(", ")}
                              )
                            </li>
                          ))}
                        </ul>
                      </div>
                      <button type="button" className="tool-button tool-button--secondary" onClick={() => setClampNotice(null)}>
                        Dismiss
                      </button>
                    </div>
                  ) : null}
                  {overflow.length === 0 ? (
                    <span className="ok">
                      All elements fit within {layoutReport?.bounds.width} × {layoutReport?.bounds.height}px.
                    </span>
                  ) : (
                    <ul>
                      {overflow.map((item) => (
                        <li key={item.element.id}>
                          <code>{item.element.id}</code> exceeded the {layoutReport?.bounds.width ?? DISPLAY_WIDTH} ×{" "}
                          {layoutReport?.bounds.height ?? DISPLAY_HEIGHT}px viewport (raw x: {item.originalLeft}, y: {item.originalTop}, w: {item.originalWidth}, h: {item.originalHeight}) → clipped to (x: {item.left}, y: {item.top}, w: {item.width}, h: {item.height}).
                        </li>
                      ))}
                    </ul>
                  )}
                </section>

                <div className="workspace-footer">
                  <span>
                    Data Source: <code>src/data/screens.json</code>
                  </span>
                  <span>
                    Resolution: {layoutReport?.bounds.width ?? 135} × {layoutReport?.bounds.height ?? 240} px · Zoomed to {(zoom / 100).toFixed(1)}×
                  </span>
                </div>
              </div>
              <div className="panel-column panel-column--trace">
                <SimulationTracePanel
                  entries={traceEntries}
                  filter={traceFilter}
                  onFilterChange={setTraceFilter}
                  onReplay={handleTraceReplay}
                  onClear={handleTraceClear}
                />
              </div>
            </div>
          </section>
        )}

        {activePanel === "design" && (
          <section className="panel design-view">
            <div className="panel-grid panel-grid--design">
              <div className="panel-column panel-column--context">
                <ScreenHierarchyPanel
                  nodes={hierarchy.roots}
                  selectedId={selectedScreenId}
                  breadcrumbs={breadcrumbs}
                  pathLabel={scrollIndicator}
                  onSelect={setSelectedScreenId}
                  onAddRoot={() => handleAddScreen("root")}
                  onAddChild={() => handleAddScreen("child")}
                  onDuplicate={handleDuplicateScreen}
                  onDelete={handleDeleteScreen}
                  onReorder={handleReorderScreen}
                  canDelete={canDeleteScreen}
                />
                {renderScreenContext({ showThemeSnapshot: true, hideSelector: true })}
              </div>
              <div className="panel-column panel-column--main">
                <ThemeEditor
                  layout={layoutReport}
                  zoomPercent={zoom}
                  showGrid={showGrid}
                  screen={selectedScreen}
                  previewFooter={
                    <section className="json-live json-live--design">
                      <strong>Live screen JSON</strong>
                      <p>
                        Snapshot of <code>{selectedScreen?.id ?? "—"}</code> rendered from the dataset. Updates immediately when you
                        switch screens or edit JSON.
                      </p>
                      <pre>{selectedScreen ? JSON.stringify(selectedScreen, null, 2) : "// Select a screen to view JSON."}</pre>
                    </section>
                  }
                />
                <DesignToolbox
                  screen={selectedScreen}
                  onAddElement={handleAddElement}
                  onRemoveElement={handleRemoveElement}
                  onUpdateElement={handleUpdateElement}
                  selectedElementId={selectedElementId}
                  onSelectElement={setSelectedElementId}
                  onNudgeElement={handleNudgeSelectedElement}
                  maxCoordinateX={MAX_COORD_X}
                  maxCoordinateY={MAX_COORD_Y}
                  maxDimension={MAX_SIZE}
                  maxInputLength={MAX_INPUT_LENGTH}
                />
                <EventBindingPanel
                  screen={selectedScreen}
                  actions={firmwareManifest.actions}
                  screens={screens}
                  onAddFlow={handleAddFlow}
                  onDeleteFlow={handleDeleteFlow}
                  onUpdateFlow={handleUpdateFlow}
                />
                <AnimationInspector
                  screen={selectedScreen}
                  onUploadFrames={handleUploadFrames}
                  onReorderFrame={handleReorderFrame}
                />
                <LiveJsonEditorPanel
                  dataset={dataset}
                  onApplyDataset={handleApplyDatasetFromJson}
                  validateDatasetSafe={validateDatasetSafe}
                />
              </div>
            </div>
          </section>
        )}

        {activePanel === "importExport" && (
          <section className="panel export-view">
            <header>
              <h3>Import &amp; Export workflow</h3>
              <p>
                Bring datasets into the workspace, validate them, and translate directly to firmware assets. Recent backups are created automatically in
                <code> backups/ui/</code>, and design tokens travel with <code>screens.json</code> so firmware mirrors the preview.
              </p>
            </header>
            <div className="import-export-layout">
              <div className="dataset-tools-panel">
                <h4>Dataset tools</h4>
                <p>Manage the JSON definition before exporting or translating.</p>
                <div className="dataset-actions">
                  <button type="button" className="tool-button" onClick={handleImportClick}>
                    Import JSON
                  </button>
                  <button
                    type="button"
                    className="tool-button tool-button--secondary"
                    onClick={handleValidateClick}
                  >
                    Validate JSON
                  </button>
                  <button
                    type="button"
                    className="tool-button tool-button--secondary"
                    onClick={screenJsonDownload}
                  >
                    Export JSON
                  </button>
                  <input
                    ref={fileInputRef}
                    type="file"
                    accept="application/json"
                    onChange={handleDatasetImport}
                    data-testid="dataset-import"
                    style={{ display: "none" }}
                  />
                </div>
                <div className="manifest-actions">
                  <h5>Firmware actions</h5>
                  <p>Import the JSON manifest exported from the firmware to annotate simulation traces.</p>
                  <button
                    type="button"
                    className="tool-button tool-button--secondary"
                    onClick={handleManifestUploadClick}
                  >
                    Import manifest
                  </button>
                  <input
                    ref={manifestInputRef}
                    type="file"
                    accept="application/json"
                    onChange={handleManifestImport}
                    style={{ display: "none" }}
                  />
                  <p className={`manifest-feedback manifest-feedback--${manifestFeedback.status}`}>
                    {manifestFeedback.message}
                  </p>
                </div>
                {validationFeedback.status !== "idle" && (
                  <div
                    className={`dataset-feedback dataset-feedback--${validationFeedback.status}`}
                    data-testid="dataset-feedback"
                  >
                    <p>{validationFeedback.message}</p>
                    {validationFeedback.issues.length > 0 && (
                      <ul>
                        {validationFeedback.issues.map((issue) => (
                          <li key={issue}>{issue}</li>
                        ))}
                      </ul>
                    )}
                  </div>
                )}
              </div>

              <div className="export-workflow">
                <div className="export-summary">
                  <div>
                    <strong>Validation status:</strong>
                    <span className={`status-tag status-${validationFeedback.status}`}>{validationLabel}</span>
                  </div>
                  <div>
                    <strong>Screens:</strong> {datasetSummary.screenCount}
                  </div>
                  <div>
                    <strong>IDs:</strong> {datasetSummary.ids.join(", ") || "—"}
                  </div>
                </div>
                <ExporterPanel
                  disabled={validationFeedback.status === "error"}
                  onNavigateToScreen={handleNavigateFromValidation}
                />
              </div>
            </div>
          </section>
        )}

        {activePanel === "help" && (
          <section className="panel help-view">
            <div className="panel-grid">
              <div className="panel-column panel-column--context">{renderScreenContext()}</div>
              <div className="panel-column panel-column--main">
                <HelpPanel dataset={dataset} selectedScreen={selectedScreen} />
              </div>
            </div>
          </section>
        )}
      </main>
    </div>
  );
}

export default App;
