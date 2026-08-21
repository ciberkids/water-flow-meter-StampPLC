/**
 * Emits the firmware manifest the web design tool validates against.
 *
 * Run via `tools/manifest_gen/run.sh`, which writes
 * `web/mockup/src/data/actionManifest.json`. CI regenerates and fails on any diff, so the
 * manifest cannot drift from the firmware it claims to describe (decision D2).
 *
 * Before this, the manifest was maintained by hand. It over-claimed (eight actions with no
 * handler) and under-claimed (two per-sensor registers the firmware has always published).
 * Both classes of error are now structurally impossible: the actions are cross-checked
 * against the handler table at compile time, and every register comes from register_map.h.
 */
#include <cstdio>
#include <string>
#include <vector>

#include "ui/core/ui_action_catalogue.h"
#include "ui/core/ui_pages.h"
#include "ui/core/ui_settings_types.h"
#include "ui/core/ui_value_catalogue.h"

namespace {

std::string escape(const char* text) {
  std::string out;
  for (const char* p = text; p != nullptr && *p != '\0'; ++p) {
    if (*p == '"' || *p == '\\') {
      out.push_back('\\');
    }
    out.push_back(*p);
  }
  return out;
}

const char* categoryName(ui::ValueCategory category) {
  switch (category) {
    case ui::ValueCategory::Setting:     return "setting";
    case ui::ValueCategory::Reading:     return "reading";
    case ui::ValueCategory::Accumulated: return "accumulated";
    case ui::ValueCategory::System:      return "system";
    case ui::ValueCategory::Derived:     return "derived";
  }
  return "unknown";
}

const char* typeName(ui::ValueType type) {
  switch (type) {
    case ui::ValueType::Number:  return "number";
    case ui::ValueType::String:  return "string";
    case ui::ValueType::Boolean: return "boolean";
  }
  return "unknown";
}

/** The JSON `type` a setting presents to the design tool. */
const char* settingTypeName(ui::SettingKind kind) {
  switch (kind) {
    case ui::SettingKind::Numeric: return "number";
    case ui::SettingKind::Enum:    return "number";
    case ui::SettingKind::Boolean: return "boolean";
    case ui::SettingKind::Text:    return "string";
  }
  return "unknown";
}

struct Field {
  std::string key;
  std::string value;  // Pre-rendered JSON.
};

std::string str(const char* text) { return "\"" + escape(text) + "\""; }
std::string num(long long value) { return std::to_string(value); }

void emitObject(const std::vector<Field>& fields, const char* indent, bool last) {
  std::printf("%s{\n", indent);
  for (std::size_t i = 0; i < fields.size(); ++i) {
    std::printf("%s  \"%s\": %s%s\n", indent, fields[i].key.c_str(), fields[i].value.c_str(),
                i + 1 < fields.size() ? "," : "");
  }
  std::printf("%s}%s\n", indent, last ? "" : ",");
}

}  // namespace

