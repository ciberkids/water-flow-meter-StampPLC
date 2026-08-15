#!/usr/bin/env node
/**
 * Generates the wiki's Modbus register reference FROM the firmware headers.
 *
 *   node tools/wiki/gen-registers.mjs            print the page to stdout
 *   node tools/wiki/gen-registers.mjs --out FILE write it
 *
 * WHY THIS IS GENERATED AND NOT WRITTEN.
 *
 * A register table is the worst possible thing to hand-maintain in a wiki. There are 733 addresses
 * across two headers, an integrator decodes real hardware against them, and a wrong address does not
 * fail loudly — it returns a plausible number from the wrong field. This repository's recurring defect
 * is exactly this shape: a range hint that duplicated a descriptor, a sample table that duplicated a
 * resolver, an id list that duplicated an enum. The duplicate always wins on screen and always drifts.
 *
 * So the ADDRESSES have one home — `modbus/register_map.h` and `net/net_register_map.h` — and this
 * script reads them. What it adds is the part a header cannot carry: what an integrator has to know to
 * decode the value. Those descriptions live here, once, and the two halves are reconciled: a register
 * the headers declare and this file does not describe is an ERROR, and so is a description for a
 * register that no longer exists. Neither can be forgotten quietly, which is the only property that
 * makes a generated document trustworthy.
 */
import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const repoRoot = path.join(import.meta.dirname, "..", "..");
const firmware = (...parts) => path.join(repoRoot, "Water-Flow-Meter-PlatformIO", "src", ...parts);

const CORE_HEADER = firmware("modbus", "register_map.h");
const NET_HEADER = firmware("net", "net_register_map.h");

/** Every `inline constexpr <int type> NAME = VALUE;` in a header, as a name -> number map. */
function readConstants(file) {
  const source = fs.readFileSync(file, "utf-8");
  const out = new Map();
  // Hex as well as decimal: `kApplyMagic = 0x5AA5` is the one that matters, and a decimal-only
  // pattern skipped it silently — the generator then crashed on an undefined, which is the loud
  // failure this comment is here to keep loud.
  const pattern =
    /inline\s+constexpr\s+(?:uint16_t|uint8_t|std::size_t)\s+(\w+)\s*=\s*(0x[0-9a-fA-F]+|\d+)\s*;/g;
  for (const match of source.matchAll(pattern)) {
    out.set(match[1], Number(match[2]));
  }
  return out;
}

const core = readConstants(CORE_HEADER);
const net = readConstants(NET_HEADER);

/**
 * How a value is laid out on the wire.
 *
 * Read off `RegisterBank::setFloat` / `setDouble`, which write the HIGH word first, and off
 * `net_register_map.h`'s stated convention for text. Named rather than repeated per row so the page
 * explains each encoding once and every row just points at one.
 */
const ENCODING = {
  u16: { regs: 1, label: "`uint16`", decode: "one register" },
  i16: { regs: 1, label: "`int16`", decode: "one register, two's complement" },
  f32: { regs: 2, label: "`float32`", decode: "2 registers, IEEE-754, **high word first**" },
  f64: { regs: 4, label: "`float64`", decode: "4 registers, IEEE-754, **high word first**" },
  ipv4: { regs: 2, label: "`uint32`", decode: "2 registers, high word first: `a.b.c.d` as `(a<<24)|(b<<16)|(c<<8)|d`" },
  mac: { regs: 3, label: "`bytes[6]`", decode: "3 registers, 2 bytes each, high byte first" },
  text: { regs: null, label: "`text`", decode: "2 characters per register, **high byte first**, NUL-padded" }
};

const RW = { r: "R", w: "W", rw: "R/W" };

