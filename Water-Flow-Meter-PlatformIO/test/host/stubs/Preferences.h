#pragma once

// Host stub for the Arduino Preferences (NVS) API.
//
// Enough of the surface for the UI and LED layers to link on a Linux host. Backed by a
// std::map so a test can assert what WOULD be persisted — which matters, because
// "the setting was committed but never written to NVS" is a bug class this project has
// already shipped once.
// The ESP32 core's Preferences.h pulls in Arduino.h, and modbus_manager.cpp relies on that: it calls
// millis() while including neither. Mirroring the transitive include here is what lets that file compile
// on the host at all.
#include <Arduino.h>

#include <cstdint>
#include <map>
#include <string>

class Preferences {
 public:
  /**
   * DF25 — make one key's writes fail, so a test can drive the failure path.
   *
   * OPT-IN and static: default behaviour is unchanged, because five suites and three firmware sources
   * depend on writes succeeding here. A test that wants a failure names the key and clears it after.
   * The real `Preferences::put*` return 0 on failure, which is what `failKey` reproduces.
   */
  static void failKey(const std::string& key) { failing_() = key; }
  static void failNoKeys() { failing_().clear(); }
  bool begin(const char* name, bool readOnly = false) {
    (void)name;
    (void)readOnly;
    begun_ = true;
    return true;
  }
  void end() { begun_ = false; }
  bool clear() {
    u32_.clear();
    i32_.clear();
    f32_.clear();
    return true;
  }
  bool remove(const char* key) { return u32_.erase(key) + i32_.erase(key) + f32_.erase(key) > 0; }

  std::size_t putUInt(const char* key, uint32_t value) { if (refused(key)) return 0; u32_[key] = value; return sizeof(value); }
  std::size_t putUShort(const char* key, uint16_t value) { if (refused(key)) return 0; u32_[key] = value; return sizeof(value); }
  std::size_t putUChar(const char* key, uint8_t value) { if (refused(key)) return 0; u32_[key] = value; return sizeof(value); }
  std::size_t putInt(const char* key, int32_t value) { i32_[key] = value; return sizeof(value); }
  std::size_t putShort(const char* key, int16_t value) { i32_[key] = value; return sizeof(value); }
  std::size_t putFloat(const char* key, float value) { f32_[key] = value; return sizeof(value); }
  std::size_t putDouble(const char* key, double value) { if (refused(key)) return 0; f32_[key] = value; return sizeof(value); }
  std::size_t putBool(const char* key, bool value) { u32_[key] = value ? 1 : 0; return sizeof(value); }

  uint32_t getUInt(const char* key, uint32_t fallback = 0) const { return get(u32_, key, fallback); }
  uint16_t getUShort(const char* key, uint16_t fallback = 0) const {
    return static_cast<uint16_t>(get(u32_, key, static_cast<uint32_t>(fallback)));
  }
  uint8_t getUChar(const char* key, uint8_t fallback = 0) const {
    return static_cast<uint8_t>(get(u32_, key, static_cast<uint32_t>(fallback)));
  }
  int32_t getInt(const char* key, int32_t fallback = 0) const { return get(i32_, key, fallback); }
  int16_t getShort(const char* key, int16_t fallback = 0) const {
    return static_cast<int16_t>(get(i32_, key, static_cast<int32_t>(fallback)));
  }
  float getFloat(const char* key, float fallback = 0.0f) const {
    return static_cast<float>(get(f32_, key, static_cast<double>(fallback)));
  }
  double getDouble(const char* key, double fallback = 0.0) const { return get(f32_, key, fallback); }
  bool getBool(const char* key, bool fallback = false) const {
    return get(u32_, key, static_cast<uint32_t>(fallback ? 1 : 0)) != 0;
  }
  bool isKey(const char* key) const {
    return u32_.count(key) || i32_.count(key) || f32_.count(key);
  }

  /** Test-only: has begin() been called? A missing begin() is a real firmware bug. */
  bool begun() const { return begun_; }
  std::size_t keyCount() const { return u32_.size() + i32_.size() + f32_.size(); }

  static bool refused(const char* key) { return !failing_().empty() && failing_() == key; }


 private:
  static std::string& failing_() {
    static std::string key;
    return key;
  }
  template <typename Map, typename T>
  static T get(const Map& map, const char* key, T fallback) {
    const auto it = map.find(key);
    return it == map.end() ? fallback : static_cast<T>(it->second);
  }

  std::map<std::string, uint32_t> u32_;
  std::map<std::string, int32_t> i32_;
  std::map<std::string, double> f32_;
  bool begun_ = false;
};
