#pragma once

#include <cstddef>
#include <cstdint>

#include "ui/core/ui_settings_types.h"

namespace ui {

/**
 * The firmware's declaration of every value the on-screen UI can bind to.
 *
 * This table is the SOURCE of the manifest that the web tool validates designs against
 * (decision D2). `tools/manifest_gen` reads it and emits
 * `web/mockup/src/data/actionManifest.json`; CI fails when the committed manifest and a
 * fresh generation disagree.
 *
 * Before this existed the manifest was maintained by hand, and it was wrong: it declared
 * `sensor.N.cumulativeM3` and `sensor.N.sessionM3` as having no register, while
 * `modbus_manager.cpp` has always published them at offsets 7 and 13. A Modbus master
 * could read registers the tooling insisted did not exist. Generating removes the class
 * of bug rather than that one instance.
 *
 * Deliberately free of Arduino headers so the generator is a plain host program and the
 * table can be exercised by `test/host/run.sh`.
 */

/**
 * The catalogue's ABI version (Loadable_UI_Menu_Packs.md §4.7b).
 *
 * A menu pack records the version it was built against. The firmware accepts a pack targeting
 * this version or an OLDER one — the completeness rule means an older pack can only be missing
 * editors the firmware can supply itself — and refuses a newer one, which may reference values
 * that do not exist here.
 *
 * **BUMP THIS ON EVERY ADDITION.** Owner's decision, 2026-08-21, reversing what this comment said
 * before: it used to exempt additions as "backward compatible", and they are — a pack that predates a
 * new value still loads and still works. The reason to bump anyway is that nothing could otherwise
 * tell "this pack was built before that setting existed" from "this pack is missing an editor it
 * should have carried", and those want opposite responses. With a bump per addition, every id records
 * the ABI it appeared at and the exporter can warn about the first while still failing the second
 * (`Loadable_UI_Menu_Packs.md` §3.0.1, and N-b in the open register).
 *
 * A removal or a rename is forbidden outright — the catalogue is append-only (project rule I2) — and
 * `tools/catalogue/check-ledger.mjs` refuses both, along with an addition that forgot to bump this.
 *
 * The cost, stated: a pack stamped with a NEWER ABI than the firmware is refused, so moving a pack
 * from a newer device to an older one now fails where an addition-only change used to be tolerated.
 * That is the trade for being able to tell the two failures apart.
 */
constexpr uint16_t kUiCatalogueAbi = 1;

enum class ValueCategory : uint8_t { Setting, Reading, Accumulated, System, Derived };

enum class ValueType : uint8_t { Number, String, Boolean };

/**
 * Which resolver arm serves a value.
 *
 * **This does NOT produce a build failure, despite what this comment used to claim.** Nothing
 * switches on ValueSource: `UiBindingResolver` dispatches on the binding STRING, so adding a source
 * without teaching the resolver about it compiles fine and renders blank — the failure mode that
 * produced an empty display twice in this project's history, which the old comment asserted was
 * impossible.
 *
 * The guarantee that does exist is the exporter's `firmware-manifest-resolvable` gate: it asks the
 * firmware to resolve every value the manifest advertises and fails the export when one cannot be.
 * That is a stronger check than a switch arm, because it exercises real resolution rather than the
 * presence of a case label — but it fires at export time, not at compile time. Adding a value here
 * without a resolver arm therefore fails the gate, not the build.
 */
enum class ValueSource : uint8_t { SensorMetric, Setting, Telemetry, Diagnostics, UiState, Network };

/** One metric within a sensor's register block, instantiated once per sensor. */
struct SensorMetric {
  /** Appended to `sensor.<n>.` to form the binding id. */
  const char* suffix;
  ValueCategory category;
  ValueType type;
  const char* unit;
  /** Offset within the sensor block; see modbus/register_map.h. */
  uint16_t offset;
  /** Rendered as "Sensor <n> <descriptionSuffix>". */
  const char* descriptionSuffix;
};

/** A value with a fixed id — system aggregates, diagnostics and UI-derived strings. */
struct SimpleValue {
  const char* id;
  ValueCategory category;
  ValueType type;
  const char* unit;
  /** Absolute holding register, or kNoRegister for values computed in firmware. */
  uint16_t registerAddress;
  ValueSource source;
  /**
   * True when the value describes "the current sensor" rather than a fixed one. Unlike a
   * SensorMetric this is a single id, resolved against whichever sensor the navigation
   * level selected.
   */
  bool perSensor;
  const char* description;
};

std::size_t sensorMetricCount();
const SensorMetric* sensorMetricAt(std::size_t index);

std::size_t simpleValueCount();
const SimpleValue* simpleValueAt(std::size_t index);

/** Number of sensors the catalogue expands per-sensor metrics across. */
std::size_t catalogueSensorCount();

/** Absolute register for a metric on a 1-based sensor number. */
uint16_t sensorMetricRegister(const SensorMetric& metric, std::size_t sensorNumber);

}  // namespace ui
