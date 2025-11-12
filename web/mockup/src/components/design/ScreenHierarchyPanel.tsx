import type { ScreenTreeNode } from "../../utils/screenHierarchy";

interface ScreenHierarchyPanelProps {
  nodes: ScreenTreeNode[];
  selectedId: string;
  breadcrumbs: string[];
  pathLabel?: string;
  onSelect: (id: string) => void;
  onAddRoot: () => void;
  onAddChild: () => void;
  onDuplicate: () => void;
  onDelete: () => void;
  onReorder: (direction: "up" | "down") => void;
  canDelete: boolean;
}

export function ScreenHierarchyPanel({
  nodes,
  selectedId,
  breadcrumbs,
  pathLabel,
  onSelect,
  onAddRoot,
  onAddChild,
  onDuplicate,
  onDelete,
  onReorder,
  canDelete
}: ScreenHierarchyPanelProps) {
  const renderNode = (node: ScreenTreeNode) => {
    const isSelected = node.screen.id === selectedId;
    return (
      <li key={node.screen.id}>
        <button
          type="button"
          className={isSelected ? "tree-item active" : "tree-item"}
          onClick={() => onSelect(node.screen.id)}
        >
          <span>{node.screen.name}</span>
          <small>{node.path}</small>
        </button>
        {node.children.length > 0 ? <ul>{node.children.map(renderNode)}</ul> : null}
      </li>
    );
  };

  return (
    <section className="hierarchy-panel" data-testid="screen-hierarchy">
      <div className="hierarchy-panel__header">
        <h2>Screen hierarchy</h2>
        <div className="hierarchy-actions">
          <button type="button" className="tool-button" onClick={onAddRoot}>
            Add root
          </button>
          <button type="button" className="tool-button" onClick={onAddChild} disabled={!selectedId}>
            Add child
          </button>
        </div>
      </div>
      <div className="hierarchy-breadcrumbs">
        <strong>Breadcrumbs:</strong>{" "}
        {breadcrumbs.length > 0 ? breadcrumbs.join(" / ") : "Select a screen"}
      </div>
      <div className="hierarchy-breadcrumbs">
        <strong>Scroll code:</strong> {pathLabel ?? "—"}
      </div>
      <ul className="hierarchy-tree">{nodes.map(renderNode)}</ul>
      <div className="hierarchy-secondary-actions">
        <button type="button" className="tool-button tool-button--secondary" onClick={() => onReorder("up")}>
          Move up
        </button>
        <button type="button" className="tool-button tool-button--secondary" onClick={() => onReorder("down")}>
          Move down
        </button>
        <button
          type="button"
          className="tool-button tool-button--secondary"
          onClick={onDuplicate}
          disabled={!selectedId}
        >
          Duplicate
        </button>
        <button
          type="button"
          className="tool-button tool-button--danger"
          onClick={onDelete}
          disabled={!canDelete}
        >
          Delete
        </button>
      </div>
    </section>
  );
}