int main() {
  std::printf("{\n");
  // Format 3: enums carry their register value rather than a bare label list, and
  // per-sensor settings carry their offset. There is deliberately no `generatedAt` — this
  // file is committed and diffed against a fresh generation, so a timestamp would make
  // every run look like a change. Provenance is git history.
  std::printf("  \"version\": \"3\",\n");
  // The CATALOGUE's ABI, which is a different number from the manifest format's version above and
  // must not be confused with it: `version` describes the shape of this file, `catalogueAbi` describes
  // the vocabulary it lists. A menu pack records the latter in its header, and the firmware refuses a
  // pack targeting a NEWER one (Loadable_UI_Menu_Packs §4.7b, Q4).
  //
  // Emitted from `ui::kUiCatalogueAbi` rather than written here, so the number a pack is stamped with
  // and the number the firmware compares against are the same constant. It was a literal `1` in the
  // pack emitter's tests and nowhere else until 2026-08-21, which meant nothing connected the two.
  std::printf("  \"catalogueAbi\": %u,\n", static_cast<unsigned>(ui::kUiCatalogueAbi));

  // ── Actions ──────────────────────────────────────────────────────────────────
  std::printf("  \"actions\": [\n");
  for (std::size_t i = 0; i < kActionCatalogueCount; ++i) {
    const ActionDescriptor& action = kActionCatalogue[i];
    emitObject({{"id", str(action.id)},
                {"label", str(action.label)},
                {"description", str(action.description)}},
               "    ", i + 1 == kActionCatalogueCount);
  }
  std::printf("  ],\n");

  // ── Values ───────────────────────────────────────────────────────────────────
  // Settings first, then per-sensor metrics, then the fixed-id values. The order is
  // stable so the CI diff only ever reflects a real change.
  std::printf("  \"values\": [\n");

  std::vector<std::vector<Field>> values;

  for (std::size_t i = 0; i < ui::settingCount(); ++i) {
    const ui::SettingDescriptor& setting = *ui::settingAt(i);
    std::vector<Field> fields{{"id", str(setting.bindingId)},
                              {"category", str("setting")},
                              {"type", str(settingTypeName(setting.kind))}};
    if (setting.unit != nullptr) {
      fields.push_back({"unit", str(setting.unit)});
    }
    fields.push_back({"min", num(setting.min)});
    fields.push_back({"max", num(setting.max)});
    fields.push_back({"step", num(setting.step)});
    if (setting.optionCount > 0) {
      std::string list = "[";
      for (std::uint8_t o = 0; o < setting.optionCount; ++o) {
        if (o > 0) list += ", ";
        list += "{\"label\": " + str(setting.options[o].label) +
                ", \"value\": " + num(setting.options[o].value) + "}";
      }
      list += "]";
      fields.push_back({"options", list});
    }
    if (setting.perSensor) {
      fields.push_back({"perSensor", "true"});
    }
    // Settings are the only writable values; everything else the UI can bind is a reading.
    // Derived rather than stored, so a new category cannot forget to declare it.
    // A per-sensor setting has no single register: its address depends on which sensor
    // the navigation level selected, so the offset within the block is what is stable.
    if (setting.registerAddress != ui::kNoRegister) {
      fields.push_back({"register", num(setting.registerAddress)});
    }
    if (setting.registerOffset != ui::kNoRegister) {
      fields.push_back({"registerOffset", num(setting.registerOffset)});
    }
    if (setting.kind == ui::SettingKind::Text) {
      fields.push_back({"maxLength", num(setting.maxLength)});
    }
    // A secret is advertised as such so the designer can see it will render masked, and so
    // the export can refuse to bind it anywhere it would be displayed in full.
    if (setting.writeOnly) {
      fields.push_back({"writeOnly", "true"});
    }
    fields.push_back({"description", str(setting.description)});
    values.push_back(std::move(fields));
  }

  for (std::size_t s = 1; s <= ui::catalogueSensorCount(); ++s) {
    for (std::size_t m = 0; m < ui::sensorMetricCount(); ++m) {
      const ui::SensorMetric& metric = *ui::sensorMetricAt(m);
      const std::string id = "sensor." + std::to_string(s) + "." + metric.suffix;
      const std::string description =
          "Sensor " + std::to_string(s) + " " + metric.descriptionSuffix;
      std::vector<Field> fields{{"id", str(id.c_str())},
                                {"category", str(categoryName(metric.category))},
                                {"type", str(typeName(metric.type))}};
      if (metric.unit != nullptr) {
        fields.push_back({"unit", str(metric.unit)});
      }
      // A DERIVED per-sensor value has no register, and must not claim one. `pulsesPerLitre` is the
      // case: on a pulses-calibrated channel it is register 24, but on a formula-calibrated one that
      // register holds zero and the figure is computed from the multiplier — so advertising an
      // address would send a Modbus master to read a 0 and believe it.
      if (metric.offset != ui::kNoRegister) {
        fields.push_back({"register", num(ui::sensorMetricRegister(metric, s))});
      }
      fields.push_back({"readOnly", "true"});
      fields.push_back({"description", str(description.c_str())});
      values.push_back(std::move(fields));
    }
  }

  for (std::size_t i = 0; i < ui::simpleValueCount(); ++i) {
    const ui::SimpleValue& value = *ui::simpleValueAt(i);
    std::vector<Field> fields{{"id", str(value.id)},
                              {"category", str(categoryName(value.category))},
                              {"type", str(typeName(value.type))}};
    if (value.unit != nullptr) {
      fields.push_back({"unit", str(value.unit)});
    }
    if (value.registerAddress != ui::kNoRegister) {
      fields.push_back({"register", num(value.registerAddress)});
    }
    if (value.perSensor) {
      fields.push_back({"perSensor", "true"});
    }
    fields.push_back({"readOnly", "true"});
    fields.push_back({"description", str(value.description)});
    values.push_back(std::move(fields));
  }

  for (std::size_t i = 0; i < values.size(); ++i) {
    emitObject(values[i], "    ", i + 1 == values.size());
  }
  std::printf("  ],\n");

  // ── Screens ──────────────────────────────────────────────────────────────────
  // The vocabulary whose drift produced a blank display: the router resolves these ids
  // by name, so a design that renames one must fail the export.
  std::printf("  \"screens\": [\n");
  for (std::size_t i = 0; i < kRequiredScreenCount; ++i) {
    emitObject({{"id", str(kRequiredScreens[i].id)}, {"role", str(kRequiredScreens[i].role)}},
               "    ", i + 1 == kRequiredScreenCount);
  }
  std::printf("  ]\n");

  std::printf("}\n");
  return 0;
}
