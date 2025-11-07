import { ScreenDefinition } from "../types";

interface ScreenSelectorProps {
  screens: ScreenDefinition[];
  activeId: string;
  onSelect: (id: string) => void;
}

export function ScreenSelector({ screens, activeId, onSelect }: ScreenSelectorProps) {
  return (
    <div className="screen-selector">
      {screens.map((screen) => (
        <button
          key={screen.id}
          type="button"
          className={screen.id === activeId ? "active" : ""}
          onClick={() => onSelect(screen.id)}
        >
          <strong>{screen.name}</strong>
          <div style={{ fontSize: "0.75rem", opacity: 0.7 }}>
            {screen.description}
          </div>
        </button>
      ))}
    </div>
  );
}

