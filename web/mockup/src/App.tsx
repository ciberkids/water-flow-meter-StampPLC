import { ChangeEvent, useCallback, useEffect, useMemo, useRef, useState } from "react";
import "./App.css";
import { DisplayViewport } from "./components/DisplayViewport";
import { ScreenSelector } from "./components/ScreenSelector";
import screensData from "./data/screens.json";
import { DisplayOrientation, ScreenDataset, ScreenDefinition, ScreenElement } from "./types";
import { ButtonPanel } from "./components/ButtonPanel";
import { ThemeEditor } from "./components/ThemeEditor";
import { HelpPanel } from "./components/HelpPanel";
import { useTheme } from "./theme/ThemeProvider";
import { cloneTheme, type ThemeTokens } from "./theme/types";
import { defaultTheme } from "./theme/defaultTheme";
import { useSimulatedButtons } from "./hooks/useSimulatedButtons";
import { SimulatedButton, SimulatedButtonEvent } from "./types/buttonSimulation";
import { computeLayout } from "./utils/layout";
import { SchemaValidationError, validateDataset } from "./schema/validation";
import { ExporterPanel } from "./components/ExporterPanel";
import { SimulationTracePanel } from "./components/SimulationTracePanel";
import { ValuePlaceholderPanel } from "./components/ValuePlaceholderPanel";
import { SimulationTraceEntry } from "./types/simulationTrace";
import { findMatchingButtonFlows } from "./utils/flowMatching";

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

