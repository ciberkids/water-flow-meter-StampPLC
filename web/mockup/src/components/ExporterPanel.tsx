import { useCallback, useState } from "react";

interface ExportSummary {
  generatedAt?: string;
  screens?: number;
  elements?: number;
  output?: string;
  backup?: string | null;
}

type ValidationStatus = "pass" | "warning" | "fail";

interface ValidationCheck {
  id: string;
  title: string;
  status: ValidationStatus;
  message: string;
  screenId?: string;
  elementId?: string;
  recommendation?: string;
}

interface ValidationReport {
  status: ValidationStatus;
  checks: ValidationCheck[];
  issues: string[];
  log?: string;
}

interface AutomationCheck {
  id: string;
  title: string;
  status: ValidationStatus;
  message: string;
  details?: string;
  durationMs?: number;
  command?: string;
  log?: string;
}

interface BackupSummary {
  attempted: boolean;
  created: boolean;
  location?: string | null;
  restored?: boolean;
  reason?: string;
}

interface ExportResponse {
  status: string;
  summary?: ExportSummary;
  message?: string;
  issues?: string[];
  validation?: ValidationReport;
  compilation?: AutomationCheck;
  backup?: BackupSummary;
}

type ExportState = "idle" | "running" | "success" | "error";

interface ExporterPanelProps {
  disabled?: boolean;
  onNavigateToScreen?: (screenId: string) => void;
}

const statusLabels: Record<ValidationStatus, string> = {
  pass: "Pass",
  warning: "Needs attention",
  fail: "Blocked"
};

const statusClasses: Record<ValidationStatus, string> = {
  pass: "status-success",
  warning: "status-warning",
  fail: "status-error"
};

function downloadTextFile(filename: string, contents: string) {
  const blob = new Blob([contents], { type: "text/plain" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = filename;
  anchor.click();
  URL.revokeObjectURL(url);
}

export function ExporterPanel({ disabled = false, onNavigateToScreen }: ExporterPanelProps) {
  const [state, setState] = useState<ExportState>("idle");
  const [summary, setSummary] = useState<ExportSummary | null>(null);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const [issues, setIssues] = useState<string[]>([]);
  const [validationReport, setValidationReport] = useState<ValidationReport | null>(null);
  const [compilationReport, setCompilationReport] = useState<AutomationCheck | null>(null);
  const [backupSummary, setBackupSummary] = useState<BackupSummary | null>(null);

  const triggerExport = useCallback(async () => {
    setState("running");
    setSummary(null);
    setErrorMessage(null);
    setIssues([]);
    setValidationReport(null);
    setCompilationReport(null);
    setBackupSummary(null);
    try {
      const response = await fetch("/api/export", {
        method: "POST",
        headers: { "Content-Type": "application/json" }
      });
      const payload = (await response.json()) as ExportResponse;
      setValidationReport(payload.validation ?? null);
      setCompilationReport(payload.compilation ?? null);
      setBackupSummary(payload.backup ?? null);
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

  const handleValidationNavigate = useCallback(
    (screenId: string | undefined) => {
      if (screenId && onNavigateToScreen) {
        onNavigateToScreen(screenId);
      }
    },
    [onNavigateToScreen]
  );

  const handleValidationLogDownload = useCallback(() => {
    if (validationReport?.log) {
      downloadTextFile("ui-validation-log.txt", validationReport.log);
    }
  }, [validationReport]);

  const handleCompileLogDownload = useCallback(() => {
    if (compilationReport?.log) {
      downloadTextFile("platformio-compile.log", compilationReport.log);
    }
  }, [compilationReport]);

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

      {summary && state === "success" && (
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

      {validationReport && (
        <section className="validation-panel">
          <header className="validation-panel__header">
            <div>
              <strong>Validation checks</strong>
              <span className={`status-tag ${statusClasses[validationReport.status]}`}>
                {statusLabels[validationReport.status]}
              </span>
            </div>
            {validationReport.log && (
              <button
                type="button"
                className="tool-button tool-button--secondary"
                onClick={handleValidationLogDownload}
              >
                Download log
              </button>
            )}
          </header>
          <ul className="validation-checklist">
            {validationReport.checks.map((check) => (
              <li key={check.id} className={`validation-check validation-check--${check.status}`}>
                <div className="validation-check__header">
                  <span>{check.title}</span>
                  <span className={`status-tag ${statusClasses[check.status]}`}>
                    {statusLabels[check.status]}
                  </span>
                </div>
                <p>{check.message}</p>
                {check.recommendation && (
                  <p className="validation-check__recommendation">{check.recommendation}</p>
                )}
                {check.screenId && (
                  <button
                    type="button"
                    className="text-link-button"
                    onClick={() => handleValidationNavigate(check.screenId)}
                  >
                    View {check.screenId}
                  </button>
                )}
              </li>
            ))}
          </ul>
        </section>
      )}

      {compilationReport && (
        <section className="automation-panel">
          <header className="validation-panel__header">
            <div>
              <strong>{compilationReport.title}</strong>
              <span className={`status-tag ${statusClasses[compilationReport.status]}`}>
                {statusLabels[compilationReport.status]}
              </span>
            </div>
            {compilationReport.log && (
              <button
                type="button"
                className="tool-button tool-button--secondary"
                onClick={handleCompileLogDownload}
              >
                Download compile log
              </button>
            )}
          </header>
          <p>{compilationReport.message}</p>
          {compilationReport.command && (
            <p className="automation-panel__command">
              <code>{compilationReport.command}</code>
            </p>
          )}
          {compilationReport.details && <p className="automation-panel__details">{compilationReport.details}</p>}
          {typeof compilationReport.durationMs === "number" && (
            <p className="automation-panel__details">
              Duration: {compilationReport.durationMs.toFixed(0)} ms
            </p>
          )}
        </section>
      )}

      {backupSummary && (
        <section className="backup-panel">
          <strong>Backup verification</strong>
          <dl>
            <div>
              <dt>Attempted</dt>
              <dd>{backupSummary.attempted ? "Yes" : "No"}</dd>
            </div>
            <div>
              <dt>Created</dt>
              <dd>{backupSummary.created ? "Yes" : "No"}</dd>
            </div>
            {"location" in backupSummary && (
              <div>
                <dt>Location</dt>
                <dd>{backupSummary.location ? <code>{backupSummary.location}</code> : "—"}</dd>
              </div>
            )}
            {backupSummary.restored && (
              <div>
                <dt>Restore</dt>
                <dd>Previous export restored after failure.</dd>
              </div>
            )}
            {backupSummary.reason && (
              <div>
                <dt>Notes</dt>
                <dd>{backupSummary.reason}</dd>
              </div>
            )}
          </dl>
        </section>
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
