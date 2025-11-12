import { useEffect, useRef, useState } from "react";
import type { ScreenDataset } from "../../types";

interface LiveJsonEditorPanelProps {
  dataset: ScreenDataset;
  onApplyDataset: (nextDataset: ScreenDataset) => void;
  validateDatasetSafe: (raw: unknown) => { dataset: ScreenDataset | null; issues: string[] };
}

export function LiveJsonEditorPanel({
  dataset,
  onApplyDataset,
  validateDatasetSafe
}: LiveJsonEditorPanelProps) {
  const [draft, setDraft] = useState(() => JSON.stringify(dataset, null, 2));
  const [status, setStatus] = useState<"idle" | "success" | "error">("idle");
  const [issues, setIssues] = useState<string[]>([]);
  const skipSync = useRef(false);

  useEffect(() => {
    if (skipSync.current) {
      skipSync.current = false;
      return;
    }
    setDraft(JSON.stringify(dataset, null, 2));
  }, [dataset]);

  const handleChange = (value: string) => {
    setDraft(value);
    try {
      const parsed = JSON.parse(value);
      const { dataset: validated, issues: schemaIssues } = validateDatasetSafe(parsed);
      if (!validated || schemaIssues.length > 0) {
        setStatus("error");
        setIssues(schemaIssues.length ? schemaIssues : ["Schema validation failed"]);
        return;
      }
      skipSync.current = true;
      onApplyDataset(validated);
      setStatus("success");
      setIssues([]);
    } catch (error) {
      setStatus("error");
      setIssues([error instanceof Error ? error.message : String(error)]);
    }
  };

  return (
    <section className="json-editor-panel" data-testid="json-editor-panel">
      <header>
        <h3>Live JSON</h3>
        <p>Edits update the dataset immediately when the JSON parses and passes schema validation.</p>
      </header>
      <textarea
        value={draft}
        onChange={(event) => handleChange(event.target.value)}
        rows={18}
        spellCheck={false}
        data-testid="live-json-editor"
        className={status === "error" ? "has-error" : ""}
      />
      <div className={`json-editor-status json-editor-status--${status}`}>
        {status === "success" && <span>Dataset synced.</span>}
        {status === "error" && (
          <ul>
            {issues.map((issue) => (
              <li key={issue}>{issue}</li>
            ))}
          </ul>
        )}
        {status === "idle" && <span>Editing…</span>}
      </div>
    </section>
  );
}