type ValidationFeedback = {
  status: "idle" | "success" | "error";
  message: string;
  issues: string[];
};

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

  const initialDataset = useMemo<ScreenDataset>(
    () => ensureDatasetTheme(datasetValidation.dataset),
    [datasetValidation.dataset]
  );

  const { theme, updateTheme } = useTheme();

  const initialScreens = initialDataset.screens;
  const [dataset, setDataset] = useState<ScreenDataset>(initialDataset);
  const [validationFeedback, setValidationFeedback] = useState<ValidationFeedback>(() => ({
    status: datasetValidation.errors?.length ? "error" : "success",
    message: datasetValidation.errors?.length
      ? "Dataset validation failed. Import or fix the JSON and validate again."
      : "Dataset loaded successfully.",
    issues: datasetValidation.errors ?? []
  }));
  const [selectedScreenId, setSelectedScreenId] = useState<string>(initialScreens[0]?.id ?? "");
  const screens: ScreenDefinition[] = useMemo(() => dataset.screens, [dataset]);
  const datasetSummary = useMemo(
    () => ({
      screenCount: dataset.screens.length,
      ids: dataset.screens.map((screen) => screen.id)
    }),
    [dataset]
  );
  const [zoom, setZoom] = useState<number>(200);
  const [showGrid, setShowGrid] = useState<boolean>(true);
  const [orientation, setOrientation] = useState<DisplayOrientation>("landscape");
  const [interactionLog, setInteractionLog] = useState<string[]>([]);
  const [traceEntries, setTraceEntries] = useState<SimulationTraceEntry[]>([]);
  const [traceFilter, setTraceFilter] = useState<string>("");
  const [transitionPreview, setTransitionPreview] = useState<{ screenId: string; screenName?: string } | null>(null);
  const [valueOverrides, setValueOverrides] = useState<Record<string, Record<string, string>>>({});
  const [activePanel, setActivePanel] = useState<"simulation" | "design" | "importExport" | "help">("simulation");
  const fileInputRef = useRef<HTMLInputElement | null>(null);
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
          const normalized = ensureDatasetTheme(validated);
          setDataset(normalized);
          setSelectedScreenId(normalized.screens[0]?.id ?? "");
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
    [validateDatasetSafe]
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
    const normalized = ensureDatasetTheme(validated);
    setDataset(normalized);
    setValidationFeedback({
      status: "success",
      message: `Dataset validated (${validated.screens.length} screens).`,
      issues: []
    });
  }, [dataset, validateDatasetSafe]);

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
    const timer = window.setTimeout(() => setTransitionPreview(null), 2500);
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
      setTraceEntries((current) => {
        const next = [{ ...entry, timestamp: entry.timestamp ?? Date.now() }, ...current];
        return next.slice(0, 25);
      });
    },
    []
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
    [recordTraceEntry, selectedScreen?.name, valueOverrides]
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
        id: "core.action.save-config",
        label: "Save configuration",
        trigger: `value.save.${element.id}`,
        screenId,
        screenName: selectedScreen?.name,
        actionParams: { elementId: element.id, value }
      });
    },
    [recordTraceEntry, selectedScreen?.name]
  );

  const previewTransition = useCallback(
    (targetScreenId?: string) => {
      if (!targetScreenId) {
        return;
      }
      const targetScreen = dataset.screens.find((candidate) => candidate.id === targetScreenId);
      setTransitionPreview({ screenId: targetScreenId, screenName: targetScreen?.name });
    },
    [dataset.screens]
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
      const label = `${event.button.toUpperCase()} • ${event.kind}`;
      let destination: string | undefined = undefined;

      if (event.button === "up" && event.kind !== "long") {
        destination = selectByOffset(-1);
      } else if (event.button === "down" && event.kind !== "long") {
        destination = selectByOffset(1);
      } else if (event.button === "enter") {
        destination = event.kind === "long" ? selectById("countdown") : selectById("configuration");
      }

      const activeScreen = destination ?? selectedScreen?.id ?? screens[0]?.id ?? "—";
      appendLog(
        `[${new Date(event.timestamp).toLocaleTimeString()}] ${label} → ${activeScreen}`
      );

      const resolvedFlows = findMatchingButtonFlows(selectedScreen, event);
      if (resolvedFlows.length > 0) {
        resolvedFlows.forEach((flow) => {
          recordTraceEntry({
            id: flow.actionId ?? "ui.action.unassigned",
            label: flow.label,
            trigger: `${event.button}.${event.kind}`,
            screenId: selectedScreen?.id ?? "unknown",
            screenName: selectedScreen?.name,
            actionParams: flow.actionParams ?? null,
            targetScreenId: flow.targetScreenId
          });
          previewTransition(flow.targetScreenId);
        });
      } else {
        recordTraceEntry({
          id: "ui.action.unmapped",
          label: "No matching flow",
          trigger: `${event.button}.${event.kind}`,
          screenId: selectedScreen?.id ?? "unknown",
          screenName: selectedScreen?.name
        });
      }
    },
    [appendLog, previewTransition, recordTraceEntry, selectById, selectByOffset, selectedScreen, screens]
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

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
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
  }, [cancelAll, mapKeyToButton, press, release]);

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

  const renderScreenContext = (options?: { showThemeSnapshot?: boolean }) => (
    <div className="screen-context">
      <section>
        <h2>Screens</h2>
        <ScreenSelector
          screens={screens}
          activeId={selectedScreen?.id ?? ""}
          onSelect={setSelectedScreenId}
        />
      </section>
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
                    />
                  ) : (
                    <p>No screen selected.</p>
                  )}
                  {transitionPreview && (
                    <div className="transition-preview-banner">
                      <strong>Transition preview:</strong> {transitionPreview.screenName ?? transitionPreview.screenId}
                    </div>
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
                  {overflow.length === 0 ? (
            <span className="ok">All elements fit within {layoutReport?.bounds.width} × {layoutReport?.bounds.height}px.</span>
                  ) : (
                    <ul>
                      {overflow.map((item) => (
                        <li key={item.element.id}>
                          <code>{item.element.id}</code> spills beyond the viewport (x: {item.left}, y: {item.top}, w: {item.width}, h: {item.height})
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
              <div className="panel-column panel-column--context">{renderScreenContext({ showThemeSnapshot: true })}</div>
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
