#pragma once

#include <cstdint>
#include <map>
#include <string>

// In-memory stand-in for the ESP32 NVS wrapper; values live for one run.
class Preferences {
 public:
  bool begin(const char* name, bool /*read_only*/ = false) {
    ns_ = name;
    return true;
  }
  void end() {}

  size_t putUChar(const char* key, uint8_t v) { return put(key, v); }
  size_t putBool(const char* key, bool v) { return put(key, v ? 1 : 0); }
  uint8_t getUChar(const char* key, uint8_t def = 0) {
    return static_cast<uint8_t>(get(key, def));
  }
  bool getBool(const char* key, bool def = false) { return get(key, def ? 1 : 0) != 0; }
  bool remove(const char* key) { return store().erase(ns_ + "/" + key) > 0; }

 private:
  static std::map<std::string, long>& store() {
    static std::map<std::string, long> s;
    return s;
  }
  size_t put(const char* key, long v) {
    store()[ns_ + "/" + key] = v;
    return 1;
  }
  long get(const char* key, long def) {
    const auto it = store().find(ns_ + "/" + key);
    return it == store().end() ? def : it->second;
  }
  std::string ns_;
};
