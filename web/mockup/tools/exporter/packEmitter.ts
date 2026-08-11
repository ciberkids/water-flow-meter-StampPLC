// Emits a `.uipack` binary: a relocatable, offset-based menu the firmware reads in place.
//
// Loadable_UI_Menu_Packs.md §3.2. The layout is defined by this file and by
// Water-Flow-Meter-PlatformIO/src/ui/pack/ui_pack.cpp, and by nothing else — no struct is cast
// over the bytes on either side, because a cast would inherit whatever padding and endianness
// the compiler chose and this file runs on a developer's machine while the reader runs on an
// ESP32.
//
// The two are kept in step by a ROUND-TRIP test rather than by review: the host test emits the
// real 48-screen dataset with this code, reads it back with the real C++ reader, and asserts
// every screen, element and flow matches. Two implementations of one format is exactly the shape
// of bug this project keeps finding, so the agreement is checked by execution.
import type { ScreenDataset, ScreenDefinition, ScreenElement } from "../../src/types.js";

const MAGIC = "WFMUI\0";
const HEADER_BYTES = 64;
/**
 * 2: the screen record grew by eight bytes for screen visibility.
 *
 * Bumped so a v1 pack is REFUSED rather than read at the wrong stride, which would misparse every
 * screen after the first. Must match `MenuPack::kFormatVersion`.
 */
const FORMAT_VERSION = 2;
const LABEL_BYTES = 20;

/**
 * 24, not 16: the record carries `visibleWhenStr` and `visibleWhenEquals` so a pack can express
 * screen visibility, which the built-in menu already uses for the calibration branch. Must match
 * `MenuPack::kScreenRecordBytes`, and the format version moved with it — see FORMAT_VERSION.
 */
const SCREEN_RECORD_BYTES = 24;
const ELEMENT_RECORD_BYTES = 20;
const FLOW_RECORD_BYTES = 16;
const LEVEL_RECORD_BYTES = 8;

/** Sentinel for "this flow does not navigate", distinct from screen index 0. */
export const NO_TARGET_SCREEN = 0xffff;

/** Mirrors ui_exporter::ElementType in GeneratedUi.h — the firmware's own numbering. */
const ELEMENT_KIND: Record<string, number> = {
  text: 0,
  value: 1,
  badge: 2,
  box: 3,
  icon: 4,
  scrollbar: 5
};

const TRIGGER_KIND: Record<string, number> = { button: 0, timeout: 1, data: 2 };
const BUTTON: Record<string, number> = { none: 0, up: 1, down: 2, enter: 3 };
const GESTURE: Record<string, number> = { short: 0, long: 1, hold: 2 };
const ALIGN: Record<string, number> = { left: 0, center: 1, right: 2 };
const EMPHASIS: Record<string, number> = { normal: 0, muted: 1, strong: 2 };

/**
 * A deduplicated string block.
 *
 * §3.2 notes the dataset repeats "< BACK" and the footer hints across many screens. Offset 0 is
 * reserved as "no string" — the reader treats a zero `*Str` as absent — so the block always opens
 * with a lone NUL that nothing else can occupy.
 */
class StringTable {
  private readonly offsets = new Map<string, number>();
  private readonly chunks: Buffer[] = [Buffer.from([0])];
  private length = 1;

  intern(value: string | undefined | null): number {
    if (value === undefined || value === null || value === "") {
      return 0;
    }
    const existing = this.offsets.get(value);
    if (existing !== undefined) {
      return existing;
    }
    const offset = this.length;
    const encoded = Buffer.from(value + "\0", "utf-8");
    this.chunks.push(encoded);
    this.length += encoded.length;
    this.offsets.set(value, offset);
    return offset;
  }

  build(): Buffer {
    return Buffer.concat(this.chunks);
  }

  get byteLength(): number {
    return this.length;
  }
}

/** CRC-32 (IEEE 802.3, reflected, 0xEDB88320) — must match ui::packCrc32 exactly. */
export function packCrc32(bytes: Buffer): number {
  let crc = 0xffffffff;
  for (const byte of bytes) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit += 1) {
      crc = (crc >>> 1) ^ (0xedb88320 & -(crc & 1));
    }
  }
  return (~crc) >>> 0;
}

export interface PackOptions {
  /** Shown in the firmware's menu selector; truncated to 20 bytes. */
  label: string;
  /** The catalogue version this pack targets (§4.7b). */
  catalogueAbi: number;
}

