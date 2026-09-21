#pragma once

#include <cstdint>
#include <cstring>
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

  bool isKey(const char* key) { return numbers().count(ns_ + "/" + key) > 0; }

  size_t putUChar(const char* key, uint8_t v) { return put(key, v); }
  size_t putBool(const char* key, bool v) { return put(key, v ? 1 : 0); }
  size_t putInt(const char* key, int32_t v) { return put(key, v); }
  size_t putFloat(const char* key, float v) { return put(key, v); }
  size_t putDouble(const char* key, double v) { return put(key, v); }
  uint8_t getUChar(const char* key, uint8_t def = 0) {
    return static_cast<uint8_t>(get(key, def));
  }
  bool getBool(const char* key, bool def = false) { return get(key, def ? 1 : 0) != 0; }
  int32_t getInt(const char* key, int32_t def = 0) {
    return static_cast<int32_t>(get(key, def));
  }
  float getFloat(const char* key, float def = 0.0f) {
    return static_cast<float>(get(key, def));
  }
  double getDouble(const char* key, double def = 0.0) { return get(key, def); }

  size_t putBytes(const char* key, const void* value, size_t len) {
    blobs()[ns_ + "/" + key] = std::string(static_cast<const char*>(value), len);
    return len;
  }
  size_t getBytes(const char* key, void* buf, size_t max_len) {
    const auto it = blobs().find(ns_ + "/" + key);
    if (it == blobs().end()) {
      return 0;
    }
    const size_t n = it->second.size() < max_len ? it->second.size() : max_len;
    std::memcpy(buf, it->second.data(), n);
    return n;
  }

  bool remove(const char* key) {
    const std::string full = ns_ + "/" + key;
    return numbers().erase(full) + blobs().erase(full) > 0;
  }

 private:
  static std::map<std::string, double>& numbers() {
    static std::map<std::string, double> s;
    return s;
  }
  static std::map<std::string, std::string>& blobs() {
    static std::map<std::string, std::string> s;
    return s;
  }
  size_t put(const char* key, double v) {
    numbers()[ns_ + "/" + key] = v;
    return 1;
  }
  double get(const char* key, double def) {
    const auto it = numbers().find(ns_ + "/" + key);
    return it == numbers().end() ? def : it->second;
  }
  std::string ns_;
};