/* ── The global block ─────────────────────────────────────────────────────────────────────────── */
const GLOBAL = [
  ["REG_POLLING_RATE_KHZ", "r", "u16", "kHz", "Live pulse-sampling rate. Compare against the baseline in the MQTT diagnostics payload; a fall here is an under-sampling regression (§2.1.2)."],
  ["REG_CONNECTED_SENSORS_BITMAP", "r", "u16", "—", "Bit *n* set = sensor *n+1* is in use. The persisted companion to each channel's own status flag."],
  ["REG_MASTER_RESET_ALL_SENSORS", "w", "u16", "—", "Write `1` to clear every channel's totals, session and peak. Any other value is ignored."],
  ["REG_MASTER_RESET_ALL_MEASURED", "w", "u16", "—", "Write `1` to clear every channel's measured values but keep its calibration."],
  ["REG_MASTER_RESET_ALL_SESSION", "w", "u16", "—", "Write `1` to clear every channel's session volume only."],
  ["REG_MASTER_RESET_ALL_MAX", "w", "u16", "—", "Write `1` to clear every channel's peak flow and nothing else. The cheapest reset here: the peak is volatile, never written to NVS, so a power cycle already clears it. Its own command because the alternatives destroy something — register 22 takes the session volume with it, register 21 the lifetime total."],
  ["REG_UNDERSAMPLING_FLAGS", "r", "u16", "—", "Bit *n* set = sensor *n+1* is pulsing faster than the sampler can count, so its readings are low. Also published on the MQTT diagnostics topic."],
  ["REG_LED_RED_VOLUME_STEP", "rw", "u16", "L", "How many litres of cumulative volume make the red LED pulse once."],
  ["REG_LED_RED_PULSE_PERIOD", "rw", "u16", "ms", "How long that pulse lasts."],
  ["REG_DISPLAY_FLOW_UNIT", "rw", "u16", "enum", "What the PANEL shows flows in: `0` L/min, `1` L/s, `2` m³/h. **A display preference only.** Every wire surface — these registers, MQTT, Home Assistant — stays L/min regardless, so a master must never rescale because somebody changed the screen."]
];

// Every row is [name, access, encoding, unit, meaning]. Asserted rather than tolerated: a row missing
// its encoding used to be silently reshaped here, which is how a `float32` field would end up
// documented as a `uint16` and an integrator would decode half a number.
for (const row of GLOBAL) {
  if (row.length !== 5) {
    throw new Error(`global row ${row[0]} has ${row.length} fields, expected 5`);
  }
}
const GLOBAL_ROWS = GLOBAL;

/* ── The serial link block (Project_document.md §4.1.1) ───────────────────────────────────────── */
const LINK = [
  ["REG_LINK_SLAVE_ID", "rw", "u16", "—", "Staged. 1..247."],
  ["REG_LINK_BAUD_INDEX", "rw", "u16", "index", "Staged. An index into the supported baud list, not the baud rate itself."],
  ["REG_LINK_PARITY", "rw", "u16", "enum", "Staged. `0` none, `1` odd, `2` even."],
  ["REG_LINK_STOP_BITS", "rw", "u16", "—", "Staged. 1 or 2."],
  ["REG_LINK_APPLY", "w", "u16", "magic", "Write `0x5AA5` to commit the four staged values and re-bind the port. See *The apply protocol* below."],
  ["REG_LINK_REVISION", "r", "u16", "—", "Increments on every successful apply, so a master can confirm its write took effect."]
];

/* ── One sensor's block ───────────────────────────────────────────────────────────────────────── */
const SENSOR = [
  ["OFF_STATUS_FLAGS", "r", "u16", "—", "Bit 0 = in use, bit 1 = calibration valid. A channel that is not in use publishes zeros for every measured field rather than being absent."],
  ["OFF_INSTANT_FLOW", "r", "f32", "L/min", "Instantaneous flow."],
  ["OFF_CUMULATIVE_LITERS", "r", "f64", "L", "Lifetime volume. `float64` because litre resolution over years of accumulation exceeds `float32`."],
  ["OFF_CUMULATIVE_M3", "r", "f64", "m³", "The same lifetime volume in cubic metres — the figure the panel shows."],
  ["OFF_SESSION_LITERS", "r", "f32", "L", "Volume since the last session reset."],
  ["OFF_SESSION_M3", "r", "f32", "m³", "The same session volume in cubic metres. `float32`, unlike the lifetime pair: a session is short enough that it cannot outrun the precision."],
  ["OFF_MAX_FLOW", "r", "f32", "L/min", "Peak flow since the last reset. Volatile — it is not persisted across a reboot."],
  ["OFF_CMD_RESET_SESSION", "w", "u16", "—", "Write `1` to clear this channel's session volume."],
  ["OFF_CMD_RESET_ALL", "w", "u16", "—", "Write `1` to clear this channel's totals, session and peak."],
  ["OFF_CMD_RESET_CONFIG", "w", "u16", "—", "Write `1` to decommission this channel: the calibration returns to defaults **and every measurement is destroyed** — cumulative volume, session volume, peak and pulse count — with the zeroed lifetime total persisted to NVS. The name undersells it; use `OFF_CMD_RESET_CALIBRATION` if the totals must survive."],
  ["OFF_CFG_Q_MAX", "rw", "u16", "L/min", "Rated maximum flow, from the meter's datasheet."],
  ["OFF_CFG_F_MULT", "rw", "i16", "—", "Formula calibration: the multiplier in `F = mult*Q + adjust`. Used as a DIVISOR when converting pulses to flow, so `0` is refused and the legal range is 1..32767."],
  ["OFF_CFG_ADJUST", "rw", "i16", "—", "Formula calibration: the additive term in `F = mult*Q + adjust`."],
  ["OFF_CFG_CAL_TYPE", "rw", "u16", "enum", "How this channel is calibrated: `0` formula (uses the two fields above), `1` pulses per litre (uses the field below)."],
  ["OFF_CFG_PULSES_PER_L", "rw", "u16", "pulses/L", "Pulses-per-litre calibration, exact. The formula fields cannot express it: `f_multiplier` is an integer used as a divisor, so a 450 pulses/L meter would need 7.5 and the nearest integers carry a 6 % error on every reading."],
  ["OFF_CMD_RESET_CALIBRATION", "w", "u16", "—", "Write `1` to return this channel's calibration to defaults and **nothing else** — cumulative volume, session volume and peak are all kept and keep accumulating. For the meter swap: a broken sensor replaced by one with different characteristics, where the volume already measured is real. Clears the channel's Nyquist override with it, so the replacement's first figures are sampling-checked rather than inheriting the old meter's exemption. The channel reads `SET?` until new figures are entered, because a defaulted config has `q_max = 0` and so fails `configIsValid`."]
];

