import { useCallback, useEffect, useRef, useState } from "react";
import type { ScreenDefinition } from "../../types";
import type { ScreenGraphicAsset } from "../../types";

interface AnimationInspectorProps {
  screen?: ScreenDefinition;
  onUploadFrames: (files: FileList) => void;
  onReorderFrame: (assetId: string, frameIndex: number, direction: "up" | "down") => void;
}

export function AnimationInspector({
  screen,
  onUploadFrames,
  onReorderFrame
}: AnimationInspectorProps) {
  const [previewAsset, setPreviewAsset] = useState<string | null>(null);
  const [previewIndex, setPreviewIndex] = useState(0);
  const timerRef = useRef<number | null>(null);

  const assets = screen?.assets?.filter((asset) => asset.type === "svg-sequence") ?? [];

  useEffect(() => {
    return () => {
      if (timerRef.current) {
        window.clearInterval(timerRef.current);
      }
    };
  }, []);

  const startPreview = (asset: ScreenGraphicAsset) => {
    stopPreview();
    if (!asset.embeddedFrames || asset.embeddedFrames.length === 0) {
      setPreviewAsset(asset.id);
      setPreviewIndex(0);
      return;
    }
    setPreviewAsset(asset.id);
    setPreviewIndex(0);
    if (timerRef.current) {
      window.clearInterval(timerRef.current);
    }
    timerRef.current = window.setInterval(() => {
      setPreviewIndex((current) => {
        const next = (current + 1) % asset.embeddedFrames!.length;
        return next;
      });
    }, 500);
  };

  const stopPreview = useCallback(() => {
    if (timerRef.current) {
      window.clearInterval(timerRef.current);
    }
    timerRef.current = null;
    setPreviewAsset(null);
    setPreviewIndex(0);
  }, []);

  useEffect(() => {
    stopPreview();
  }, [screen?.id, stopPreview]);

  const currentPreview = assets.find((asset) => asset.id === previewAsset);
  const currentFrameSrc =
    currentPreview?.embeddedFrames && currentPreview.embeddedFrames[previewIndex]
      ? currentPreview.embeddedFrames[previewIndex]
      : undefined;

  return (
    <section className="animation-inspector" data-testid="animation-inspector">
      <header>
        <h3>Animation inspector</h3>
        <p>Upload SVG frames to build sequences, reorder them, and preview the playback.</p>
        <label className="tool-button">
          Upload SVG frames
          <input
            type="file"
            accept=".svg"
            multiple
            hidden
            data-testid="animation-upload"
            disabled={!screen}
            onChange={(event) => {
              const files = event.target.files;
              if (files && files.length > 0) {
                onUploadFrames(files);
                event.target.value = "";
              }
            }}
          />
        </label>
      </header>

      {assets.length === 0 ? (
        <p>No animation assets yet.</p>
      ) : (
        <div className="animation-assets">
          {assets.map((asset) => (
            <div key={asset.id} className="animation-card">
              <div className="animation-card__header">
                <strong>{asset.id}</strong>
                <span>{asset.frames?.length ?? 0} frames</span>
              </div>
              <ul>
                {(asset.frames ?? []).map((frame, index) => (
                  <li key={`${asset.id}-${frame}-${index}`}>
                    <span>{frame}</span>
                    <div className="frame-actions">
                      <button
                        type="button"
                        className="tool-button tool-button--secondary"
                        onClick={() => onReorderFrame(asset.id, index, "up")}
                        disabled={index === 0}
                      >
                        ↑
                      </button>
                      <button
                        type="button"
                        className="tool-button tool-button--secondary"
                        onClick={() => onReorderFrame(asset.id, index, "down")}
                        disabled={index === (asset.frames?.length ?? 0) - 1}
                      >
                        ↓
                      </button>
                    </div>
                  </li>
                ))}
              </ul>
              <div className="animation-preview">
                <button type="button" className="tool-button" onClick={() => startPreview(asset)}>
                  Preview
                </button>
                <button type="button" className="tool-button tool-button--secondary" onClick={stopPreview}>
                  Stop
                </button>
              </div>
            </div>
          ))}
        </div>
      )}

      {currentFrameSrc ? (
        <div className="animation-preview-stage">
          <img src={currentFrameSrc} alt="Animation preview" />
        </div>
      ) : (
        <div className="animation-preview-stage placeholder">Select an asset to preview</div>
      )}
    </section>
  );
}
