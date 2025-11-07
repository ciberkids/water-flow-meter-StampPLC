import { useCallback, useState } from "react";

interface ExportSummary {
  generatedAt?: string;
  screens?: number;
  elements?: number;
  output?: string;
  backup?: string | null;
}

interface ExportResponse {
  status: string;
  summary?: ExportSummary;
  message?: string;
  issues?: string[];
}

type ExportState = "idle" | "running" | "success" | "error";

interface ExporterPanelProps {
  disabled?: boolean;
}

export function ExporterPanel({ disabled = false }: ExporterPanelProps) {
  const [state, setState] = useState<ExportState>("idle");
  const [summary, setSummary] = useState<ExportSummary | null>(null);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [issues, setIssues] = useState<string[]>([]);

  const triggerExport = useCallback(async () => {
    setState("running");
    setSummary(null);
    setErrorMessage(null);
    setIssues([]);
    try {
      const response = await fetch("/api/export", {
        method: "POST",
        headers: { "Content-Type": "application/json" }
      });
      const payload = (await response.json()) as ExportResponse;
      if (!response.ok || payload.status !== "ok") {
        setState("error");
        setErrorMessage(payload.message ?? "Export failed");
        setIssues(payload.issues ?? []);
        return;
      }
      setSummary(payload.summary ?? null);
      setState("success");
    } catch (error) {
      setState("error");
      setErrorMessage(error instanceof Error ? error.message : String(error));
    }
  }, []);

  return (
    <section>
      <h2>Export</h2>
      <p className="exporter-description">
        Validate the current mockup dataset and generate firmware assets (IR +
        C++). Previous exports are backed up automatically.
      </p>
      <button
        className="tool-button exporter-button"
        onClick={triggerExport}
        disabled={state === "running" || disabled}
        type="button"
      >
        {state === "running" ? "Exporting…" : "Export to Firmware"}
      </button>
      {disabled && state !== "running" && (
        <p className="exporter-hint">Resolve validation issues before exporting.</p>
      )}
      {state === "success" && summary && (
        <dl className="exporter-summary">
          {summary.generatedAt && (
            <>
              <dt>Generated</dt>
              <dd>{new Date(summary.generatedAt).toLocaleString()}</dd>
            </>
          )}
          {typeof summary.screens === "number" && (
            <>
              <dt>Screens</dt>
              <dd>{summary.screens}</dd>
            </>
          )}
          {typeof summary.elements === "number" && (
            <>
              <dt>Elements</dt>
              <dd>{summary.elements}</dd>
            </>
          )}
          {summary.output && (
            <>
              <dt>Output</dt>
              <dd><code>{summary.output}</code></dd>
            </>
          )}
          {"backup" in summary && (
            <>
              <dt>Backup</dt>
              <dd>{summary.backup ?? "No previous export detected"}</dd>
            </>
          )}
        </dl>
      )}
      {state === "error" && (
        <div className="exporter-error" role="alert">
          <p>{errorMessage ?? "Export failed"}</p>
          {issues.length > 0 && (
            <ul>
              {issues.map((issue) => (
                <li key={issue}>{issue}</li>
              ))}
            </ul>
          )}
        </div>
      )}
    </section>
  );
}
