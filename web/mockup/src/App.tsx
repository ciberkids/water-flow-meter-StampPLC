import { ChangeEvent, useCallback, useEffect, useMemo, useRef, useState } from "react";
import "./App.css";
import packageJson from "../package.json";
import { DisplayViewport } from "./components/DisplayViewport";
import { ScreenSelector } from "./components/ScreenSelector";
import screensData from "./data/screens.json";
import actionManifestJson from "./data/actionManifest.json";
import {
  DisplayOrientation,
  ScreenDataset,
  ScreenDefinition,
  ScreenElement,
  ScreenEvent,
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
  clampElementGeometry,
  clampCoordinate,
  clampWidth,
  clampHeight,
  type DisplayBounds,
  ClampAdjustment,
  ClampCorrection
} from "./utils/datasetClamp";
import { SchemaValidationError, validateDataset } from "./schema/validation";
import { ExporterPanel } from "./components/ExporterPanel";
import { SimulationTracePanel } from "./components/SimulationTracePanel";
import { ValuePlaceholderPanel } from "./components/ValuePlaceholderPanel";
import { SimulationTraceEntry } from "./types/simulationTrace";
import { FirmwareActionManifest, FirmwareActionDefinition, FirmwareValueDefinition } from "./types/firmwareActions";
import { TransitionEffect, TransitionPreviewState } from "./types/transitionPreview";
import { findMatchingButtonFlows } from "./utils/flowMatching";
import { ScreenHierarchyPanel } from "./components/design/ScreenHierarchyPanel";
import { buildScreenHierarchy } from "./utils/screenHierarchy";
import { DesignToolbox } from "./components/design/DesignToolbox";
import { LiveJsonEditorPanel } from "./components/design/LiveJsonEditorPanel";
import { validateManifest } from "./schema/manifestValidation";
import { FirmwareLoopPanel } from "./components/FirmwareLoopPanel";
import { FirmwareValuesPanel } from "./components/FirmwareValuesPanel";
import { sampleValueFor } from "./utils/sampleValues";
import {
  SimulatedSensor,
  advanceSensorTick,
  createSensorTable,
  isPerSensorSetting,
  kSensorCount,
  pulsesForFlow,
  resolveSensorBinding,
  sensorIndexForScreen,
  setSensor,
  warningSensorNumbers
} from "./utils/sensorConfig";

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
const APP_VERSION = packageJson.version ?? "dev";
const MAX_INPUT_LENGTH = 4;
const NUDGE_STEP = 1;

type NudgeDirection = "up" | "down" | "left" | "right";

const getNudgeDelta = (direction: NudgeDirection) => {
  switch (direction) {
    case "up":
      // Verify direction: Up should increase Y (Visual Up in Bottom-Left origin)
      return { deltaX: 0, deltaY: NUDGE_STEP };
    case "down":
      return { deltaX: 0, deltaY: -NUDGE_STEP };
    case "left":
      return { deltaX: -NUDGE_STEP, deltaY: 0 };
    case "right":
      return { deltaX: NUDGE_STEP, deltaY: 0 };
    default:
      return { deltaX: 0, deltaY: 0 };
  }
};