/* ── The network block (WiFi_MQTT_Connectivity.md §5) ─────────────────────────────────────────── */
const NETWORK = [
  ["kWifiEnabled", "rw", "u16", 1, "Staged. `0`/`1`. With WiFi disabled and no credentials the radio is never powered at all."],
  ["kWifiState", "r", "u16", 1, "`0` disabled, `1` idle, `2` connecting, `3` connected, `4` retrying, `5` AP portal, `6` failed."],
  ["kWifiRssi", "r", "i16", 1, "dBm, negative. Only meaningful while connected."],
  ["kWifiIp", "r", "ipv4", 2, "Station address, `0` before DHCP completes."],
  ["kWifiMac", "r", "mac", 3, "Station MAC. Its last three bytes are the `<mac-suffix>` every MQTT identity is derived from."],
  ["kWifiSsid", "rw", "text", 16, "Staged. 32 bytes."],
  ["kWifiPsk", "w", "text", 32, "Staged. 64 bytes. **Reads back as zeros** — an operator secret the device never hands out."],
  ["kMqttEnabled", "rw", "u16", 1, "Staged. `0`/`1`."],
  ["kMqttState", "r", "u16", 1, "Broker connection state."],
  ["kMqttPort", "rw", "u16", 1, "Staged. Default `1883`."],
  ["kMqttPeriodS", "rw", "u16", 1, "Staged. Publish period in seconds, clamped to 1..3600, default 10. Note the 60 s heartbeat below becomes the effective cadence above 60."],
  ["kMqttFlags", "rw", "u16", 1, "Staged. Bit 0 Home Assistant discovery, bit 1 QoS 1 for telemetry. **Bit 2 is reserved** (it briefly carried a TLS toggle; TLS is out of scope and the bit was left unused rather than reassigned). Writing this register sets every flag, so a read-modify-write is mandatory."],
  ["kMqttLastCmdResult", "r", "u16", 1, "Result of the last MQTT-originated command."],
  ["kMqttHost", "rw", "text", 32, "Staged. 64 bytes."],
  ["kMqttUser", "rw", "text", 16, "Staged. 32 bytes."],
  ["kMqttPassword", "w", "text", 16, "Staged. 32 bytes. **Reads back as zeros.**"],
  ["kMqttBaseTopic", "rw", "text", 24, "Staged. 48 bytes. Defaults to `watermeter/<mac-suffix>`. Refused — not repaired — if it contains `+`, `#`, an interior `//`, or a leading or trailing `/`."],
  ["kMqttPrefix", "rw", "text", 16, "Staged. 32 bytes. The Home Assistant discovery prefix, default `homeassistant`."],
  ["kPortalRemainingS", "r", "u16", 1, "Seconds left before the provisioning AP shuts itself down. `0` when no portal is open."],
  ["kApSsid", "r", "text", 16, "The SSID the device broadcasts while provisioning: `water_flow_meter_<n>`, stable for the life of the board."],
  ["kApPassword", "r", "text", 16, "The AP passphrase, **readable in clear** — deliberately, unlike every other secret here. It describes an access point the device is broadcasting, which anyone in radio range can already see, and a remote operator needs it to direct somebody standing at the panel."],
  ["kApIp", "r", "ipv4", 2, "The device's address on its own AP."],
  ["kPortalReset", "w", "u16", 1, "Write `0x5AA5` to restore the portal login to `admin`/`admin`. Acts IMMEDIATELY rather than staging — a recovery action that needs a second write is one somebody gets half-way through."],
  ["kPortalUser", "rw", "text", 8, "Staged. 16 bytes."],
  ["kPortalPassword", "w", "text", 16, "Staged. 32 bytes. **Reads back as zeros.**"],
  ["kApply", "w", "u16", 1, "Write `0x5AA5` to commit every staged field in this block."],
  ["kRevision", "r", "u16", 1, "Increments on every successful apply."],
  ["kLastError", "r", "u16", 1, "Why the last apply failed: `0` none, `1` nothing staged, `2` bad magic, `3` invalid value."]
];

