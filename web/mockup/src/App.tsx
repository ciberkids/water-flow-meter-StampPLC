import { ChangeEvent, useCallback, useEffect, useMemo, useRef, useState } from "react";
import "./App.css";
import packageJson from "../package.json";
import { DisplayViewport, PackSelectorEntry } from "./components/DisplayViewport";
import { movePackCursor, packCommitAction } from "./utils/packSelector";
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
import {
  adjustSettingRaw,
  formatSetting,
  pendingRawFor,
  rangeHintFor,
  sampleRawFor,
  settingOfScreen
} from "./utils/settingHints";
import { SchemaValidationError, validateDataset } from "./schema/validation";
import { ExporterPanel } from "./components/ExporterPanel";
import { SimulationTracePanel } from "./components/SimulationTracePanel";
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
  resetMeasured,
  resetSession,
  sensorAt,
  isPerSensorSetting,
  kSensorCount,
  pulsesForFlow,
  resolveSensorBinding,
  sensorIndexForScreen,
  type FlowUnit,
  flowFromLpm,
  flowUnitLabel,
  readSensorSettingRaw,
  setSensor,
  writeSensorSetting,
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
  /**
   * The same screen, readable from a live timer.
   *
   * §5.4's held ramp is driven by a self-rescheduling timeout inside `useSimulatedButtons`, which must
   * see the screen the operator is on NOW rather than the one captured when the press began.
   */
  const selectedScreenRef = useRef<ScreenDefinition | undefined>(selectedScreen);
  selectedScreenRef.current = selectedScreen;
  const totalScreens = screens.length;

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
      perSensor: value.perSensor,
      /**
       * The descriptor's fixed set, WITHOUT WHICH the panel cannot offer a dropdown.
       *
       * This mapping listed six fields and dropped `options`, so every row arrived with none and
       * `renderRow` fell through to its text-box branch — for booleans and enums alike. The
       * `<select>` branch existed, was commented, and could never render: changing a sensor from On
       * to Off meant typing "Off" exactly, and typing it wrong looked identical to typing it right,
       * because the box kept the text and the resolver ignored it.
       *
       * A projection that silently drops the one field a downstream branch depends on is the same
       * defect shape as a catalogue entry with no resolver arm — everything compiles, nothing works,
       * and the dead branch reads as evidence that it does.
       */
      options: value.options
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
  /**
   * How the loop drives flow, and at what figure when it is steady.
   *
   * The loop only ever drove each channel with a fresh `Math.random()`, which shows the panel moving and
   * makes the three aggregate readings impossible to check: the total, the peak and the accumulated
   * volume all change every tick, so there is nothing to compare a number against. Steady mode makes all
   * three arithmetic — `channels x flow`, `flow`, and `channels x flow x t / 60`.
   *
   * `steady` holds each channel at whatever its instant flow already is in memory, which is editable
   * per channel in the Device memory panel. There is deliberately no figure here: this control had one,
   * and it was a second home for the same fact that could only ever speak for all eight channels at once.
   * Switching to steady mid-run therefore also works as a freeze, at whatever the channels had reached.
   */
  const [flowMode, setFlowMode] = useState<"random" | "steady">("random");
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
  /**
   * The open value editor: which setting, and the value dialled up but not yet committed.
   *
   * `UiEditorState` (ui_controller.h) in miniature. Without it UP/DOWN on an editor screen did
   * nothing at all — `config.action.value.increment` and `.decrement` had no handler, so the panel
   * showed a fixed `New` value that no amount of pressing changed, on every one of the twelve
   * editors.
   *
   * `raw` is the stored integer, not the rendered string, for the same reason the device stores it
   * that way: stepping has to happen on the number, and `formatSetting` is what turns it into text.
   */
  const [editorState, setEditorState] = useState<{ bindingId: string; raw: number } | null>(null);
  const editorStateRef = useRef<typeof editorState>(null);
  editorStateRef.current = editorState;

  /**
   * The live hold countdown, when ENTER is being held on a screen that asks for one.
   *
   * Hold-to-confirm was DECLARED and never implemented: `types.ts` documents
   * `trigger: { type: "timeout", holdButton: "enter", durationMs }`, and nothing in the app looked at
   * it. So `countdown.value` sat at the static sample `3` — the reported symptom — and, worse, the
   * confirm screens could not be completed at all: holding ENTER on RESET SESSION? did nothing, and
   * the only way out was the short press that cancels.
   *
   * `remainingMs` rather than whole seconds, because the four confirms run 1.5 s, 3 s and 30 s and a
   * seconds-only counter cannot show the first one moving.
   */
  /**
   * How many times the simulated loop has repainted.
   *
   * The flow-dot chase advances ONE position per repaint, which is the only rate the device can
   * actually show. `drawFlowDots` derives a step period from the flow — 25 ms at the 10 L/s clamp —
   * but an info page repaints at 1 Hz, so that counter is sampled far slower than it advances and the
   * "chase" aliases into apparent randomness. Stepping per repaint makes the motion mean something:
   * one dot per frame the panel actually draws.
   */
  const [repaintCount, setRepaintCount] = useState(0);

  const [holdCountdown, setHoldCountdown] = useState<{ screenId: string; remainingMs: number; totalMs: number } | null>(null);
  const holdTimerRef = useRef<number | undefined>(undefined);

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

  /**
   * The Select Menu's contents and cursor (Loadable_UI_Menu_Packs §3.4).
   *
   * A simulated card, because the simulator has no SD slot — but shaped by the firmware's rules rather
   * than invented: index 0 is the built-in default, always present and always first, and it is the one
   * marked active until something else is selected. `PackSelector::kMaxEntries` is 8, chosen against the
   * panel rather than the filesystem, and `truncated` is what the device says out loud when the card held
   * more, so an operator whose pack is missing is told rather than left to suspect the card.
   */
  const [packEntries, setPackEntries] = useState<PackSelectorEntry[]>([
    { label: "Built-in", active: true },
    { label: "production", active: false },
    { label: "commissioning", active: false }
  ]);
  const [packCursor, setPackCursor] = useState(0);
  const packCursorRef = useRef(0);
  packCursorRef.current = packCursor;
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
   * `SensorStateEngine::update` accumulates `totalSessionLiters` and `aggregateFlowLpm` INSIDE
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
  /**
   * The aggregate flow, hoisted out of the resolver because the flow-dot chase needs it too.
   *
   * One computation, not two: the dots and `telemetry.totalFlowLpm` must agree about whether water is
   * moving, or the panel shows a still chase beside a non-zero reading.
   */
  const aggregateFlowLpm = useMemo(
    () => sensors.filter((sensor) => sensor.connected).reduce((sum, s) => sum + s.instantFlowLpm, 0),
    [sensors]
  );

  /**
   * The panel's flow unit, from `config.flowUnit`.
   *
   * A display preference: it changes what the flow pages draw and moves their header unit with them,
   * and changes nothing that is stored or published.
   */
  const flowUnit = useMemo<FlowUnit>(() => {
    const definition = manifestValueById.get("config.flowUnit");
    if (!definition) return 0;
    const pinned = pinnedValues["config.flowUnit"];
    const raw = pinned !== undefined
      ? definition.options?.find((o) => o.label === pinned.trim())?.value ?? Number.parseInt(pinned, 10)
      : sampleRawFor(definition);
    return (raw === 1 || raw === 2 ? raw : 0) as FlowUnit;
  }, [manifestValueById, pinnedValues]);

  const resolvedValues = useMemo(() => {
    const out: Record<string, string> = {};
    const inUse = sensors.filter((sensor) => sensor.connected);
    const totalSessionLiters = inUse.reduce((sum, sensor) => sum + sensor.sessionLiters, 0);
    const warnings = warningSensorNumbers(sensors).length;

    /**
     * The peak across enabled channels and which channel holds it, mirroring
     * `ui_bindings.cpp`'s `telemetry.maxFlowLpm` arm including its three states: nothing enabled,
     * enabled but no flow seen yet, and a real peak with an owner. §5a's purpose is spotting a
     * sensor under-dimensioned for its pipe, which needs the channel number and not just the number.
     */
    const maxFlowRow = (): string => {
      let owner = 0;
      let peak = 0;
      for (const sensor of inUse) {
        if (sensor.maxFlowLpm > peak) {
          peak = sensor.maxFlowLpm;
          owner = sensor.number;
        }
      }
      if (inUse.length === 0) {
        return "Max Flow: --";
      }
      if (owner === 0) {
        return `Max Flow: 0.00 ${flowUnitLabel(flowUnit)}`;
      }
      return `Max Flow: ${flowFromLpm(peak, flowUnit).toFixed(2).padStart(7)} ${flowUnitLabel(flowUnit)} (S${owner})`;
    };

    /**
     * P0's combined legend, from the same net strings its own rows use plus the LED pulse volume.
     *
     * Read through `pinnedValues` first so that pinning `net.wifi.state` to RETRY moves this row too
     * — the whole point of folding three facts onto one row is that they stay the same three facts.
     */
    const legendRow = (): string => {
      const part = (id: string) => pinnedValues[id] ?? sampleValueFor(id, manifestValueById.get(id), "sample");
      // The LED half reads the PIN too. Taking it from `sampleRawFor` alone meant pinning
      // config.ledPulseVolume to 100 showed `100 L` on C5 while P0 still said `LED 1p/10L` — the two
      // rows disagreeing about one setting, which is the whole thing folding them together was meant
      // to prevent. The pin is already formatted (`10 L`), so the unit suffix is stripped rather than
      // re-added.
      const led = part("config.ledPulseVolume").replace(/\s*L$/, "");
      return `WiFi ${part("net.wifi.state")}  MQTT ${part("net.mqtt.state")}  LED 1p/${led}L`;
    };

    /**
     * `L2 3/8` — depth, then which entry of the level this is.
     *
     * The position comes from the SIBLING ORDER in the screen hierarchy, not from walking the ring.
     * A ring is cyclic, so walking it from the current screen makes the current screen index 0 every
     * time — it can count the members but not say where you are among them. The hierarchy has a
     * canonical order because it is built from the descent that opened the level.
     *
     * Depth is the nav stack, mirroring `UiNavigator::depth()`.
     */
    const navPosition = (): string => {
      const depth = navParents.length;
      const id = selectedScreen?.id;
      if (!id) {
        return `L${depth}`;
      }
      const parent = hierarchy.parentMap.get(id);
      if (!parent) {
        return `L${depth}`;
      }
      const siblings = screens
        .filter((candidate) => hierarchy.parentMap.get(candidate.id) === parent)
        .map((candidate) => candidate.id);
      const index = siblings.indexOf(id);
      if (index < 0 || siblings.length <= 1) {
        return `L${depth}`;
      }
      return `L${depth} ${index + 1}/${siblings.length}`;
    };

    const aggregate = (id: string): string | undefined => {
      switch (id) {
        case "nav.position":
          return navPosition();
        // `%u s` of whole seconds remaining (ui_bindings.cpp). Rounded UP so a 1.5 s hold opens at
        // "2 s" and reaches "1 s" rather than starting at "1 s" and looking stuck.
        case "countdown.value":
          return holdCountdown && holdCountdown.screenId === selectedScreen?.id
            ? `${Math.ceil(holdCountdown.remainingMs / 1000)} s`
            : undefined;
        case "telemetry.total":
          return `Total ${totalSessionLiters.toFixed(2)} L | Flow ${aggregateFlowLpm.toFixed(2)} L/m`;
        // L/s is the DERIVED reading now (§2a moved storage to L/min); it stays for any consumer
        // bound to it that still wants per-second.
        case "telemetry.totalFlowLps":
          return (aggregateFlowLpm / 60).toFixed(2);
        // `%7.2f` on the device: a field width, so the column cannot shift under a growing integer
        // part. padStart reproduces it rather than approximating with a fixed prefix.
        case "telemetry.totalFlowLpm":
          return flowFromLpm(aggregateFlowLpm, flowUnit).toFixed(2).padStart(7);
        case "telemetry.flowUnitLabel":
          return flowUnitLabel(flowUnit);
        case "telemetry.totalVolumeLiters":
          return totalSessionLiters.toFixed(2);
        case "telemetry.totalVolumeM3":
          return (totalSessionLiters / 1000).toFixed(2);
        case "telemetry.maxFlowLpm":
          return maxFlowRow();
        case "legend.status":
          return legendRow();
        case "telemetry.status":
          return warnings > 0 ? `${warnings} warning${warnings === 1 ? "" : "s"}` : "All sensors ready";
        default:
          return undefined;
      }
    };

    /**
     * The two facts a setting page derives from the setting IT shows, rather than from a global.
     *
     * Both were single fixed strings, so `config.editor.range` read `1 to 247` on Baud Rate, Parity,
     * Stop Bits and every sensor page, and `config.editor.pending` read `19200` on Modbus ID, whose
     * domain stops at 247. They depend on the screen, so they are resolved here where the screen is
     * known — and only for a screen that actually shows a setting, so a reading page draws nothing.
     */
    const screenSetting = settingOfScreen(selectedScreen, manifestValueById);
    const editorDerived = (id: string): string | undefined => {
      if (id !== "config.editor.range" && id !== "config.editor.pending") {
        return undefined;
      }
      if (!screenSetting) {
        return "";
      }
      if (id === "config.editor.range") {
        return rangeHintFor(screenSetting);
      }
      // An OPEN editor's dialled-up value wins over the descriptor's sample, which is what makes
      // UP/DOWN visible on screen. Without this the pending line was a fixed sample and the
      // increment handler had nothing to show for its work.
      if (editorState?.bindingId === screenSetting.id) {
        return formatSetting(screenSetting, editorState.raw);
      }
      // A text setting has no numeric domain to step, so there is no pending value to invent; the
      // editor for one is a keyboard, which R5.3 does not give this device.
      if (screenSetting.type === "string") {
        return "";
      }
      return formatSetting(screenSetting, pendingRawFor(screenSetting));
    };

    for (const binding of manifestValueBindings) {
      // MEMORY FIRST. A pin only fills in where memory is silent — the reverse order meant one keystroke
      // in this panel permanently outranked the device's own state, which is the bug the round exists to
      // remove rather than relocate.
      out[binding.id] =
        resolveSensorBinding(binding.id, sensors, selectedSensor, undefined, flowUnit) ??
        aggregate(binding.id) ??
        editorDerived(binding.id) ??
        pinnedValues[binding.id] ??
        sampleValueFor(binding.id, manifestValueById.get(binding.id), "sample");
    }
    return out;
  }, [aggregateFlowLpm, editorState, flowUnit, hierarchy, holdCountdown, manifestValueBindings, manifestValueById, navParents, pinnedValues, screens, selectedScreen, selectedSensor, sensors]);

  /**
   * The stored integer a setting currently holds, from whichever home owns it.
   *
   * Per-sensor settings live in the sensor table; device-wide ones have no simulated store of their
   * own, so a pin is their memory and the descriptor's sample is the default. Reading through one
   * function keeps the editor from having to know which is which.
   */
  const currentRawFor = useCallback(
    (definition: FirmwareValueDefinition): number => {
      if (isPerSensorSetting(definition) && selectedSensor !== 0) {
        return readSensorSettingRaw(sensors, selectedSensor, definition);
      }
      const pinned = pinnedValues[definition.id];
      if (pinned !== undefined) {
        const byLabel = definition.options?.find((o) => o.label === pinned.trim());
        if (byLabel) return byLabel.value;
        const numeric = Number.parseInt(pinned, 10);
        if (Number.isFinite(numeric)) return numeric;
      }
      return sampleRawFor(definition);
    },
    [pinnedValues, selectedSensor, sensors]
  );

  /** Writes a committed editor value to whichever home owns the setting. */
  const commitEditorValue = useCallback(
    (definition: FirmwareValueDefinition, raw: number) => {
      const text = formatSetting(definition, raw);
      if (isPerSensorSetting(definition)) {
        if (selectedSensor === 0) {
          return;
        }
        setSensors((table) => writeSensorSetting(table, selectedSensor, definition, text));
        return;
      }
      // Device-wide settings have no simulated store, so the pin IS their memory. Storing the
      // FORMATTED string keeps the values panel and the panel itself showing the same text.
      setPinnedValues((current) => ({ ...current, [definition.id]: text }));
    },
    [selectedSensor]
  );

  /**
   * One step of an open editor, whether it came from a tap or from §5.4's held ramp.
   *
   * Both paths were about to grow their own copy of "find the screen's setting, read the pending value,
   * clamp or cycle it" — the two-homes shape this codebase keeps having to undo. `multiplier` is the only
   * difference between them, and it is exactly the difference the firmware expresses:
   * `magnitude = editor.setting->step * tier.multiplier`, with a tap being tier 1.
   *
   * Returns whether it stepped, so the caller can tell "no editor here" from "stepped".
   */
  const stepEditorValue = useCallback(
    (screen: ScreenDefinition | undefined, direction: 1 | -1, multiplier: number): boolean => {
      const setting = settingOfScreen(screen, manifestValueById);
      if (!setting) {
        return false;
      }
      const delta = direction * (setting.step ?? 1) * multiplier;
      const current =
        editorStateRef.current?.bindingId === setting.id
          ? editorStateRef.current.raw
          : currentRawFor(setting);
      const next = adjustSettingRaw(setting, current, delta);
      editorStateRef.current = { bindingId: setting.id, raw: next };
      setEditorState(editorStateRef.current);
      return true;
    },
    [currentRawFor, manifestValueById]
  );
  const stepEditorValueRef = useRef(stepEditorValue);
  stepEditorValueRef.current = stepEditorValue;

  /**
   * Whether a conditional screen is part of its level right now — `UiNavigator::screenVisible`.
   *
   * A screen with no `visibleWhen` is unconditional, which is every screen but the six the
   * calibration branch gates. An unresolvable gate leaves the screen VISIBLE, the same choice the
   * firmware makes: hiding a row because the question could not be asked would make a setting
   * unreachable, which is the failure the completeness rule exists to prevent.
   */
  const screenVisible = useCallback(
    (screen: ScreenDefinition | undefined): boolean => {
      if (!screen) return false;
      const gate = screen.visibleWhen;
      if (!gate) return true;
      const definition = manifestValueById.get(gate.binding);
      if (!definition) return true;
      if (isPerSensorSetting(definition)) {
        if (selectedSensor === 0) return true;
        return readSensorSettingRaw(sensors, selectedSensor, definition) === gate.equals;
      }
      const pinned = pinnedValues[definition.id];
      const raw = pinned !== undefined
        ? definition.options?.find((o) => o.label === pinned.trim())?.value ?? Number.parseInt(pinned, 10)
        : sampleRawFor(definition);
      return raw === gate.equals;
    },
    [manifestValueById, pinnedValues, selectedSensor, sensors]
  );

  /**
   * The next reachable screen in the ring, stepping over any that are hidden.
   *
   * `direction` walks the same DOWN chain either way: a ring closes, so "the one before X" is the last
   * visible member reached before coming back to X. There is no reverse chain to follow.
   */
  const nextVisibleInRing = useCallback(
    (from: string, direction: "down" | "up"): string | undefined => {
      const byIdLocal = new Map(screens.map((screen) => [screen.id, screen]));
      const step = (id: string): string | undefined =>
        byIdLocal.get(id)?.flows?.find(
          (flow) => flow.trigger.type === "button" && flow.trigger.button === "down" && flow.targetScreenId
        )?.targetScreenId;

      if (direction === "down") {
        let cursor = step(from);
        for (let hops = 0; cursor && hops < 32; hops += 1) {
          if (screenVisible(byIdLocal.get(cursor))) return cursor;
          if (cursor === from) return undefined;
          cursor = step(cursor);
        }
        return cursor;
      }
      // Walk the whole visible ring forward; the last member before returning to `from` is the one UP
      // wants.
      let cursor = step(from);
      let previous: string | undefined;
      for (let hops = 0; cursor && cursor !== from && hops < 32; hops += 1) {
        if (screenVisible(byIdLocal.get(cursor))) previous = cursor;
        cursor = step(cursor);
      }
      return previous;
    },
    [screenVisible, screens]
  );

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

  /**
   * Sets one channel's instantaneous flow. Clamped to that channel's own q_max on the way in.
   *
   * Clamped HERE rather than left to the engine so the box shows what the channel will actually read:
   * the engine clamps on the next tick anyway, so an unclamped field would display 500 until the loop
   * ran and then silently become 150.
   */
  const handleSensorFlowChange = useCallback(
    (sensorNumber: number, flowLpm: number) => {
      setSensors((table) => {
        const sensor = sensorAt(table, sensorNumber);
        if (!sensor) {
          return table;
        }
        const clamped = Math.min(Math.max(Number.isFinite(flowLpm) ? flowLpm : 0, 0), sensor.qMaxLpm);
        return setSensor(table, sensorNumber, { instantFlowLpm: clamped });
      });
    },
    []
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
        // Descriptor-driven, in sensorConfig. This used to be a chain naming four bindings by hand,
        // and its own comment predicted what went wrong: "a fifth entry in the manifest would
        // otherwise silently land in qMaxLpm". The calibration branch added a fifth and a sixth, so
        // typing Pulses/L into Calibration did nothing at all — silently, while the input kept
        // showing the typed text. Adding a per-sensor setting to the firmware catalogue now makes it
        // writable here with no change to this file.
        setSensors((table) => writeSensorSetting(table, selectedSensor, definition, value));
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
    // One repaint per tick, which is what paces the flow-dot chase. See the note on `repaintCount`.
    setRepaintCount((count) => count + 1);
    setSensors((table) =>
      table.map((sensor) => {
        /**
         * The channel's own ceiling, in L/MIN — the unit `pulsesForFlow` takes.
         *
         * This was `sensor.qMaxLpm / 60`, named `ceilingLps`, and handed straight to a function whose
         * parameter is `targetFlowLpm`. `pulsesForFlow` used to multiply by 60 to convert a caller's
         * L/s; when storage moved to L/min (§2a) that conversion was correctly deleted and this caller
         * was not updated, so every simulated run since has driven the channels at ONE SIXTIETH of the
         * intended flow. Eight channels topped out around 20 L/min where the hardware would be at 1200.
         *
         * Nothing caught it because nothing checked: with a random target per tick, no reading has a
         * value anyone can compare against. It surfaced the moment a steady flow made the aggregate
         * arithmetic — 8 channels at 60 L/min showing a total of 8.00 rather than 480.00.
         */
        const ceilingLpm = sensor.qMaxLpm;
        const live = sensor.connected && sensor.ready;
        /**
         * `steady` asks for the SAME figure on every channel and every tick, so the aggregate, the peak
         * and the volume can be checked by hand. It is still clamped to the channel's own ceiling — the
         * engine would clamp it anyway, and a channel with a smaller q_max visibly running slower than
         * its neighbours is the correct picture rather than a discrepancy to explain away.
         */
        const target = !live
          ? 0
          : flowMode === "steady"
            // Feeding the channel's own reading back through `pulsesForFlow` reproduces it exactly —
            // that round trip is unit-tested — so the flow holds and the volume, the peak and the
            // aggregate all derive from it.
            ? Math.min(sensor.instantFlowLpm, ceilingLpm)
            : Math.random() * ceilingLpm;
        return advanceSensorTick(sensor, {
          pulses: pulsesForFlow(sensor, target, loopIntervalMs),
          elapsedMs: loopIntervalMs
        });
      })
    );
  }, [flowMode, loopIntervalMs]);

  useEffect(() => {
    if (!loopRunning) {
      return;
    }
    const handle = setInterval(handleLoopTick, loopIntervalMs);
    return () => clearInterval(handle);
  }, [handleLoopTick, loopIntervalMs, loopRunning]);

  const handleMemoryReset = useCallback(() => {
    /**
     * Clears the MEASUREMENTS and keeps the configuration, which is the device's own
     * `reset-all-measured`. It used to call `createSensorTable()`, restoring the seeded 123.45 L and
     * 140.4 L/min — so the one control offering a clean slate handed back fabricated readings, and any
     * attempt to check the aggregates started from 0.99 m3 of volume that had never flowed.
     *
     * Calibration survives deliberately: it is what somebody is usually editing when they want a run
     * from zero, and the device keeps it too. A full return to defaults is a page reload.
     */
    setSensors((table) => resetMeasured(table));
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
          /**
           * `openPackSelector` does two more things, and both matter to what the operator sees.
           *
           * The cursor resets to the top, because `PackSelector::begin` rebuilds the list from scratch
           * every time rather than caching it — the card may have been swapped since the page was last
           * open, and a stale list would offer a pack that is no longer there. Keeping the cursor where
           * it was would point at a different pack than it did before.
           *
           * And any pending edit is DISCARDED (`endEdit()`), because §3.4.1's gesture is reachable from
           * inside a value editor, and committing a half-typed value on the way out would be worse than
           * losing it.
           */
          setPackCursor(0);
          editorStateRef.current = null;
          setEditorState(null);
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
        const activeIndex = Math.max(packEntries.findIndex((entry) => entry.active), 0);
        const commit =
          closes && event.kind !== "long"
            ? packCommitAction(packCursorRef.current, activeIndex)
            : null;
        if (closes) {
          setSelector(false);
          /**
           * A committed choice writes the pack pointer and REBOOTS. The simulator cannot reboot, so it
           * moves the active marker — which is what the operator would see on the way back — and the
           * trace entry carries what actually happened.
           *
           * `packCommitAction` is consulted rather than assumed, because selecting the pack that is
           * already running does NOTHING: the firmware refuses to write the same pointer and restart
           * into an identical UI, since that "would look like the device ignoring the press". A
           * simulator that moved a marker and logged a reboot there would be inventing an event.
           */
          if (commit && commit !== "nothing") {
            setPackEntries((current) =>
              current.map((entry, index) => ({ ...entry, active: index === packCursorRef.current }))
            );
          }
        } else if (event.button === "up" || event.button === "down") {
          /**
           * UP/DOWN move the cursor on ANY event kind, not just a tap.
           *
           * `handlePackSelector` drains the queue and switches on the button alone, so a long press and
           * a held repeat move it too. Clamped rather than wrapped, matching `PackSelector::moveCursor`.
           */
          const delta = event.button === "down" ? 1 : -1;
          // WRAPS, per `PackSelector::moveCursor`. Clamping was my first guess and it was wrong.
          setPackCursor((current) => movePackCursor(current, delta, packEntries.length));
        }
        recordTraceEntry({
          id: closes ? "ui.selector.close" : "ui.selector.move",
          label: closes
            ? event.kind === "long"
              ? "Select Menu closed without selecting"
              : commit === "nothing"
                ? "Select Menu: that pack is already running — no write, no reboot"
                : commit === "delete-pointer"
                  ? "Select Menu: pointer deleted, rebooting into the built-in menu"
                  : "Select Menu: pointer written, rebooting into the selected pack"
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
          // Any navigation out of an editor abandons its pending value. Leaving it set would let a
          // value dialled up on one screen reappear as the pending value of the next editor opened.
          if (action === "ui.action.nav.back" || action === "ui.action.nav.escape") {
            editorStateRef.current = null;
            setEditorState(null);
          }

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
            /**
             * Long ENTER ascends ONE level, not all of them.
             *
             * It used to clear the stack and land on P0 from any depth, so holding ENTER three levels
             * deep in the sensor settings threw the operator all the way out to the status page and
             * they had to walk back down to see the change they had just made. One level up is what a
             * hold means everywhere else in the tree — it is what the editor's `hold=cancel` already
             * did — and repeated holds still walk out to the top for anyone who wants that.
             *
             * The display-off gesture is unaffected: BtnA+BtnB resets navigation to P0 by its own
             * path (§3.1), which is a different thing and still resets fully.
             */
            const parent = popNavParent();
            if (parent !== undefined) {
              const resolvedId = selectById(parent);
              if (resolvedId) {
                activeScreenId = resolvedId;
              }
            }
            // At depth 0 there is nowhere to ascend to, so the press is deliberately a no-op rather
            // than a jump to a page the operator is already on.
          } else if (action === "ui.action.page.next" || action === "ui.action.page.previous") {
            /**
             * Ring stepping SKIPS hidden screens (`handlePageNext`/`handlePagePrevious`).
             *
             * The flow's own target is the next sibling in the DATASET, which the calibration branch
             * may have gated off. Following it blindly would land on a screen the level no longer
             * contains, drawing rows for a calibration form the operator is not using.
             */
            const direction = action === "ui.action.page.next" ? "down" : "up";
            let target = flow.targetScreenId;
            if (target && !screenVisible(screens.find((screen) => screen.id === target))) {
              target = nextVisibleInRing(selectedScreen?.id ?? kRootScreenId, direction);
            }
            if (target) {
              const resolvedId = selectById(target);
              if (resolvedId) {
                activeScreenId = resolvedId;
              }
            }
          } else if (action === "config.action.value.increment" ||
                     action === "config.action.value.decrement") {
            /**
             * UP/DOWN step the PENDING value; they do not navigate.
             *
             * Neither action had a handler, so on every editor screen UP/DOWN fell through to the
             * ring and the `New` line never moved. `adjustSettingRaw` mirrors the firmware:
             * numerics clamp at their ends, enums cycle through their option list.
             *
             * A TAP is tier one — "a short press is always exactly ±1 step"
             * (interaction_handler.cpp:148). The held ramp calls the same function with the tier's
             * multiplier.
             */
            stepEditorValue(
              selectedScreen,
              action === "config.action.value.increment" ? 1 : -1,
              1
            );
          } else if (action === "config.action.value.commit" ||
                     action === "config.action.value.discard") {
            /**
             * COMMIT writes the pending value into memory; DISCARD throws it away.
             *
             * Both then ascend, which is what the firmware does — and which is why this used to
             * "work" while doing nothing: the navigation half was implemented and the value half
             * was not, so an editor always looked like it had saved.
             */
            const setting = settingOfScreen(selectedScreen, manifestValueById);
            const pending = editorStateRef.current;
            if (action === "config.action.value.commit" && setting && pending?.bindingId === setting.id) {
              commitEditorValue(setting, pending.raw);
            }
            editorStateRef.current = null;
            setEditorState(null);
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
          } else if (action === "core.action.reset-session" || action === "core.action.reset-all-measured") {
            /**
             * A reset performed from level 0 RETURNS TO WHERE IT WAS STARTED — and actually resets.
             *
             * Two defects, one flow. The values never reset in the simulation at all: neither action
             * had a handler, so the confirm screen and its acknowledgement toast played out over
             * completely unchanged numbers. And the navigation left you nowhere useful — the toast
             * dismisses with `nav.back`, which pops ONE level and lands back on the confirm screen,
             * so the operator had to page out and back in to see whether anything had happened.
             *
             * The rule is the owner's: a reset invoked from a level-0 page stays at that page. It
             * cannot be a fixed target, because `confirm-reset-session` is reached from both P3 and
             * P4 and a fixed one would send a P4 operator to P3. So the stack is unwound to depth 0,
             * whose bottom frame IS the page the descent started from.
             */
            setSensors((table) =>
              action === "core.action.reset-session" ? resetSession(table) : resetMeasured(table)
            );
            const origin = navParentsRef.current[0];
            clearNavParents();
            const resolvedId = selectById(origin ?? kRootScreenId);
            if (resolvedId) {
              activeScreenId = resolvedId;
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
            /**
             * Descending into an EDITOR starts its pending value at the value in force.
             *
             * `UiEditorState::pending = saved` on open (`beginEdit`), so `New` and `Saved` agree until
             * the operator moves something. The panel used to show the static sample here — a
             * deliberately DIFFERENT value — so an editor looked like it had already been changed
             * before a single button was pressed, and the first UP appeared to do nothing because it
             * landed on the number already displayed.
             */
            {
              const target = screens.find((candidate) => candidate.id === flow.targetScreenId);
              const targetSetting = settingOfScreen(target, manifestValueById);
              const opensEditor = target?.elements.some(
                (element) => element.binding === "config.editor.pending"
              );
              if (opensEditor && targetSetting) {
                editorStateRef.current = {
                  bindingId: targetSetting.id,
                  raw: currentRawFor(targetSetting)
                };
                setEditorState(editorStateRef.current);
              }
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
        /**
         * A press the dataset does not answer falls back to browsing the screen LIST.
         *
         * That is a design-tool affordance, not device behaviour: `selectByOffset` walks the flat list in
         * tree order, so it steps onto editor and confirm screens that the current screen has no route to,
         * and it does not push a navigation parent, so the depth it leaves behind is a fiction.
         *
         * It must therefore answer a deliberate TAP and nothing else. It used to answer `kind !== "long"`,
         * which included REPEAT — so holding UP or DOWN anywhere the dataset had no hold flow (which is
         * everywhere: the dataset declares zero) walked the operator through the catalogue at 250 ms a
         * step. From M1 that is M1.V, its own editor, entered without descending. This was the second and
         * larger half of the reported hold bug, and the harder half to see, because it records no trace
         * entry naming a flow and the screen it lands on looks plausible.
         */
        let fallbackDestination: string | undefined;
        if (event.button === "up" && event.kind === "short") {
          fallbackDestination = selectByOffset(-1);
        } else if (event.button === "down" && event.kind === "short") {
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

  /**
   * AUTO-DISMISS: a `timeout` flow with no `holdButton` fires unattended after its duration.
   *
   * The other half of the trigger `types.ts` documents, and it was as unimplemented as hold-to-confirm
   * was — so the three acknowledgement toasts, whose only flow is a 2 s auto-dismiss, could be reached
   * and then never left. Generic rather than special-cased per toast: the dataset says which screens
   * have one and how long, and anything added later works with no change here.
   *
   * `nav.back` from a toast lands on the originating page because the toast REPLACED the confirm at
   * the same depth rather than being pushed — see fireHoldFlow.
   */
  useEffect(() => {
    const auto = selectedScreen?.flows?.find(
      (flow) =>
        flow.trigger.type === "timeout" &&
        !(flow.trigger as { holdButton?: string }).holdButton
    );
    if (!auto) {
      return;
    }
    const delay = (auto.trigger as { durationMs?: number }).durationMs ?? 2000;
    const handle = window.setTimeout(() => {
      recordTraceEntry({
        id: auto.actionId ?? "timeout",
        label: auto.label,
        functionName: auto.actionId ? actionCatalog.get(auto.actionId)?.label : undefined,
        trigger: `timeout ${delay}ms`,
        screenId: selectedScreen?.id ?? "unknown",
        screenName: selectedScreen?.name,
        actionParams: auto.actionParams ?? null,
        targetScreenId: auto.targetScreenId
      });
      if (auto.actionId === "ui.action.nav.back") {
        const parent = popNavParent();
        const resolved = selectById(auto.targetScreenId ?? parent ?? kRootScreenId);
        if (resolved) {
          setSelectedScreenId(resolved);
        }
        return;
      }
      if (auto.targetScreenId) {
        const resolved = selectById(auto.targetScreenId);
        if (resolved) {
          setSelectedScreenId(resolved);
        }
      }
    }, delay);
    return () => window.clearTimeout(handle);
  }, [actionCatalog, popNavParent, recordTraceEntry, selectById, selectedScreen]);

  /**
   * The screen's hold-to-confirm flow, if it has one.
   *
   * A `timeout` trigger with `holdButton` is a HOLD; one without is an auto-dismiss and must not be
   * started by a button press (that is what the toasts use).
   */
  const holdFlowFor = useCallback(
    (screen: ScreenDefinition | undefined) =>
      screen?.flows?.find(
        (flow) => flow.trigger.type === "timeout" && (flow.trigger as { holdButton?: string }).holdButton === "enter"
      ),
    []
  );

  const clearHoldTimer = useCallback(() => {
    if (holdTimerRef.current !== undefined) {
      window.clearInterval(holdTimerRef.current);
      holdTimerRef.current = undefined;
    }
  }, []);

  /** Fires the flow's action and target the way a button flow would, once the hold completes. */
  const fireHoldFlow = useCallback(
    (screen: ScreenDefinition, flow: ScreenFlow) => {
      recordTraceEntry({
        id: flow.actionId ?? "timeout",
        label: flow.label,
        functionName: flow.actionId ? actionCatalog.get(flow.actionId)?.label : undefined,
        trigger: `enter.hold ${(flow.trigger as { durationMs?: number }).durationMs ?? 0}ms`,
        screenId: screen.id,
        screenName: screen.name,
        actionParams: flow.actionParams ?? null,
        targetScreenId: flow.targetScreenId
      });
      if (flow.actionId === "core.action.reset-session" || flow.actionId === "core.action.reset-all-measured") {
        // Same two helpers as the keyboard path above — the three fields a measured reset clears had
        // been written out by hand in both places, which is two chances to forget the third.
        setSensors((table) =>
          flow.actionId === "core.action.reset-session" ? resetSession(table) : resetMeasured(table)
        );
      }
      if (flow.actionId === "core.action.factory-reset") {
        setSensors(createSensorTable());
        setPinnedValues({});
      }
      /**
       * The acknowledgement TOAST is shown, then it dismisses itself.
       *
       * This used to jump straight to the originating page, so the toast never appeared in the
       * simulator at all — the reset happened and the operator got no confirmation that it had. The
       * firmware shows it, which is the whole reason the screen exists.
       *
       * The toast REPLACES the confirm at the same depth rather than being pushed. That is what makes
       * its own `nav.back` land on the originating page instead of back on the confirm — being asked
       * again whether to do the thing you just did. So the stack is left untouched here.
       */
      if (flow.targetScreenId) {
        const resolved = selectById(flow.targetScreenId);
        if (resolved) {
          setSelectedScreenId(resolved);
        }
        return;
      }
      // No toast — the factory reset has none, because the device reboots. The owner's rule then
      // applies directly: a reset started from a level-0 page returns to that page rather than
      // dropping the operator at the root to re-navigate.
      const origin = navParentsRef.current[0];
      clearNavParents();
      const resolved = selectById(origin ?? kRootScreenId);
      if (resolved) {
        setSelectedScreenId(resolved);
      }
    },
    [actionCatalog, clearNavParents, recordTraceEntry, selectById]
  );

  /**
   * Jumping straight to a screen ADOPTS ITS ANCESTRY, instead of leaving the stack as it was.
   *
   * Picking from the list only set `selectedScreenId`, so the nav stack kept whatever the last real
   * navigation had left on it. The depth was then a fiction — `nav.position` reported L2 on a page
   * reached by one click — and anything reading the stack's bottom frame got the wrong answer: a
   * reset performed after jumping around returned the operator to a page from a previous descent
   * rather than the one they were on. Now the stack says what the tree says.
   */
  const jumpToScreen = useCallback(
    (id: string) => {
      const ancestors: string[] = [];
      let cursor = hierarchy.parentMap.get(id) ?? null;
      const guard = new Set<string>();
      while (cursor && !guard.has(cursor)) {
        guard.add(cursor);
        ancestors.unshift(cursor);
        cursor = hierarchy.parentMap.get(cursor) ?? null;
      }
      setNavParents(ancestors);
      navParentsRef.current = ancestors;
      setSelectedScreenId(id);
    },
    [hierarchy]
  );

  /**
   * §5.4's held ramp: the editor's claim on UP/DOWN, handed to the input adapter.
   *
   * This is the half that was missing, and the missing half was the whole reported bug. Holding UP in an
   * editor did nothing for 1500 ms, and then the queue's ring repeats arrived — `f-inc` has no
   * targetScreenId so `findMatchingButtonFlows` correctly dropped it, leaving the value frozen; on a
   * SETTING screen the same repeats hit `f-prev`/`f-next`, which do have targets, so the ring paged and
   * the operator landed on the next setting. Both symptoms, one cause: nothing here spoke for the editor.
   *
   * `owns` is derived from the dataset rather than from a second flag: a screen owns UP/DOWN precisely
   * when it declares an increment flow AND resolves to a setting, which is the dataset's spelling of
   * `editor.active && editor.setting`. It also stands down while the pack selector or the blanked display
   * owns the buttons — the firmware's own `handleEditorRepeat` does not test those, but it cannot step a
   * value nobody can see, and mirroring that would be mirroring a bug.
   */
  const editorRepeatBridge = useMemo(
    () => ({
      owns: () => {
        if (selectorOpenRef.current || !displayOnRef.current) {
          return false;
        }
        const screen = selectedScreenRef.current;
        const declaresAdjust = screen?.flows?.some(
          (flow) => flow.actionId === "config.action.value.increment"
        );
        if (!declaresAdjust) {
          return false;
        }
        return Boolean(settingOfScreen(screen, manifestValueById));
      },
      step: (button: "up" | "down", multiplier: number, heldMs: number) => {
        const screen = selectedScreenRef.current;
        const direction = button === "up" ? 1 : -1;
        if (!stepEditorValueRef.current(screen, direction, multiplier)) {
          return;
        }
        recordTraceEntry({
          id: direction > 0 ? "config.action.value.increment" : "config.action.value.decrement",
          trigger: `${button === "up" ? "BtnA" : "BtnB"} held ${(heldMs / 1000).toFixed(1)}s`,
          screenId: screen?.id ?? "unknown",
          screenName: screen?.name,
          notes: `§5.4 ramp: x${multiplier} step (handleEditorRepeat owns the button; the release is swallowed)`
        });
      }
    }),
    [manifestValueById, recordTraceEntry]
  );

  const { pressed, armedCombo, press, release, cancelAll } = useSimulatedButtons(
    handleButtonEvent,
    editorRepeatBridge
  );

  const handleButtonPressStart = useCallback(
    (button: "up" | "down" | "enter") => {
      press(button);
      if (button !== "enter") return;
      const flow = holdFlowFor(selectedScreen);
      if (!flow || !selectedScreen) return;
      const totalMs = (flow.trigger as { durationMs?: number }).durationMs ?? 3000;
      clearHoldTimer();
      setHoldCountdown({ screenId: selectedScreen.id, remainingMs: totalMs, totalMs });
      const startedAt = performance.now();
      holdTimerRef.current = window.setInterval(() => {
        const remaining = totalMs - (performance.now() - startedAt);
        if (remaining <= 0) {
          clearHoldTimer();
          setHoldCountdown(null);
          fireHoldFlow(selectedScreen, flow);
          return;
        }
        setHoldCountdown({ screenId: selectedScreen.id, remainingMs: remaining, totalMs });
      }, 100);
    },
    [clearHoldTimer, fireHoldFlow, holdFlowFor, press, selectedScreen]
  );

  const handleButtonPressEnd = useCallback(
    (button: "up" | "down" | "enter") => {
      if (button === "enter") {
        // Released early: the hold is abandoned, which is the whole point of a hold gesture.
        clearHoldTimer();
        setHoldCountdown(null);
      }
      release(button);
    },
    [clearHoldTimer, release]
  );


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
            onSelect={jumpToScreen}
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
                      boundValues={resolvedValues}
                      powered={displayOn}
                      // The firmware-drawn page. Passing it is what stops the chrome claiming the Select
                      // Menu is open while the panel shows the screen underneath it.
                      packSelector={
                        selectorOpen
                          ? { entries: packEntries, cursor: packCursor, truncated: false }
                          : null
                      }
                      // The dots chase at the rate the aggregate flow implies, and only while the
                      // loop is advancing — on the device they are driven by millis(), so a stopped
                      // loop is a stopped panel.
                      aggregateFlowLpm={aggregateFlowLpm}
                      animating={loopRunning && displayOn}
                      repaintCount={repaintCount}
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
                  onPressStart={handleButtonPressStart}
                  onPressEnd={handleButtonPressEnd}
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
                  flowMode={flowMode}
                  onFlowModeChange={setFlowMode}
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
                  onSensorFlowChange={handleSensorFlowChange}
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
                    boundValues={resolvedValues}
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
