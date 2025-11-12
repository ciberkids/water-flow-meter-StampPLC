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

  screens.forEach((screen) => {
    screen.submenus?.forEach((submenu) => {
      parentMap.set(submenu.screenId, screen.id);
    });
  });

  const pathMap = new Map<string, string>();
  const breadcrumbsMap = new Map<string, string[]>();

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

    const children: ScreenTreeNode[] =
      screen.submenus
        ?.map((submenu, childIndex) => {
          const childScreen = screenMap.get(submenu.screenId);
          if (!childScreen) {
            return null;
          }
          const suffix = childIndex + 1;
          const childLabel = indexLabel ? `${indexLabel}-${suffix}` : `${suffix}`;
          const childNode = visit(childScreen, depth + 1, childLabel, nodeBreadcrumbs, screen.id);
          childNode.submenuMeta = submenu;
          return childNode;
        })
        .filter((node): node is ScreenTreeNode => Boolean(node)) ?? [];

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
