import { CSSProperties, useEffect, useMemo, useRef } from "react";

interface DeviceGridProps {
  width: number;
  height: number;
  visible: boolean;
  minorColor: string;
  majorColor: string;
}

const GRID_MINOR_PITCH = 1;
const GRID_MAJOR_PITCH = 8;

export function DeviceGrid({ width, height, visible, minorColor, majorColor }: DeviceGridProps) {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) {
      return;
    }

    if (!visible) {
      const ctx = canvas.getContext("2d");
      if (ctx) {
        ctx.clearRect(0, 0, canvas.width, canvas.height);
      }
      return;
    }

    const devicePixelRatio = window.devicePixelRatio || 1;
    const targetWidth = Math.max(1, Math.floor(width * devicePixelRatio));
    const targetHeight = Math.max(1, Math.floor(height * devicePixelRatio));

    if (canvas.width !== targetWidth) {
      canvas.width = targetWidth;
    }
    if (canvas.height !== targetHeight) {
      canvas.height = targetHeight;
    }
    canvas.style.width = `${width}px`;
    canvas.style.height = `${height}px`;

    const ctx = canvas.getContext("2d");
    if (!ctx) {
      return;
    }

    ctx.setTransform(devicePixelRatio, 0, 0, devicePixelRatio, 0, 0);
    ctx.clearRect(0, 0, width, height);
    ctx.imageSmoothingEnabled = false;

    ctx.fillStyle = minorColor;
    for (let x = GRID_MINOR_PITCH; x < width; x += GRID_MINOR_PITCH) {
      if (x % GRID_MAJOR_PITCH !== 0) {
        ctx.fillRect(x, 0, 1, height);
      }
    }
    for (let y = GRID_MINOR_PITCH; y < height; y += GRID_MINOR_PITCH) {
      if (y % GRID_MAJOR_PITCH !== 0) {
        ctx.fillRect(0, y, width, 1);
      }
    }

    ctx.fillStyle = majorColor;
    for (let x = 0; x < width; x += GRID_MAJOR_PITCH) {
      ctx.fillRect(x, 0, 1, height);
    }
    ctx.fillRect(width - 1, 0, 1, height);

    for (let y = 0; y < height; y += GRID_MAJOR_PITCH) {
      ctx.fillRect(0, y, width, 1);
    }
    ctx.fillRect(0, height - 1, width, 1);
  }, [width, height, visible, majorColor, minorColor]);

  const canvasStyle = useMemo<CSSProperties>(
    () => ({
      position: "absolute",
      inset: 0,
      pointerEvents: "none",
      mixBlendMode: "normal",
      opacity: visible ? 1 : 0,
      zIndex: 1
    }),
    [visible]
  );

  return <canvas ref={canvasRef} className="grid-overlay" style={canvasStyle} aria-hidden />;
}