const createScreenTemplate = (name: string): ScreenDefinition => ({
  id: createId("screen"),
  name,
  description: "",
  elements: [],
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

/** The device is landscape-only (decision D3); see the note in datasetClamp.ts. */
const LANDSCAPE_BOUNDS = { width: DISPLAY_HEIGHT, height: DISPLAY_WIDTH };

const prepareDataset = (source: ScreenDataset) => {
  const themed = ensureDatasetTheme(source);
  // Bounds passed explicitly: this runs on all four ingest paths (boot, JSON apply, import,
  // and the manual Validate immediately before Export), so an implicit default being wrong
  // silently rewrote geometry on every one of them.
  return clampDatasetToDisplay(themed, LANDSCAPE_BOUNDS);
};

const normalizeElementUpdate = (
  element: ScreenElement,
  updates: Partial<ScreenElement>,
  bounds: DisplayBounds
): ScreenElement => {
  const next: Partial<ScreenElement> = { ...updates };
  if (updates.x !== undefined) {
    next.x = clampCoordinate(updates.x, "x", bounds);
  }
  if (updates.y !== undefined) {
    next.y = clampCoordinate(updates.y, "y", bounds);
  }
  if (updates.width !== undefined) {
    next.width = clampWidth(updates.width, bounds);
  }
  if (updates.height !== undefined) {
    next.height = clampHeight(updates.height, bounds);
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
  // OFF by default. The grid draws minor lines at 28% and major at 55% opacity across the whole
  // panel, so every text row sits on a crosshatch — which is exactly what made the default screen
  // "not very readable". It is a measuring tool for the design tab, not a viewing default.
  const [showGrid, setShowGrid] = useState<boolean>(false);
  const [orientation, setOrientation] = useState<DisplayOrientation>("landscape");
  const orientationBounds: DisplayBounds = useMemo(
    () =>
      orientation === "landscape"
        ? { width: DISPLAY_HEIGHT, height: DISPLAY_WIDTH, orientation }
        : { width: DISPLAY_WIDTH, height: DISPLAY_HEIGHT, orientation },
    [orientation]
  );
  const datasetBounds: DisplayBounds = useMemo(
    () =>
      orientation === "landscape"
        ? { width: DISPLAY_HEIGHT, height: DISPLAY_WIDTH, orientation }
        : { width: DISPLAY_WIDTH, height: DISPLAY_HEIGHT, orientation },
    [orientation]
  );
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
  const handleLoadManifest = useCallback(
    async (file: File) => {
      try {
        const text = await file.text();
        const data = JSON.parse(text);
        const validated = validateManifest(data);
        setFirmwareManifest(validated);
        setManifestFeedback({ status: "success", message: `Loaded manifest with ${validated.actions.length} actions.` });
      } catch (error) {
        console.error("Manifest load failed", error);
        setManifestFeedback({ status: "error", message: error instanceof Error ? error.message : "Unknown error loading manifest" });
      }
    },
    []
  );

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

  /* ---- Simulated device memory ------------------------------------------------------------------
   *
   * ONE home for every value the panel can draw.
   *
   * What this replaces: a flat `firmwareLoopValues` map of binding id → string, which was fanned out
   * into per-element `valueOverrides`. That gave every fact two homes, and the override won — so
   * switching the selected sensor left the previous sensor's text pinned on screen, and eight sensors
   * shared one config because a single binding id can only hold one string.
   *
   * Memory is the source of truth: the loop advances it, an edit writes it, and every binding RESOLVES
   * from it. `sampleValueFor` survives only as the fallback for bindings memory does not model.
   */
  const manifestValueBindings = useMemo(() => {
    return (firmwareManifest.values ?? []).map((value) => ({
      id: value.id,
      type: value.type,
      unit: value.unit,
      description: value.description,
      category: value.category,
      perSensor: value.perSensor
    }));
  }, [firmwareManifest]);

  const manifestValueById = useMemo(() => {
    const index = new Map<string, FirmwareValueDefinition>();
    for (const value of firmwareManifest.values ?? []) {
      index.set(value.id, value);
    }
    return index;
  }, [firmwareManifest]);

  const [sensors, setSensors] = useState<SimulatedSensor[]>(() => createSensorTable());
  /** Manual pins for the bindings memory does not model (network, UART summary, page titles…). */
  const [pinnedValues, setPinnedValues] = useState<Record<string, string>>({});
  const [loopRunning, setLoopRunning] = useState<boolean>(false);
  const [loopIntervalMs, setLoopIntervalMs] = useState<number>(1000);
  /**
   * A sensor picked in the values panel, used only when navigation implies none.
   *
   * On the device the selected sensor comes from the navigation level and nowhere else
   * (`UiNavigator::sensorIndex_`). The simulator needs to reach a sensor's settings without walking the
   * tree first, so a manual pick fills in — but the navigation-derived index always wins, which keeps
   * the device's rule authoritative wherever it applies.
   */
  const [pickedSensor, setPickedSensor] = useState<number>(0);
  const connectedSensorCount = sensors.filter((sensor) => sensor.connected).length;
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
        if (!screen.elements.some((element) => element.id === elementId)) {
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
          element.id === elementId ? normalizeElementUpdate(element, updates, datasetBounds) : element
        );
        const nextScreens = [...current.screens];
        nextScreens[index] = { ...screen, elements };
        return { ...current, screens: nextScreens };
      });
    },
    [datasetBounds, selectedScreenId]
  );

  const handleNudgeSelectedElement = useCallback(
    (direction: NudgeDirection) => {
      if (!selectedScreenId || !selectedElementId) {
        return;
      }
      const { deltaX, deltaY } = getNudgeDelta(direction);
      if (deltaX === 0 && deltaY === 0) {
        return;
      }
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
            x: clampCoordinate(element.x + deltaX, "x", datasetBounds),
            y: clampCoordinate(element.y + deltaY, "y", datasetBounds)
          };
        });
        const nextScreens = [...current.screens];
        nextScreens[index] = { ...screen, elements };
        return { ...current, screens: nextScreens };
      });
    },
    [datasetBounds, orientation, selectedElementId, selectedScreenId]
  );

  const handleClampAllOverflow = useCallback(() => {
    setDataset((current) => {
      const result = clampDatasetToDisplay(current, datasetBounds);
      if (result.corrections.length === 0) {
        return current;
      }
      announceClampCorrections(result.corrections, "running “Clamp all to display”");
      return result.dataset;
    });
  }, [announceClampCorrections, datasetBounds]);

  const handleClampElement = useCallback(
    (elementId: string) => {
      if (!selectedScreenId) {
        return;
      }
      setDataset((current) => {
        const screenIndex = current.screens.findIndex((screen) => screen.id === selectedScreenId);
        if (screenIndex === -1) {
          return current;
        }
        const screen = current.screens[screenIndex];
        const elementIndex = screen.elements.findIndex((element) => element.id === elementId);
        if (elementIndex === -1) {
          return current;
        }
        const { element: clampedElement, adjustments } = clampElementGeometry(screen.elements[elementIndex], datasetBounds);
        if (adjustments.length === 0) {
          return current;
        }
        const nextElements = [...screen.elements];
        nextElements[elementIndex] = clampedElement;
        const nextScreens = [...current.screens];
        nextScreens[screenIndex] = { ...screen, elements: nextElements };
        announceClampCorrections(
          [
            {
              screenId: screen.id,
              elementId,
              adjustments
            }
          ],
          `fixing ${elementId}`
        );
        return { ...current, screens: nextScreens };
      });
    },
    [announceClampCorrections, datasetBounds, selectedScreenId]
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

  const handleAddEvent = useCallback((screenId: string) => {
    setDataset((current) => {
      const screenIndex = current.screens.findIndex((s) => s.id === screenId);
      if (screenIndex === -1) {
        return current;
      }
      const screen = current.screens[screenIndex];
      const nextEvents = [...(screen.events ?? []), { trigger: "btn_a_click" }];
      const nextScreens = [...current.screens];
      nextScreens[screenIndex] = { ...screen, events: nextEvents };
      return { ...current, screens: nextScreens };
    });
  }, []);

  const handleUpdateEvent = useCallback(
    (screenId: string, index: number, updates: Partial<ScreenEvent>) => {
      setDataset((current) => {
        const screenIndex = current.screens.findIndex((s) => s.id === screenId);
        if (screenIndex === -1) {
          return current;
        }
        const screen = current.screens[screenIndex];
        const nextEvents = [...(screen.events ?? [])];
        if (index < 0 || index >= nextEvents.length) {
          return current;
        }
        nextEvents[index] = { ...nextEvents[index], ...updates };
        const nextScreens = [...current.screens];
        nextScreens[screenIndex] = { ...screen, events: nextEvents };
        return { ...current, screens: nextScreens };
      });
    },
    []
  );

  const handleRemoveEvent = useCallback((screenId: string, index: number) => {
    setDataset((current) => {
      const screenIndex = current.screens.findIndex((s) => s.id === screenId);
      if (screenIndex === -1) {
        return current;
      }
      const screen = current.screens[screenIndex];
      const nextEvents = [...(screen.events ?? [])];
      nextEvents.splice(index, 1);
      const nextScreens = [...current.screens];
      nextScreens[screenIndex] = { ...screen, events: nextEvents };
      return { ...current, screens: nextScreens };
    });
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
        await handleLoadManifest(file);
      } catch (error) {
        setManifestFeedback({
          status: "error",
          message: error instanceof Error ? error.message : String(error)
        });
      } finally {
        event.target.value = "";
      }
    },
    [handleLoadManifest]
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
  const overflowElementIds = useMemo(
    () => new Set(overflow.map((item) => item.element.id)),
    [overflow]
  );

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

  /**
   * The navigation stack, mirroring firmware `UiNavigator`.
   *
   * The simulator used to follow `flow.targetScreenId` and nothing else, which cannot express
   * "back": the firmware POPS a level, so 14 of the dataset's 68 back/escape flows deliberately
   * carry no static target at all — `nyquist-warning`'s were removed with the note "Static targets
   * cannot express 'the editor we came from'". Pressing ENTER on any BACK page therefore did
   * nothing, which is what the owner reported.
   *
   * Holds the PARENT ids; the currently selected screen is the top of the level. Depth is capped at
   * `UiNavigator::kMaxDepth` (5) so the simulator refuses a descent the device would refuse too —
   * otherwise a pack could look navigable here and dead-end on hardware.
   */
  const kMaxNavDepth = 5;
  const kRootScreenId = "info-p0-global-status";
  const [navParents, setNavParents] = useState<string[]>([]);
  /**
   * The stack, readable from inside handleButtonEvent without being a dependency.
   *
   * handleButtonEvent is memoised, and adding `navParents` to its dependency array would rebuild it
   * on every navigation. Leaving it OUT while reading the state directly is worse: the callback would
   * capture the array from the render that created it, so a second descent would push the parent from
   * before the first — losing a level while looking like it worked. A ref is read at call time, so
   * neither happens.
   */
  const navParentsRef = useRef<string[]>([]);
  const pushNavParent = useCallback((parent: string) => {
    navParentsRef.current = [...navParentsRef.current, parent];
    setNavParents(navParentsRef.current);
  }, []);
  const popNavParent = useCallback((): string | undefined => {
    const parent = navParentsRef.current[navParentsRef.current.length - 1];
    if (parent === undefined) {
      return undefined;
    }
    navParentsRef.current = navParentsRef.current.slice(0, -1);
    setNavParents(navParentsRef.current);
    return parent;
  }, []);
  const clearNavParents = useCallback(() => {
    navParentsRef.current = [];
    setNavParents([]);
  }, []);

  /**
   * Simulated backlight and Select Menu state.
   *
   * Both are device MODE, not values: the firmware has no catalogue entry for either, and adding one
   * would mean a firmware catalogue edit plus a manifest regeneration — two CI gates — to report
   * something the simulator already knows. So they live here and are drawn as panel chrome, never as an
   * element inside the 240x135 area. The device draws nothing at all while idle (`setBacklight(false)`
   * then `fillScreen`), which is why the dataset's "- Display off -" text is not what we render.
   *
   * Refs alongside the state for the same reason as `navParentsRef`: handleButtonEvent is memoised and
   * must read the current value without being rebuilt on every change.
   */
  const [displayOn, setDisplayOn] = useState<boolean>(true);
  const displayOnRef = useRef<boolean>(true);
  const setDisplay = useCallback((on: boolean) => {
    displayOnRef.current = on;
    setDisplayOn(on);
  }, []);
  const [selectorOpen, setSelectorOpen] = useState<boolean>(false);
  const selectorOpenRef = useRef<boolean>(false);
  const setSelector = useCallback((open: boolean) => {
    selectorOpenRef.current = open;
    setSelectorOpen(open);
  }, []);

  /* ---- Resolving a binding against device memory ------------------------------------------------ */

  /**
   * Which sensor the shared `config.sensor.*` editors describe.
   *
   * Navigation decides it, exactly as `UiNavigator::descend` reads the index off the screen being left;
   * the manual pick only fills in when no level implies a sensor. 1-based, 0 for none — the navigator's
   * own sentinel, which is why the values panel can show "no sensor selected" rather than a plausible 1.
   */
  const navSensorIndex = sensorIndexForScreen(selectedScreenId, navParents);
  const selectedSensor = navSensorIndex !== 0 ? navSensorIndex : pickedSensor;

  /**
   * The aggregates, with the firmware's own formats and its own summation set.
   *
   * `SensorStateEngine::update` accumulates `totalSessionLiters` and `aggregateFlowLps` INSIDE
   * `if (sensor.inUse)` (sensor_state_engine.cpp:21-49), so a disconnected channel contributes to
   * neither — which is what makes switching one off visible on P0. Two details worth stating because I
   * got both wrong first time round: the volume aggregate is SESSION litres, not cumulative, and
   * `telemetry.totalVolumeLiters` is the same quantity as the volume inside `telemetry.total`
   * (ui_bindings.cpp:233-235). Formats are the firmware's, quoted:
   *   telemetry.total   -> "Total %.2f L | Flow %.2f L/s"   (ui_bindings.cpp:224)
   *   telemetry.status  -> "%u warning%s" or "All sensors ready" (ui_bindings.cpp:237-244)
   *
   * P0 is the screen the app opens on, and `telemetry.total` is the only aggregate any element binds —
   * so while these came from the sample table, disconnecting every sensor left the landing screen
   * cheerfully reporting 1234.56 L.
   */
  const resolvedValues = useMemo(() => {
    const out: Record<string, string> = {};
    const inUse = sensors.filter((sensor) => sensor.connected);
    const aggregateFlowLps = inUse.reduce((sum, sensor) => sum + sensor.instantFlowLps, 0);
    const totalSessionLiters = inUse.reduce((sum, sensor) => sum + sensor.sessionLiters, 0);
    const warnings = warningSensorNumbers(sensors).length;

    const aggregate = (id: string): string | undefined => {
      switch (id) {
        case "telemetry.total":
          return `Total ${totalSessionLiters.toFixed(2)} L | Flow ${aggregateFlowLps.toFixed(2)} L/s`;
        case "telemetry.totalFlowLps":
          return aggregateFlowLps.toFixed(2);
        case "telemetry.totalVolumeLiters":
          return totalSessionLiters.toFixed(2);
        case "telemetry.status":
          return warnings > 0 ? `${warnings} warning${warnings === 1 ? "" : "s"}` : "All sensors ready";
        default:
          return undefined;
      }
    };

    for (const binding of manifestValueBindings) {
      // MEMORY FIRST. A pin only fills in where memory is silent — the reverse order meant one keystroke
      // in this panel permanently outranked the device's own state, which is the bug the round exists to
      // remove rather than relocate.
      out[binding.id] =
        resolveSensorBinding(binding.id, sensors, selectedSensor) ??
        aggregate(binding.id) ??
        pinnedValues[binding.id] ??
        sampleValueFor(binding.id, manifestValueById.get(binding.id), "sample");
    }
    return out;
  }, [manifestValueBindings, manifestValueById, pinnedValues, selectedSensor, sensors]);

  /** True when device memory answers this binding, so nothing may pin over it. */
  const memoryOwnsBinding = useCallback(
    (bindingId: string) =>
      resolveSensorBinding(bindingId, sensors, selectedSensor) !== undefined ||
      bindingId.startsWith("telemetry."),
    [selectedSensor, sensors]
  );

  /** Editable in the values panel: a setting, and for a per-sensor one, only with a sensor selected. */
  const canEditBinding = useCallback(
    (bindingId: string) => {
      const definition = manifestValueById.get(bindingId);
      if (isPerSensorSetting(definition)) {
        return selectedSensor !== 0;
      }
      // Everything memory owns is a READING on the device — read-only there, so read-only here. The
      // panel used to render all 56 per-sensor readings as text inputs.
      return definition?.category === "setting" && !memoryOwnsBinding(bindingId);
    },
    [manifestValueById, memoryOwnsBinding, selectedSensor]
  );

  /** What the device would draw for one sensor's flow row — the visible effect of a toggle. */
  const sensorPreview = useCallback(
    (sensorNumber: number) =>
      resolveSensorBinding(`sensor.${sensorNumber}.instantFlow`, sensors, selectedSensor) ?? "—",
    [selectedSensor, sensors]
  );

  const handleSensorFieldChange = useCallback(
    (sensorNumber: number, field: "connected" | "ready", value: boolean) => {
      setSensors((table) => setSensor(table, sensorNumber, { [field]: value }));
    },
    []
  );

  const handleSelectSensor = useCallback((sensorNumber: number) => {
    setPickedSensor((current) => (current === sensorNumber ? 0 : sensorNumber));
  }, []);

  /**
   * An edit writes MEMORY when memory owns the fact, and pins the string otherwise.
   *
   * The four per-sensor settings are stored integers on the device (`SensorCharacteristics` is three
   * int fields plus a bitmap bit), so they are parsed rather than kept as text — which is what makes
   * "set sensor 1 disconnected" change the eight telemetry rows too, instead of only the settings page.
   */
  const handleMemoryWrite = useCallback(
    (bindingId: string, value: string) => {
      const definition = manifestValueById.get(bindingId);
      if (isPerSensorSetting(definition)) {
        if (selectedSensor === 0) {
          return;
        }
        if (bindingId === "config.sensor.connected") {
          setSensors((table) =>
            setSensor(table, selectedSensor, { connected: /^(on|1|true|yes)$/i.test(value.trim()) })
          );
          return;
        }
        // Clamped to the stored type's range, because the panel must not render a string the device
        // cannot emit: q_max is a uint16_t and the other two are int16_t (sensor_types.h:6-8).
        const numeric = Number.parseInt(value, 10);
        if (!Number.isFinite(numeric)) {
          return;
        }
        if (bindingId === "config.sensor.multiplier" || bindingId === "config.sensor.adjust") {
          const field = bindingId === "config.sensor.multiplier" ? "multiplier" : "adjust";
          setSensors((table) =>
            setSensor(table, selectedSensor, { [field]: clamp(numeric, -32768, 32767) })
          );
          return;
        }
        if (bindingId === "config.sensor.maxFlow") {
          setSensors((table) => setSensor(table, selectedSensor, { qMaxLpm: clamp(numeric, 0, 65535) }));
        }
        // No trailing else: an unrecognised per-sensor setting is DROPPED rather than being written to
        // whichever field the chain happened to end on. A fifth entry in the manifest would otherwise
        // silently land in qMaxLpm.
        return;
      }
      if (memoryOwnsBinding(bindingId)) {
        return;  // read-only: memory is the authority for it
      }
      setPinnedValues((current) => ({ ...current, [bindingId]: value }));
    },
    [manifestValueById, memoryOwnsBinding, selectedSensor]
  );

  /**
   * One pass of the loop: advance every channel through the state engine's rules.
   *
   * Pulses, not flow, because that is what the device counts — `pulsesForFlow` inverts the engine's two
   * lines so a target flow can drive it. A disconnected or not-ready channel is still ticked: the engine
   * zeroes its instant flow and leaves its totals frozen, and seeing frozen totals is the point.
   */
  const handleLoopTick = useCallback(() => {
    setSensors((table) =>
      table.map((sensor) => {
        // Inside the channel's OWN ceiling. A flat 0..4 L/s target exceeded the default q_max of
        // 150 L/min (2.5 L/s), so the engine clamped and several rows sat at exactly 2.50 for runs of
        // ticks — indistinguishable from a frozen row, which is the one distinction this panel exists
        // to make legible.
        const ceilingLps = sensor.qMaxLpm / 60;
        const target = sensor.connected && sensor.ready ? Math.random() * ceilingLps : 0;
        return advanceSensorTick(sensor, {
          pulses: pulsesForFlow(sensor, target, loopIntervalMs),
          elapsedMs: loopIntervalMs
        });
      })
    );
  }, [loopIntervalMs]);

  useEffect(() => {
    if (!loopRunning) {
      return;
    }
    const handle = setInterval(handleLoopTick, loopIntervalMs);
    return () => clearInterval(handle);
  }, [handleLoopTick, loopIntervalMs, loopRunning]);

  const handleMemoryReset = useCallback(() => {
    setSensors(createSensorTable());
    setPinnedValues({});
  }, []);

  const handleButtonEvent = useCallback(
    (event: SimulatedButtonEvent) => {
      const triggerLabel = formatTriggerLabel(event);
      const effect = deriveTransitionEffect(event);
      let activeScreenId = selectedScreen?.id ?? screens[0]?.id ?? "—";

      /* ── Multi-button gestures, BEFORE flow matching ──────────────────────────────────────
       *
       * Order mirrors interaction_handler::update, which runs handleSelectorCombo and
       * handleDisplayOffCombo before its event queue drains. The combo branch used to run LAST, after
       * generic flow matching — and since no dataset flow can match `up+down` (the flow schema's
       * `button` is a single-value enum), every combo ALSO recorded "No matching flow". A 3 s hold
       * produced four trace entries.
       *
       * Neither gesture has a dispatchable actionId: FlowButton cannot express a combo, so the
       * firmware handles both outside the action registry. The trace entry is therefore SYNTHESIZED,
       * and uses the one catalogued id that names the effect — `ui.action.mode.idle` / "Enter idle" —
       * marked firmware-internal so it does not imply a dataset binding that does not exist.
       */
      if (event.button === "up+down" || event.button === "up+down+enter") {
        if (event.button === "up+down" && event.kind === "short") {
          // enterIdle(): backlight off, editor discarded, navigation stack cleared, page reset to P0.
          // The stack clear is the load-bearing half — without it the display would wake with a stale
          // parent stack and BACK would ascend into screens the device has forgotten.
          clearNavParents();
          // enterIdle does NOT close the pack selector — ui_controller.cpp:92-96 ends the edit, escapes
          // the navigator, sets the page and the mode, and never touches the selector. So the device
          // blanks with the Select Menu still active and the next press wakes back into it, not P0.
          setDisplay(false);
          const resolvedId = selectById(kRootScreenId);
          if (resolvedId) {
            activeScreenId = resolvedId;
          }
          recordTraceEntry({
            id: "ui.action.mode.idle",
            label: "Display off — navigation reset to P0, editor discarded",
            functionName: "Enter idle",
            trigger: triggerLabel,
            screenId: selectedScreen?.id ?? "unknown",
            screenName: selectedScreen?.name,
            targetScreenId: kRootScreenId,
            notes: "firmware-internal: no flow, no actionId (interaction_handler.cpp handleDisplayOffCombo)"
          });
        } else if (event.button === "up+down+enter" && event.kind === "long") {
          setSelector(true);
          recordTraceEntry({
            id: "ui.selector.open",
            label: "Select Menu opened (firmware-drawn page)",
            functionName: "Open pack selector",
            trigger: triggerLabel,
            screenId: selectedScreen?.id ?? "unknown",
            screenName: selectedScreen?.name,
            notes: "ui_renderer draws this before every table-driven path, so the dataset cannot preview it"
          });
        }
        appendLog(
          `[${new Date(event.timestamp).toLocaleTimeString()}] ${triggerLabel} → ${activeScreenId}`
        );
        return;
      }

      /* ── While the Select Menu is open, the firmware owns all three buttons ───────────────
       * UP/DOWN move the selector cursor on any event kind, ENTER-short commits, ENTER-long closes
       * without selecting. None of it reaches flow matching.
       */
      if (selectorOpenRef.current) {
        const closes = event.button === "enter";
        if (closes) {
          setSelector(false);
        }
        recordTraceEntry({
          id: closes ? "ui.selector.close" : "ui.selector.move",
          label: closes
            ? event.kind === "long"
              ? "Select Menu closed without selecting"
              : "Select Menu choice committed (the device reboots into the pack)"
            : "Select Menu cursor moved",
          functionName: "Pack selector",
          trigger: triggerLabel,
          screenId: selectedScreen?.id ?? "unknown",
          screenName: selectedScreen?.name,
          notes: "consumed by the firmware-drawn selector"
        });
        appendLog(
          `[${new Date(event.timestamp).toLocaleTimeString()}] ${triggerLabel} → selector`
        );
        return;
      }

      /* ── A press while the display is off wakes it ────────────────────────────────────────
       * Mirroring the firmware AS BUILT, not the spec: `buttonInput.update()` and
       * `interactionHandler.update()` run every loop pass regardless of UiMode, and nothing drops the
       * waking event — so the press that wakes the panel ALSO dispatches. Spec §3.1 and the dataset's
       * state-idle wake flows say the press should be swallowed and the device wake on P0 unchanged;
       * the two cannot both be previewed, and this simulator previews the device that ships.
       */
      if (!displayOnRef.current) {
        setDisplay(true);
        recordTraceEntry({
          id: "ui.display.wake",
          label: "Display woke — the waking press still dispatches (firmware as built)",
          functionName: "Wake",
          trigger: triggerLabel,
          screenId: selectedScreen?.id ?? "unknown",
          screenName: selectedScreen?.name,
          notes: "spec §3.1 would swallow this press; firmware does not"
        });
      }

      const resolvedFlows = findMatchingButtonFlows(selectedScreen, event);
      if (resolvedFlows.length > 0) {
        resolvedFlows.forEach((flow) => {
          const actionDefinition = flow.actionId ? actionCatalog.get(flow.actionId) : undefined;
          const action = flow.actionId ?? "";
          const from = selectedScreen?.id;

          // Dispatch on the ACTION first. Following targetScreenId alone is what made BACK dead: the
          // firmware resolves "one level up" from its stack, so the dataset has no target to follow.
          if (action === "ui.action.nav.back") {
            // ascend(): pop one level. A no-op at the root, exactly as UiNavigator::ascend returns
            // false there rather than underflowing.
            const parent = popNavParent();
            if (parent !== undefined) {
              const resolvedId = selectById(parent);
              if (resolvedId) {
                activeScreenId = resolvedId;
              }
            }
          } else if (action === "ui.action.nav.escape") {
            // escape(): clear the stack and land on P0, whatever the depth.
            clearNavParents();
            const resolvedId = selectById(flow.targetScreenId ?? kRootScreenId);
            if (resolvedId) {
              activeScreenId = resolvedId;
            }
          } else if (action === "config.action.value.commit" ||
                     action === "config.action.value.discard") {
            // Both ascend in the firmware. The dataset also names the parent, so following either
            // lands in the same place — popping keeps the DEPTH right, which following would not.
            const parent = popNavParent();
            if (parent !== undefined) {
              const resolvedId = selectById(flow.targetScreenId ?? parent);
              if (resolvedId) {
                activeScreenId = resolvedId;
              }
            } else if (flow.targetScreenId) {
              const resolvedId = selectById(flow.targetScreenId);
              if (resolvedId) {
                activeScreenId = resolvedId;
              }
            }
          } else if (action === "ui.action.nav.descend" && flow.targetScreenId) {
            // descend(): push, and REFUSE past the cap rather than silently going deeper than the
            // device can.
            if (navParentsRef.current.length + 1 >= kMaxNavDepth) {
              recordTraceEntry({
                id: "ui.action.nav.descend",
                label: `refused: depth ${kMaxNavDepth} reached (UiNavigator::kMaxDepth)`,
                functionName: actionDefinition?.label,
                trigger: `${event.button}.${event.kind}`,
                screenId: from ?? "unknown",
                screenName: selectedScreen?.name,
                actionParams: flow.actionParams ?? null,
                targetScreenId: flow.targetScreenId
              });
              return;
            }
            if (from) {
              pushNavParent(from);
            }
            const resolvedId = selectById(flow.targetScreenId);
            if (resolvedId) {
              activeScreenId = resolvedId;
            }
          } else if (flow.targetScreenId) {
            // Everything else — paging with UP/DOWN above all — is a SIBLING move: the level is
            // unchanged, so the stack must not move either. Treating paging as a descent was the
            // other half of why depth never made sense here.
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
      /* The retired factory-reset combo used to be handled here — `up+down`.long recorded a
       * "Factory reset countdown started" entry and previewed a transition to `countdown-factory-reset`.
       * Deleted, for three separate reasons:
       *   - the gesture is gone: Display_UI_Requirements §3.3 retired it and interaction_handler.cpp:226
       *     confirms "The blind UP+DOWN 30 s arming combo is GONE";
       *   - it fired for UP+DOWN+ENTER too, since the hook never looked at ENTER;
       *   - `countdown-factory-reset` is not in the shipped dataset, but it IS in
       *     tests/fixtures/legacy-screens.json — so with that dataset imported the retired gesture
       *     really did navigate to a factory-reset countdown.
       * Factory reset is reachable only the way the device reaches it: P8 → confirm screen → ENTER held.
       */
      appendLog(
        `[${new Date(event.timestamp).toLocaleTimeString()}] ${triggerLabel} → ${activeScreenId}`
      );
    },
    [actionCatalog, appendLog, previewTransition, recordTraceEntry, selectById, selectByOffset, selectedScreen, screens, pushNavParent, popNavParent, clearNavParents, setDisplay, setSelector]
  );

  const { pressed, armedCombo, press, release, cancelAll } = useSimulatedButtons(handleButtonEvent);

  const mapKeyToButton = useCallback((key: string): "up" | "down" | "enter" | undefined => {
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
                </section>

                <section className="viewport-container">
                  {layoutReport ? (
                    <DisplayViewport
                      layout={layoutReport}
                      zoomPercent={zoom}
                      showGrid={showGrid}
                      valueOverrides={selectedScreenOverrides}
                      boundValues={resolvedValues}
                      powered={displayOn}
                      // The transition overlay is OFF. It faded a miniature of the incoming screen
                      // over the panel on every UP/DOWN, which on a device whose whole navigation IS
                      // UP/DOWN meant an animation over almost every press — and it obscured the
                      // thing the viewport exists to show. The firmware draws no such transition; it
                      // clears and repaints. Keeping a preview the device cannot produce made the
                      // panel less faithful, not more.
                      pendingTransition={undefined}
                      scrollIndicator={scrollIndicator}
                      firmwareValues={firmwareManifest.values ?? []}
                    />
                  ) : (
                    <p>No screen selected.</p>
                  )}
                  {/* Chrome, deliberately outside the 240x135 frame: the device draws nothing at all
                      while idle, so anything explanatory has to live here or it is a lie about pixels. */}
                  <p
                    className={displayOn ? "viewport-power viewport-power--on" : "viewport-power"}
                    role="status"
                    aria-live="polite"
                  >
                    <span className="viewport-power__lamp" aria-hidden="true" />
                    {displayOn
                      ? "Display on"
                      : "Display off — backlight off, framebuffer cleared. Any button wakes it."}
                  </p>
                </section>

                <ButtonPanel
                  pressed={pressed}
                  armedCombo={armedCombo}
                  displayOn={displayOn}
                  selectorOpen={selectorOpen}
                  onPressStart={press}
                  onPressEnd={release}
                />

                <ValuePlaceholderPanel
                  screen={selectedScreen}
                  overrides={selectedScreenOverrides}
                  onChange={handleValueChange}
                  onRevert={handleValueRevert}
                  onSave={handleValueSave}
                />

                {/* Controls only. The value editors are under the Function trace — see the trace
                    column below — so the display and the values no longer compete for one column. */}
                <FirmwareLoopPanel
                  running={loopRunning}
                  intervalMs={loopIntervalMs}
                  displayOn={displayOn}
                  selectorOpen={selectorOpen}
                  connectedSensors={connectedSensorCount}
                  sensorCount={kSensorCount}
                  onRunningChange={setLoopRunning}
                  onIntervalChange={setLoopIntervalMs}
                  onSingleTick={handleLoopTick}
                  onResetValues={handleMemoryReset}
                />

                <section className="layout-warnings">
                  <strong>Layout diagnostics</strong>
                  {clampNotice ? (
                    <div className="layout-correction-alert">
                      <div>
                        <p>
                          {clampNotice.total} element{clampNotice.total === 1 ? "" : "s"}{" "}
                          {clampNotice.total === 1 ? "was" : "were"} clamped while {clampNotice.context}.
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
                  <span>
                    Build: {APP_VERSION}
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
                {/* Under the trace, as asked. This column had several hundred px of unused height
                    below a trace list capped at 360px, and the values had none. */}
                <FirmwareValuesPanel
                  bindings={manifestValueBindings}
                  values={resolvedValues}
                  onValueChange={handleMemoryWrite}
                  canEdit={canEditBinding}
                  sensors={sensors}
                  selectedSensor={selectedSensor}
                  selectionFromNavigation={navSensorIndex !== 0}
                  sensorPreview={sensorPreview}
                  onSensorFieldChange={handleSensorFieldChange}
                  onSelectSensor={handleSelectSensor}
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
                <section className="controls design-controls">
                  <div className="labelled-control">
                    <label>Orientation</label>
                    <div className="orientation-group">
                      {/*
                        Landscape only. Decision D3 fixed the panel at 240x135 and every screen in the
                        dataset is laid out for it; the Portrait button that used to sit here produced
                        a 135x240 canvas the device cannot produce, so any layout judged in it was
                        judged against a shape that does not exist. It also silently reported
                        out-of-bounds elements as fitting, which is how a portrait-clamped dataset
                        once mutated 49 of 375 elements.

                        Kept as a disabled control rather than deleted so the constraint is visible:
                        an absent button invites someone to add one back.
                      */}
                      <button type="button" className="active" disabled>
                        Landscape 240x135
                      </button>
                      <span
                        style={{ marginLeft: 8, fontSize: 11, color: "#94a3b8", alignSelf: "center" }}
                      >
                        fixed by decision D3
                      </span>
                    </div>
                  </div>
                </section>
                <div className="design-main-grid">
                  <ThemeEditor
                    layout={layoutReport}
                    zoomPercent={zoom}
                    showGrid={showGrid}
                    screen={selectedScreen}
                    firmwareValues={firmwareManifest.values ?? []}
                    previewFooter={
                      <section className="viewport-controls" aria-label="Viewport controls">
                        <h4>Viewport movement</h4>
                        <div className="element-move-pad" role="group">
                          <span aria-hidden="true" />
                          <button
                            type="button"
                            className="tool-button tool-button--secondary"
                            data-testid="element-nudge-up"
                            onClick={() => handleNudgeSelectedElement("up")}
                            disabled={!selectedElementId}
                          >
                            ↑
                          </button>
                          <span aria-hidden="true" />
                          <button
                            type="button"
                            className="tool-button tool-button--secondary"
                            data-testid="element-nudge-left"
                            onClick={() => handleNudgeSelectedElement("left")}
                            disabled={!selectedElementId}
                          >
                            ←
                          </button>
                          <span aria-hidden="true" />
                          <button
                            type="button"
                            className="tool-button tool-button--secondary"
                            data-testid="element-nudge-right"
                            onClick={() => handleNudgeSelectedElement("right")}
                            disabled={!selectedElementId}
                          >
                            →
                          </button>
                          <span aria-hidden="true" />
                          <button
                            type="button"
                            className="tool-button tool-button--secondary"
                            data-testid="element-nudge-down"
                            onClick={() => handleNudgeSelectedElement("down")}
                            disabled={!selectedElementId}
                          >
                            ↓
                          </button>
                          <span aria-hidden="true" />
                        </div>
                        <div className="element-move-actions">
                          <button
                            type="button"
                            className="tool-button tool-button--secondary"
                            onClick={handleClampAllOverflow}
                            disabled={overflowElementIds.size === 0}
                            data-testid="element-clamp-all"
                          >
                            Clamp all to display
                          </button>
                          <p className="element-move-hint">
                            Keyboard arrows follow the live viewport orientation while the Design tab is active.
                          </p>
                        </div>
                      </section>
                    }
                    sidebarContent={
                      <DesignToolbox
                        screen={selectedScreen}
                        onAddElement={handleAddElement}
                        onRemoveElement={handleRemoveElement}
                        onUpdateElement={handleUpdateElement}
                        selectedElementId={selectedElementId}
                        onSelectElement={setSelectedElementId}
                        onClampElement={handleClampElement}
                        overflowElementIds={overflowElementIds}
                        maxCoordinateX={layoutReport?.bounds.width ?? DISPLAY_WIDTH}
                        maxCoordinateY={layoutReport?.bounds.height ?? DISPLAY_HEIGHT}
                        maxWidth={layoutReport?.bounds.width ?? DISPLAY_WIDTH}
                        maxHeight={layoutReport?.bounds.height ?? DISPLAY_HEIGHT}
                        maxInputLength={MAX_INPUT_LENGTH}
                        onLoadManifest={handleLoadManifest}
                        firmwareActions={firmwareManifest.actions}
                        firmwareValues={firmwareManifest.values ?? []}
                        screens={dataset.screens}
                        onAddEvent={handleAddEvent}
                        onUpdateEvent={handleUpdateEvent}
                        onRemoveEvent={handleRemoveEvent}
                      />
                    }
                    stackControls
                  />
                </div>
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
                  dataset={dataset}
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
                <HelpPanel
                  dataset={dataset}
                  selectedScreen={selectedScreen}
                  onNavigateToTab={setActivePanel}
                />
              </div>
            </div>
          </section>
        )}
      </main>
    </div>
  );
}

export default App;