export interface PackResult {
  bytes: Buffer;
  screenCount: number;
  stringBytes: number;
  /** Bytes saved by deduplication, for the export report. */
  dedupSaved: number;
}

export function emitPack(dataset: ScreenDataset, options: PackOptions): PackResult {
  const strings = new StringTable();
  const screens = dataset.screens;
  const indexById = new Map<string, number>();
  screens.forEach((screen, i) => indexById.set(screen.id, i));

  let rawStringBytes = 0;
  const intern = (value: string | undefined | null): number => {
    if (value) rawStringBytes += Buffer.byteLength(value, "utf-8") + 1;
    return strings.intern(value);
  };

  // Interned first so the string block is complete before any offset is computed. Element and
  // flow bodies are laid out per screen, contiguously, so a screen's flows begin immediately
  // after its elements — which is how the reader derives flowsOffset from elementsOffset without
  // spending four more bytes per screen on it.
  interface Body {
    elements: Buffer;
    flows: Buffer;
  }

  const bodies: Body[] = screens.map((screen) => ({
    elements: encodeElements(screen, intern),
    flows: encodeFlows(screen, indexById, intern)
  }));

  // ── Offsets ──
  const levelsOffset = HEADER_BYTES;
  const levelsBytes = LEVEL_RECORD_BYTES;  // one level for now; §3.1 allows more
  const screensOffset = levelsOffset + levelsBytes;
  const bodiesOffset = screensOffset + screens.length * SCREEN_RECORD_BYTES;

  const screenRecords = Buffer.alloc(screens.length * SCREEN_RECORD_BYTES);
  let cursor = bodiesOffset;
  screens.forEach((screen, i) => {
    const body = bodies[i];
    const at = i * SCREEN_RECORD_BYTES;
    screenRecords.writeUInt32LE(intern(screen.id), at);
    screenRecords.writeUInt32LE(intern(screen.name ?? screen.id), at + 4);
    screenRecords.writeUInt16LE(screen.elements.length, at + 8);
    screenRecords.writeUInt16LE((screen.flows ?? []).length, at + 10);
    screenRecords.writeUInt32LE(cursor, at + 12);
    // Screen visibility. Offset 0 is the string table's own empty string, which is the format's
    // established "no string" sentinel — so an unconditional screen needs no special value.
    screenRecords.writeUInt32LE(intern(screen.visibleWhen?.binding), at + 16);
    screenRecords.writeInt32LE(screen.visibleWhen?.equals ?? 0, at + 20);
    cursor += body.elements.length + body.flows.length;
  });

  // Interned here, BEFORE the string block is built. Interning it after — which the first
  // version did — put its offset past the end of the block, so the reader could not resolve it.
  // The C++ validator caught that the moment it started checking level records.
  const labelStr = intern(options.label);

  const bodyBlock = Buffer.concat(bodies.flatMap((b) => [b.elements, b.flows]));
  const themeOffset = cursor;
  const themeBlock = encodeTheme(dataset, intern);
  const stringsOffset = themeOffset + themeBlock.length;

  // Interning during layout can add strings, so the block is rebuilt after every offset that
  // needs it is known. The offsets above do not depend on the block's contents, only on its
  // position, so this is safe — and it is checked by the round-trip test rather than assumed.
  const finalStrings = strings.build();

  const levels = Buffer.alloc(LEVEL_RECORD_BYTES);
  levels.writeUInt32LE(labelStr, 0);
  levels.writeUInt16LE(screens.length, 4);
  levels.writeUInt16LE(0, 6);

  const payload = Buffer.concat([levels, screenRecords, bodyBlock, themeBlock, finalStrings]);

  const header = Buffer.alloc(HEADER_BYTES);
  header.write(MAGIC, 0, "latin1");
  header.writeUInt16LE(FORMAT_VERSION, 6);
  header.writeUInt16LE(options.catalogueAbi, 8);
  header.writeUInt32LE(payload.length, 10);
  header.writeUInt32LE(packCrc32(payload), 14);
  header.writeUInt16LE(1, 18);                 // levelCount
  header.writeUInt16LE(screens.length, 20);
  header.writeUInt32LE(levelsOffset, 22);
  header.writeUInt32LE(screensOffset, 26);
  header.writeUInt32LE(themeOffset, 30);
  header.writeUInt32LE(stringsOffset, 34);
  header.writeUInt32LE(finalStrings.length, 38);
  header.write(options.label.slice(0, LABEL_BYTES), 42, "utf-8");

  return {
    bytes: Buffer.concat([header, payload]),
    screenCount: screens.length,
    stringBytes: finalStrings.length,
    dedupSaved: Math.max(0, rawStringBytes - finalStrings.length)
  };
}

