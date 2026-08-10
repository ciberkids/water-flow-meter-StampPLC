import { ScreenDefinition } from "../types";
import { buildScreenHierarchy, flattenTree, type ScreenTreeNode } from "../utils/screenHierarchy";
import { useMemo } from "react";

interface ScreenSelectorProps {
  screens: ScreenDefinition[];
  activeId: string;
  previewId?: string;
  onSelect: (id: string) => void;
}

/**
 * The screen list, as the TREE the device navigates rather than one flat column.
 *
 * It was a flat `screens.map`, so 63 screens arrived in dataset order with nothing to say that
 * `config-s2-calibration` lives three levels inside `config-c7-sensor-select` — the structure the
 * operator walks was invisible in the tool built to walk it.
 *
 * The tree itself was a second, quieter bug: `buildScreenHierarchy` derived parentage from
 * `screen.submenus`, a field NOTHING in the dataset populates. All 63 screens had an empty
 * `submenus` while 29 carried a `ui.action.nav.descend` flow, so every screen came back a root and
 * the hierarchy was flat at the source. It now reads the descend flows, and adopts a level's whole
 * ring — otherwise Config claimed only C1, the one screen it descends into, and C2..C7 appeared as
 * top-level pages when they are six presses deep.
 */
export function ScreenSelector({ screens, activeId, previewId, onSelect }: ScreenSelectorProps) {
  const nodes = useMemo(() => flattenTree(buildScreenHierarchy(screens).roots), [screens]);

  return (
    <div className="screen-selector">
      {nodes.map((node: ScreenTreeNode) => (
        <button
          key={node.screen.id}
          type="button"
          className={[
            "screen-selector__item",
            node.screen.id === activeId ? "active" : "",
            previewId && previewId === node.screen.id ? "preview-target" : ""
          ]
            .filter(Boolean)
            .join(" ")}
          // Indent by depth, and keep a hairline rule at each level so a deep child reads as
          // belonging to something rather than as a stray short row.
          style={{
            paddingLeft: `${0.55 + node.depth * 0.85}rem`,
            borderLeft: node.depth > 0 ? "2px solid rgba(255,255,255,0.10)" : "2px solid transparent"
          }}
          onClick={() => onSelect(node.screen.id)}
          title={`${node.path} · depth ${node.depth} · ${node.screen.id}`}
        >
          <span className="screen-selector__row">
            <code className="screen-selector__path">{node.path}</code>
            <strong>{node.screen.name}</strong>
          </span>
          <div className="screen-selector__desc">{node.screen.description}</div>
        </button>
      ))}
    </div>
  );
}
