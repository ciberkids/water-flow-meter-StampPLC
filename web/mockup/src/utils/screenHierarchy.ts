import type { ScreenDefinition, ScreenSubmenu } from "../types";

export interface ScreenTreeNode {
  screen: ScreenDefinition;
  path: string;
  depth: number;
  children: ScreenTreeNode[];
  submenuMeta?: ScreenSubmenu;
  parentId?: string | null;
}

export interface ScreenHierarchy {
  roots: ScreenTreeNode[];
  pathMap: Map<string, string>;
  parentMap: Map<string, string | null>;
  breadcrumbsMap: Map<string, string[]>;
}

export function buildScreenHierarchy(screens: ScreenDefinition[]): ScreenHierarchy {
  const screenMap = new Map(screens.map((screen) => [screen.id, screen]));
  const parentMap = new Map<string, string | null>();

  /**
   * Parentage comes from the DESCEND FLOWS, which is where this dataset actually keeps its tree.
   *
   * It read `screen.submenus` alone — a field NOTHING in the dataset populates. All 63 screens have
   * an empty `submenus`, while 29 carry a `ui.action.nav.descend` flow, so every screen came back a
   * root and the panel showed one flat list. The hierarchy was not mis-rendered; it was being built
   * from the wrong field.
   *
   * `submenus` is still honoured and still wins, because a hand-authored dataset may use it and it is
   * the more explicit statement of intent. Descend flows fill in for the generated one.
   *
   * First parent wins. `confirm-reset-session` is descended into from both P3 and P4, and a tree has
   * to pick one — so the earlier screen in dataset order owns it, and the other keeps its flow
   * without adopting the child twice.
   */
  screens.forEach((screen) => {
    screen.flows?.forEach((flow) => {
      if (flow.actionId !== "ui.action.nav.descend" || !flow.targetScreenId) return;
      if (flow.targetScreenId === screen.id) return;               // a self-descent is not a child
      if (parentMap.has(flow.targetScreenId)) return;              // first parent wins
      if (!screenMap.has(flow.targetScreenId)) return;             // a dangling target is not a child
      parentMap.set(flow.targetScreenId, screen.id);
    });
  });
  /**
   * A descent lands in a LEVEL, and every ring member of that level sits at the same depth under the
   * same parent.
   *
   * Without this, P5 adopted only `config-c1-modbus-id` — the one screen it descends into — and C2
   * through C7 came back as ROOTS, which says they are top-level pages when they are six presses
   * inside the config level. The ring is walked by following UP/DOWN flows from the descended child
   * until it closes, and only screens with no parent yet are adopted, so an explicit descent always
   * beats an inferred sibling.
   */
  screens.forEach((screen) => {
    screen.flows?.forEach((flow) => {
      if (flow.actionId !== "ui.action.nav.descend" || !flow.targetScreenId) return;
      if (parentMap.get(flow.targetScreenId) !== screen.id) return;
      let cursor: string | undefined = flow.targetScreenId;
      const walked = new Set<string>([flow.targetScreenId]);
      while (cursor) {
        const node = screenMap.get(cursor);
        const next: string | undefined = node?.flows?.find(
          (f) => f.trigger.type === "button" && f.trigger.button === "down" && f.targetScreenId
        )?.targetScreenId;
        if (!next || walked.has(next)) break;
        walked.add(next);
        if (!parentMap.has(next) && next !== screen.id) {
          parentMap.set(next, screen.id);
        }
        cursor = next;
      }
    });
  });

  // Explicit submenus override, so an authored tree is never second-guessed by an inferred one.
  screens.forEach((screen) => {
    screen.submenus?.forEach((submenu) => {
      parentMap.set(submenu.screenId, screen.id);
    });
  });

  const pathMap = new Map<string, string>();
  const breadcrumbsMap = new Map<string, string[]>();

  /** parent id -> child ids, in dataset order, inverted once from parentMap. */
  const childrenOf = new Map<string, string[]>();
  screens.forEach((screen) => {
    const parent = parentMap.get(screen.id);
    if (!parent) return;
    const list = childrenOf.get(parent) ?? [];
    list.push(screen.id);
    childrenOf.set(parent, list);
  });

  /** Screens on the current recursion path, so a cyclic descent cannot recurse forever. */
  const guard = new Set<string>();

  const visit = (
    screen: ScreenDefinition,
    depth: number,
    indexLabel: string,
    breadcrumbs: string[],
    parentId: string | null
  ): ScreenTreeNode => {
    pathMap.set(screen.id, indexLabel);
    const nodeBreadcrumbs = [...breadcrumbs, screen.name];
    breadcrumbsMap.set(screen.id, nodeBreadcrumbs);

    /**
     * Children come from `parentMap`, which is now built from the descend flows as well as from
     * `submenus`. Reading `screen.submenus` here was the second half of the flat-list bug: even with
     * parentage inferred, a parent with no authored submenus reported no children.
     *
     * `guard` stops a cycle from recursing forever. Descend flows are data, and A descending into B
     * while B descends into A is expressible even though the generator does not produce it.
     */
    const children: ScreenTreeNode[] = [];
    if (!guard.has(screen.id)) {
      guard.add(screen.id);
      const childIds = childrenOf.get(screen.id) ?? [];
      childIds.forEach((childId, childIndex) => {
        const childScreen = screenMap.get(childId);
        if (!childScreen) return;
        const suffix = childIndex + 1;
        const childLabel = indexLabel ? `${indexLabel}-${suffix}` : `${suffix}`;
        const childNode = visit(childScreen, depth + 1, childLabel, nodeBreadcrumbs, screen.id);
        childNode.submenuMeta = screen.submenus?.find((entry) => entry.screenId === childId);
        children.push(childNode);
      });
      guard.delete(screen.id);
    }

    return {
      screen,
      children,
      depth,
      path: indexLabel,
      parentId,
      submenuMeta: undefined
    };
  };

  const roots: ScreenTreeNode[] = [];
  screens.forEach((screen, index) => {
    if (parentMap.has(screen.id)) {
      return;
    }
    const label = `${index + 1}`;
    roots.push(visit(screen, 0, label, [], null));
  });

  // Capture nodes that might only appear as children but their parent is missing
  screens.forEach((screen) => {
    if (pathMap.has(screen.id)) {
      return;
    }
    const label = `${pathMap.size + 1}`;
    roots.push(visit(screen, 0, label, [], parentMap.get(screen.id) ?? null));
  });

  return { roots, pathMap, parentMap, breadcrumbsMap };
}

export function flattenTree(nodes: ScreenTreeNode[]): ScreenTreeNode[] {
  const list: ScreenTreeNode[] = [];
  const walk = (node: ScreenTreeNode) => {
    list.push(node);
    node.children.forEach(walk);
  };
  nodes.forEach(walk);
  return list;
}