function encodeElements(
  screen: ScreenDefinition,
  intern: (value: string | undefined | null) => number
): Buffer {
  const out = Buffer.alloc(screen.elements.length * ELEMENT_RECORD_BYTES);
  screen.elements.forEach((element: ScreenElement, i: number) => {
    const at = i * ELEMENT_RECORD_BYTES;
    out.writeUInt8(ELEMENT_KIND[element.kind] ?? 0, at);
    out.writeUInt8(ALIGN[element.align ?? "left"] ?? 0, at + 1);
    out.writeUInt8(EMPHASIS[element.emphasis ?? "normal"] ?? 0, at + 2);
    out.writeInt16LE(element.x, at + 4);
    out.writeInt16LE(element.y, at + 6);
    out.writeInt16LE(element.width ?? 0, at + 8);
    out.writeInt16LE(element.height ?? 0, at + 10);
    out.writeUInt32LE(intern(element.content), at + 12);
    out.writeUInt32LE(intern(element.binding), at + 16);
  });
  return out;
}

function encodeFlows(
  screen: ScreenDefinition,
  indexById: Map<string, number>,
  intern: (value: string | undefined | null) => number
): Buffer {
  const flows = screen.flows ?? [];
  const out = Buffer.alloc(flows.length * FLOW_RECORD_BYTES);
  flows.forEach((flow, i) => {
    const at = i * FLOW_RECORD_BYTES;
    const trigger = flow.trigger;
    out.writeUInt8(TRIGGER_KIND[trigger.type] ?? 0, at);

    // Narrowed POSITIVELY by trigger.type. Only ButtonFlowTrigger has `button` and `gesture`,
    // only TimeoutFlowTrigger has `durationMs` and `holdButton`, and DataFlowTrigger has none of
    // them — so reaching for a field across the union does not compile. §3.8's discriminator
    // lives here: a timeout carrying `holdButton: "enter"` is a hold countdown, and its ABSENCE
    // is what marks an unattended auto-timeout. Same rule cppEmitter applies.
    let button: string = "none";
    let gesture: string = "short";
    let durationMs = 0;
    switch (trigger.type) {
      case "button":
        button = trigger.button;
        gesture = trigger.gesture ?? "short";
        break;
      case "timeout":
        button = trigger.holdButton ?? "none";
        durationMs = trigger.durationMs;
        break;
      case "data":
        break;
    }
    out.writeUInt8(BUTTON[button] ?? 0, at + 1);
    out.writeUInt8(GESTURE[gesture] ?? 0, at + 2);
    out.writeUInt32LE(durationMs, at + 4);
    const target = flow.targetScreenId ? indexById.get(flow.targetScreenId) : undefined;
    // Resolved to an index at build time, so the device bounds-checks one integer per press
    // instead of doing a string lookup. An unresolvable target is a build error, not a runtime
    // surprise — the exporter's screen-coverage gate is what catches it first.
    out.writeUInt16LE(target ?? NO_TARGET_SCREEN, at + 8);
    out.writeUInt32LE(intern(flow.actionId), at + 12);
  });
  return out;
}

function encodeTheme(
  dataset: ScreenDataset,
  intern: (value: string | undefined | null) => number
): Buffer {
  const colors = Object.entries(dataset.theme?.colors ?? {});
  const out = Buffer.alloc(2 + colors.length * 8 + 6);
  out.writeUInt16LE(colors.length, 0);
  colors.forEach(([key, value], i) => {
    const at = 2 + i * 8;
    out.writeUInt32LE(intern(key), at);
    out.writeUInt32LE(argbFromHex(String(value)), at + 4);
  });
  const typographyAt = 2 + colors.length * 8;
  const typography = dataset.theme?.typography;
  out.writeUInt16LE(typography?.base ?? 8, typographyAt);
  out.writeUInt16LE(typography?.value ?? 10, typographyAt + 2);
  out.writeUInt16LE(typography?.badge ?? 8, typographyAt + 4);
  return out;
}

function argbFromHex(hex: string): number {
  const cleaned = hex.replace("#", "");
  const expanded =
    cleaned.length === 3
      ? cleaned.split("").map((c) => c + c).join("")
      : cleaned.padEnd(6, "0").slice(0, 6);
  return (0xff000000 | parseInt(expanded, 16)) >>> 0;
}
