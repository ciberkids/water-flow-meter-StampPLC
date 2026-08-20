import { useMemo, useState } from "react";
import { ScreenDataset, ScreenDefinition } from "../types";

interface HelpPanelProps {
  dataset: ScreenDataset;
  selectedScreen?: ScreenDefinition;
  onNavigateToTab?: (tab: "simulation" | "design" | "importExport" | "help") => void;
}

// Derived from schemaDefinitions.ts element kind enum
const ELEMENT_KINDS = ["text", "value", "badge", "box", "icon", "scrollbar"] as const;

const ELEMENT_KIND_DOCS: Record<string, { description: string; keyProps: string; example: string }> = {
  text: {
    description: "Static label — renders a fixed string from the dataset.",
    keyProps: "content, align, emphasis",
    example: `{ "id": "title", "kind": "text", "x": 10, "y": 4, "content": "Flow meter", "emphasis": "strong" }`
  },
  value: {
    description: "Live value — content is replaced at runtime by a firmware data source.",
    keyProps: "content (default) or binding",
    example: `{ "id": "flow-rate", "kind": "value", "x": 10, "y": 20, "binding": "sensor.flow.instant" }`
  },
  badge: {
    description: "Status badge — coloured pill useful for diagnostics or mode indicators.",
    keyProps: "content, binding, emphasis",
    example: `{ "id": "diag", "kind": "badge", "x": 0, "y": 60, "binding": "telemetry.status" }`
  },
  box: {
    description: "Filled rectangle — used as a divider, background, or separator.",
    keyProps: "width, height (both required)",
    example: `{ "id": "separator", "kind": "box", "x": 0, "y": 30, "width": 135, "height": 1 }`
  },
  icon: {
    description: "Vector icon — references a graphic asset by its assetId.",
    keyProps: "metadata.assetId",
    example: `{ "id": "wifi-icon", "kind": "icon", "x": 120, "y": 4, "metadata": { "assetId": "wifi" } }`
  },
  animation: {
    description: "Animated asset — plays SVG frames from a graphic asset on the screen.",
    keyProps: "metadata.assetId, and a matching animation definition",
    example: `{ "id": "level-pos", "kind": "scrollbar", "x": 232, "y": 20, "width": 6, "height": 100 }`
  },
  scrollbar: {
    description: "Scroll position indicator — auto-indexed by position in the element list.",
    keyProps: "metadata.autoScrollIndex (true to auto-number)",
    example: `{ "id": "scroll-pos", "kind": "scrollbar", "x": 130, "y": 0, "height": 240, "metadata": { "autoScrollIndex": true } }`
  }
};

const VALIDATION_ERRORS = [
  {
    id: "led-legend",
    message: "LED legend is present",
    fix: "Add a text element on any screen with binding set to legend.led."
  },
  {
    id: "countdown-overlay",
    message: "Countdown overlay exists",
    fix: "Add a screen whose id contains 'countdown', with a value element bound to countdown.value."
  },
  {
    id: "diagnostics-banner",
    message: "Diagnostics banner is available",
    fix: "Add a badge element bound to telemetry.status or diagnostics.summary on any screen."
  },
  {
    id: "manifest-binding-coverage",
    message: "Firmware manifest binding coverage — unknown action",
    fix: "Import a firmware manifest that lists all custom actionId values used in flows and events. Built-in actions (ui.* and core.*) are always allowed."
  }
];

function AccordionSection({
  id,
  title,
  children,
  defaultOpen = false
}: {
  id: string;
  title: string;
  children: React.ReactNode;
  defaultOpen?: boolean;
}) {
  const [open, setOpen] = useState(defaultOpen);
  return (
    <article className="help-panel__card help-panel__accordion">
      <button
        type="button"
        className="help-panel__accordion-toggle"
        aria-expanded={open}
        aria-controls={`help-section-${id}`}
        onClick={() => setOpen((prev) => !prev)}
        id={`help-heading-${id}`}
      >
        <h4>{title}</h4>
        <span className="accordion-chevron" aria-hidden="true">{open ? "▲" : "▼"}</span>
      </button>
      <div
        id={`help-section-${id}`}
        role="region"
        aria-labelledby={`help-heading-${id}`}
        hidden={!open}
        className="help-panel__accordion-body"
      >
        {children}
      </div>
    </article>
  );
}