/* ── Reconciliation: the two halves must describe the same set ─────────────────────────────────── */
const problems = [];

function reconcile(label, declared, described) {
  for (const name of declared) {
    if (!described.has(name)) {
      problems.push(`${label}: ${name} is declared in the header but described nowhere in this generator`);
    }
  }
  for (const name of described) {
    if (!declared.has(name)) {
      problems.push(`${label}: ${name} is described here but no longer declared in the header`);
    }
  }
}

const declaredGlobal = new Set([...core.keys()].filter((n) => n.startsWith("REG_") && !n.startsWith("REG_LINK_")));
const declaredLink = new Set([...core.keys()].filter((n) => n.startsWith("REG_LINK_")));
const declaredSensor = new Set([...core.keys()].filter((n) => n.startsWith("OFF_")));
const declaredNet = new Set(
  [...net.keys()].filter((n) => n.startsWith("k") && !["kBase", "kEnd", "kApplyMagic"].includes(n))
);

reconcile("global", declaredGlobal, new Set(GLOBAL_ROWS.map((r) => r[0])));
reconcile("serial link", declaredLink, new Set(LINK.map((r) => r[0])));
reconcile("sensor block", declaredSensor, new Set(SENSOR.map((r) => r[0])));
reconcile("network block", declaredNet, new Set(NETWORK.map((r) => r[0])));

if (problems.length > 0) {
  console.error("The register reference and the firmware headers disagree:\n");
  for (const problem of problems) console.error(`  - ${problem}`);
  console.error(
    "\nAdd or remove the description above. A register nobody documented is a register an\n" +
      "integrator decodes by guessing, which is the failure this check exists to prevent."
  );
  process.exit(1);
}

/* ── Emit ─────────────────────────────────────────────────────────────────────────────────────── */
const numSensors = core.get("kNumSensors");
const blockSize = core.get("SENSOR_BLOCK_SIZE");
const sensorBase = core.get("SENSOR_1_BASE_ADDR");
const space = core.get("kHoldingRegisterSpace");
const defaultSlave = core.get("kDefaultModbusSlaveId");

const lines = [];
const w = (line = "") => lines.push(line);

w("# Modbus registers");
w();
w("> **Generated** by `tools/wiki/gen-registers.mjs` from `modbus/register_map.h` and");
w("> `net/net_register_map.h`. Do not edit this page — edit the headers, or the descriptions in the");
w("> generator, and re-run it. The two are reconciled: a register declared in a header and described");
w("> nowhere fails the generator, and so does a description for a register that no longer exists.");
w();
w("Every address below is a **holding register** (function codes 3 read, 6 and 16 write).");
w(`The device answers for addresses \`0\` to \`${space - 1}\`; anything beyond returns ILLEGAL_DATA_ADDRESS.`);
w(`Default slave id \`${defaultSlave}\`.`);
w();
w("## How to decode a value");
w();
w("| Encoding | Layout |");
w("| --- | --- |");
for (const key of Object.keys(ENCODING)) {
  w(`| ${ENCODING[key].label} | ${ENCODING[key].decode} |`);
}
w();
w("Multi-register values are **high word first**, which is the common Modbus convention but not a");
w("universal one — a client configured for little-endian word order reads a plausible and wrong");
w("number, silently. There is no scaling anywhere: a flow register holds L/min as a real number, not");
w("a fixed-point integer.");
w();

w("## Global registers");
w();
w("| Address | Name | Access | Type | Unit | Meaning |");
w("| --- | --- | --- | --- | --- | --- |");
for (const [name, access, enc, unit, note] of GLOBAL_ROWS) {
  w(`| \`${core.get(name)}\` | \`${name}\` | ${RW[access]} | ${ENCODING[enc].label} | ${unit} | ${note} |`);
}
w();

