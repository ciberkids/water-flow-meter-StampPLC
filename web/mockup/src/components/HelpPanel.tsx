import { useMemo } from "react";
import { ScreenDataset, ScreenDefinition } from "../types";

interface HelpPanelProps {
  dataset: ScreenDataset;
  selectedScreen?: ScreenDefinition;
}

const structureReference = `{
  "screens": [
    {
      "id": "unique-id",
      "name": "UI label",
      "description": "Optional helper text",
      "elements": [
        {
          "id": "element-id",
          "kind": "text | value | badge | box | icon",
          "x": 0,
          "y": 0,
          "width": 50,
          "height": 12,
          "content": "Rendered string",
          "align": "left | center | right",
          "emphasis": "normal | strong | muted",
          "binding": "optional.binding.id"
        }
      ],
      "submenus": [
        {
          "id": "submenu-id",
          "label": "Maintenance",
          "screenId": "target-screen-id",
          "iconAssetId": "optional-asset"
        }
      ],
      "flows": [
        {
          "id": "flow-id",
          "label": "Go to maintenance",
          "targetScreenId": "target-screen-id",
          "trigger": {
            "type": "button",
            "button": "up | down | enter",
            "gesture": "short | long | hold"
          },
          "actionId": "ui.navigate.page.next",
          "actionParams": {
            "durationMs": 3000
          },
          "guard": "optional boolean expression"
        }
      ],
      "assets": [
        {
          "id": "countdown-ring",
          "type": "svg-sequence | bitmap | icon",
          "source": "relative/path/to/asset.svg",
          "frames": ["frame0.svg", "frame1.svg"],
          "fps": 15,
          "palette": ["#000000", "#00FF00"]
        }
      ],
      "animations": [
        {
          "id": "animation-id",
          "targetElementId": "element-id",
          "kind": "frame-sequence | property",
          "easing": "linear | ease-in | ease-out | ease-in-out",
          "loop": false,
          "frames": [
            { "at": 0, "state": { "content": "3" }, "assetFrameIndex": 0 },
            { "at": 1, "state": { "content": "2" }, "assetFrameIndex": 1 },
            { "at": 2, "state": { "content": "1" }, "assetFrameIndex": 2 }
          ]
        }
      ]
    }
  ],
  "theme": {
    "name": "Preset label",
    "colors": {
      "displayBackground": "#000a17",
      "textPrimary": "#f5faff",
      "textMuted": "#9caec6",
      "textStrong": "#ffffff",
      "value": "#56d2ff",
      "badgeBackground": "#0f1e33",
      "badgeBorder": "#80a8c9",
      "icon": "#56d2ff",
      "legend": "#85bbe8",
      "gridMinor": "rgba(124, 162, 206, 0.28)",
      "gridMajor": "rgba(124, 162, 206, 0.55)"
    },
    "typography": {
      "base": 8,
      "value": 10,
      "badge": 8
    },
    "animation": {
      "easing": "ease-in-out"
    }
  }
}`;

export function HelpPanel({ dataset, selectedScreen }: HelpPanelProps) {
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
        <h3>Help & explanation</h3>
        <p>
          Reference the expected JSON structure and inspect the live data for the currently selected screen.
          Use this view when preparing exports or cross-checking translator output.
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
            <dd>{datasetSummary.ids.join(", ")}</dd>
          </div>
        </dl>
      </article>

      <article className="help-panel__card">
        <h4>Simulation workflow</h4>
        <p>
          Use the Simulation tab to emulate firmware behaviour before exporting. Recommended steps:
        </p>
        <ol>
          <li>Import the firmware action manifest so function traces display friendly names and parameter schemas.</li>
          <li>Drive the UI with the StampPLC buttons (or keyboard arrows/ENTER) to move between info pages and configuration.</li>
          <li>Modify value placeholders inline; overrides are highlighted until saved and each edit/save is logged.</li>
          <li>Review the Function Trace panel to confirm the dispatcher triggers the expected firmware action IDs.</li>
        </ol>
      </article>

      <article className="help-panel__card">
        <h4>Schema reference</h4>
        <p>
          Each entry in <code>screens</code> defines layout elements, navigation flows, assets, and animations.
          Use the dataset tools in the sidebar to import new JSON and validate it against this structure before exporting.
          The design tokens that drive colours and typography live under the dataset&apos;s top-level <code>theme</code> key and stay aligned with the Design tab preview.
        </p>
        <pre>{structureReference}</pre>
      </article>

      <article className="help-panel__card">
        <h4>Live screen JSON</h4>
        <p>
          Rendered from <code>{selectedScreen?.id ?? "—"}</code>. Changes to the data source update this view automatically.
        </p>
        <pre>{liveScreenJson}</pre>
      </article>
    </section>
  );
}