export function HelpPanel({ dataset, selectedScreen, onNavigateToTab }: HelpPanelProps) {
  const liveScreenJson = useMemo(() => {
    if (!selectedScreen) {
      return "// Select a screen in the sidebar to view its JSON definition.";
    }
    return JSON.stringify(selectedScreen, null, 2);
  }, [selectedScreen]);

  const datasetSummary = useMemo(
    () => ({
      screenCount: dataset.screens.length,
      ids: dataset.screens.map((screen) => screen.id)
    }),
    [dataset]
  );

  return (
    <section className="help-panel">
      <header>
        <h3>Help &amp; Documentation</h3>
        <p>
          Reference for every JSON field, element type, firmware manifest workflow, and common validation errors.
          Use the sections below as your single source of truth when authoring datasets.
        </p>
      </header>

      <article className="help-panel__card">
        <h4>Dataset summary</h4>
        <dl>
          <div>
            <dt>Total screens</dt>
            <dd>{datasetSummary.screenCount}</dd>
          </div>
          <div>
            <dt>Screen IDs</dt>
            <dd>{datasetSummary.ids.join(", ") || "—"}</dd>
          </div>
        </dl>
      </article>

      <AccordionSection id="schema-overview" title="Schema overview" defaultOpen>
        <p>
          A dataset is a JSON object with two top-level keys: <code>screens</code> (array) and <code>theme</code>.
          Each screen has a unique <code>id</code>, a <code>name</code>, an <code>elements</code> array, and optional
          <code>flows</code>, <code>events</code>, <code>assets</code>, and <code>submenus</code>.
        </p>
        <p>
          <strong>Element kinds:</strong>{" "}
          {ELEMENT_KINDS.map((k) => <code key={k}>{k}</code>).reduce<React.ReactNode[]>(
            (acc, el, i) => (i === 0 ? [el] : [...acc, ", ", el]),
            []
          )}
        </p>
        <p>
          <strong>Flow trigger types:</strong> <code>button</code> (up/down/enter, short/long),{" "}
          <code>timeout</code> (durationMs), <code>data</code> (source + condition).
        </p>
        <p>
          Validate against the canonical AJV schema in <code>shared/schemaDefinitions.ts</code>.
          Use the <em>Validate JSON</em> button in Import &amp; Export to check your dataset.
        </p>
      </AccordionSection>

      <AccordionSection id="element-reference" title="Element reference">
        <p>Every element requires <code>id</code>, <code>kind</code>, <code>x</code>, and <code>y</code>. All other fields are optional.</p>
        <table className="help-panel__table" aria-label="Element kind reference">
          <thead>
            <tr>
              <th>Kind</th>
              <th>Description</th>
              <th>Key props</th>
            </tr>
          </thead>
          <tbody>
            {ELEMENT_KINDS.map((kind) => {
              const doc = ELEMENT_KIND_DOCS[kind];
              return (
                <tr key={kind}>
                  <td><code>{kind}</code></td>
                  <td>{doc?.description}</td>
                  <td><code>{doc?.keyProps}</code></td>
                </tr>
              );
            })}
          </tbody>
        </table>
        <details className="help-panel__examples">
          <summary>JSON examples for each kind</summary>
          {ELEMENT_KINDS.map((kind) => (
            <div key={kind}>
              <strong>{kind}</strong>
              <pre>{ELEMENT_KIND_DOCS[kind]?.example}</pre>
            </div>
          ))}
        </details>
      </AccordionSection>

      <AccordionSection id="scrollbar" title="Scroll bar numbering scheme">
        <p>
          Scroll bar elements (<code>kind: "scrollbar"</code>) display a position indicator for long lists.
          Set <code>metadata.autoScrollIndex</code> to <code>true</code> to let the firmware assign the scroll
          position automatically based on the element&apos;s order within the screen.
        </p>
        <p>
          For manual control, omit <code>autoScrollIndex</code> and use the element&apos;s <code>binding</code> field
          to reference a firmware value that returns the current scroll position (0&ndash;100).
        </p>
        <p>
          <strong>Hierarchy numbering:</strong> scrollbar indices follow the <em>depth-first</em> order of the screen
          hierarchy. Root screens are numbered starting from 0; child screens inherit then extend the parent&apos;s numbering.
          The sidebar hierarchy panel in the Design tab shows the current tree so you can verify the order.
        </p>
      </AccordionSection>

      <AccordionSection id="manifest" title="Firmware manifest usage">
        <p>
          The firmware manifest is a JSON file (<code>ui_manifest.json</code>) generated from the firmware source.
          It lists every callable action with an <code>id</code>, a human-readable <code>label</code>, and optional parameter
          descriptors. Importing it unlocks two features:
        </p>
        <ol>
          <li>
            <strong>Simulation trace:</strong> function traces show friendly names and parameter schemas instead of raw IDs.
          </li>
          <li>
            <strong>Export coverage check:</strong> the exporter verifies every <code>flow.actionId</code> and
            <code>event.actionId</code> (except built-in <code>ui.*</code> / <code>core.*</code> actions) exists in the manifest.
            Unknown actions block the export.
          </li>
        </ol>
        <h5>Recommended workflow</h5>
        <ol>
          <li>Build the firmware → the PlatformIO build step emits <code>ui_manifest.json</code>.</li>
          <li>Open the <strong>Import &amp; Export</strong> tab → <em>Import manifest</em>.</li>
          <li>Switch to <strong>Simulation</strong> &mdash; the Function Trace panel now shows action labels.</li>
          <li>Switch back to <strong>Import &amp; Export</strong> → <em>Export to Firmware</em> → the manifest status card
            confirms coverage.</li>
        </ol>
        {onNavigateToTab && (
          <button
            type="button"
            className="text-link-button"
            onClick={() => onNavigateToTab("simulation")}
            aria-label="Open the Simulation tab to view the function trace panel"
          >
            → Open Simulation trace panel
          </button>
        )}
      </AccordionSection>

      <AccordionSection id="troubleshooting" title="Troubleshooting validation errors">
        <p>
          The exporter runs several checks before writing output. Each failing check blocks the export and appears
          in the <em>Validation checks</em> section of the Export panel. Common failures:
        </p>
        <dl className="help-panel__trouble-list">
          {VALIDATION_ERRORS.map((err) => (
            <div key={err.id}>
              <dt><strong>{err.message}</strong></dt>
              <dd>{err.fix}</dd>
            </div>
          ))}
        </dl>
        <p>
          For schema validation errors (wrong field type, missing required key), use the <em>Validate JSON</em> button
          in the Import &amp; Export tab to get an AJV error report, or compare against the schema reference below.
        </p>
      </AccordionSection>

      <AccordionSection id="simulation-workflow" title="Simulation workflow">
        <p>Use the Simulation tab to emulate firmware behaviour before exporting. Recommended steps:</p>
        <ol>
          <li>Import the firmware action manifest (Import &amp; Export → Import manifest) so function traces display friendly names.</li>
          <li>Drive the UI with the StampPLC buttons (or keyboard arrows/ENTER) to move between info pages and configuration.</li>
          <li>Modify value placeholders inline; overrides are highlighted until saved and each edit/save is logged.</li>
          <li>Review the Function Trace panel to confirm the dispatcher triggers the expected firmware action IDs.</li>
        </ol>
        {onNavigateToTab && (
          <button
            type="button"
            className="text-link-button"
            onClick={() => onNavigateToTab("simulation")}
            aria-label="Open the Simulation tab"
          >
            → Open Simulation tab
          </button>
        )}
      </AccordionSection>

      <AccordionSection id="live-json" title="Live screen JSON">
        <p>
          Rendered from <code>{selectedScreen?.id ?? "—"}</code>. Changes to the data source update this view automatically.
        </p>
        <pre>{liveScreenJson}</pre>
      </AccordionSection>
    </section>
  );
}
