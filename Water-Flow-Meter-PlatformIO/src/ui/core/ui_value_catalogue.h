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

enum class ValueCategory : uint8_t { Setting, Reading, Accumulated, System, Derived };

enum class ValueType : uint8_t { Number, String, Boolean };

/**
 * Which resolver arm serves a value.
 *
 * `UiBindingResolver` switches on this with no `default:` label, so introducing a source
 * without teaching the resolver about it is a build failure rather than a binding that
 * silently renders as blank — the failure mode that produced an empty display twice in
 * this project's history.
 */
enum class ValueSource : uint8_t { SensorMetric, Setting, Telemetry, Diagnostics, UiState };

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