w("## Serial link");
w();
w("Changing the port the master is talking over cannot take effect field by field — a slave id that");
w("applied before the baud rate would strand the master mid-conversation. So these four are **staged**");
w("and applied together.");
w();
w("| Address | Name | Access | Type | Unit | Meaning |");
w("| --- | --- | --- | --- | --- | --- |");
for (const [name, access, enc, unit, note] of LINK) {
  w(`| \`${core.get(name)}\` | \`${name}\` | ${RW[access]} | ${ENCODING[enc].label} | ${unit} | ${note} |`);
}
w();

w("## Per-sensor blocks");
w();
w(
  `${numSensors} channels, ${blockSize} registers each, starting at \`${sensorBase}\`. ` +
    `Sensor *n* (1-based) begins at \`${sensorBase} + (n-1) x ${blockSize}\`:`
);
w();
w(
  "| " +
    Array.from({ length: numSensors }, (_, i) => `S${i + 1}`).join(" | ") +
    " |"
);
w("| " + Array.from({ length: numSensors }, () => "---").join(" | ") + " |");
w(
  "| " +
    Array.from({ length: numSensors }, (_, i) => `\`${sensorBase + i * blockSize}\``).join(" | ") +
    " |"
);
w();
w("Offsets within a block:");
w();
w("| Offset | Name | Access | Type | Unit | Meaning |");
w("| --- | --- | --- | --- | --- | --- |");
for (const [name, access, enc, unit, note] of SENSOR) {
  w(`| \`+${core.get(name)}\` | \`${name}\` | ${RW[access]} | ${ENCODING[enc].label} | ${unit} | ${note} |`);
}
w();
const usedOffsets = Math.max(...SENSOR.map((r) => core.get(r[0]) + ENCODING[r[2]].regs));
w(
  `Offsets \`+${usedOffsets}\` to \`+${blockSize - 1}\` are unassigned and read as zero — headroom, so a new ` +
    "per-channel field never has to move an address an integrator has already deployed against."
);
w();
w("### Reading sensor 3's flow, as an example");
w();
w("```");
w(`sensor 3 base   = ${sensorBase} + (3-1) x ${blockSize} = ${sensorBase + 2 * blockSize}`);
w(`flow offset     = +${core.get("OFF_INSTANT_FLOW")}  (float32, 2 registers)`);
w(`read holding    = ${sensorBase + 2 * blockSize + core.get("OFF_INSTANT_FLOW")}, count 2`);
w("decode          = high word first -> IEEE-754 float32 -> L/min");
w("```");
w();
w("Check bit 0 of that block's status flags first. A channel that is not in use reads zero for every");
w("measured field, and zero flow is also a legitimate reading from a working meter.");
w();

w("## Network block");
w();
w(
  `Addresses \`${net.get("kBase")}\` to \`${net.get("kEnd") - 1}\`, placed above the sensor blocks so the ` +
    `channel count can grow into \`${sensorBase + numSensors * blockSize}\`..\`${net.get("kBase") - 1}\` ` +
    "without moving anything."
);
w();
w("| Address | Name | Access | Type | Regs | Meaning |");
w("| --- | --- | --- | --- | --- | --- |");
for (const [name, access, enc, regs, note] of NETWORK) {
  w(`| \`${net.get(name)}\` | \`${name}\` | ${RW[access]} | ${ENCODING[enc].label} | ${regs} | ${note} |`);
}
w();
w("Writing a read-only address in this block is **not an error** — it is ignored — so a block write");
w("spanning the whole region succeeds rather than excepting part-way through.");
w();

w("## The apply protocol");
w();
w("One mechanism, two instances: the serial link block and the network block both stage writes and");
w(`commit on a magic value of \`0x${net.get("kApplyMagic").toString(16).toUpperCase()}\`.`);
w();
w("```");
w("1. write the value registers      -> STAGED, not in force, and they read back as staged");
w("2. write 0x5AA5 to the apply reg  -> validated, then committed together");
w("3. read the revision register     -> incremented, so the write is confirmed");
w("   (or read the error register)   -> why it was refused, if it was");
w("```");
w();
w("Staging is what makes a remote reconfiguration safe over the very link being reconfigured, and the");
w("revision register is what makes success observable rather than assumed. Two exceptions, both");
w("deliberate: the sensor commands and the portal-login reset act immediately, because a reset that");
w("needs a second write is one an operator gets half-way through.");
w();

const page = lines.join("\n") + "\n";
const outIndex = process.argv.indexOf("--out");
if (outIndex !== -1 && process.argv[outIndex + 1]) {
  fs.writeFileSync(process.argv[outIndex + 1], page);
  console.error(`wrote ${process.argv[outIndex + 1]} (${page.split("\n").length} lines)`);
} else {
  process.stdout.write(page);
}
