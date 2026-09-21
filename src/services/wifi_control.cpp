#include "services/wifi_control.h"

#include <WiFi.h>
#include <esp_wifi.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace services::wifi {
namespace {

constexpr unsigned long kConnectTimeoutMs = 20000;
/** Ignore transient failure statuses right after WiFi.begin(). */
constexpr unsigned long kFailureGraceMs = 2500;

bool s_scanning = false;
Network s_results[kMaxNetworks];
size_t s_result_count = 0;

ConnectState s_state = ConnectState::kIdle;
unsigned long s_started_ms = 0;
char s_previous_ssid[kSsidMax + 1] = {};
char s_previous_password[kPasswordMax + 1] = {};

bool linkUp() {
  return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0);
}

void rememberSavedNetwork() {
  s_previous_ssid[0] = '\0';
  s_previous_password[0] = '\0';
  wifi_config_t conf = {};
  if (esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK && conf.sta.ssid[0] != '\0') {
    snprintf(s_previous_ssid, sizeof(s_previous_ssid), "%s",
             reinterpret_cast<const char*>(conf.sta.ssid));
    snprintf(s_previous_password, sizeof(s_previous_password), "%s",
             reinterpret_cast<const char*>(conf.sta.password));
  }
}

void restorePreviousNetwork() {
  WiFi.disconnect(false, false);
  if (s_previous_ssid[0] != '\0') {
    WiFi.setAutoReconnect(true);
    WiFi.begin(s_previous_ssid,
               s_previous_password[0] != '\0' ? s_previous_password : nullptr);
  }
}

void ensureStaMode() {
  const wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_OFF) {
    WiFi.mode(WIFI_STA);
    delay(50);
  } else if (mode == WIFI_AP) {
    WiFi.mode(WIFI_AP_STA);
    delay(50);
  }
}

}  // namespace

void scanStart() {
  ensureStaMode();
  WiFi.scanDelete();
  WiFi.scanNetworks(/*async=*/true, /*show_hidden=*/false);
  s_scanning = true;
  s_result_count = 0;
}

bool scanDone() {
  if (!s_scanning) {
    return true;
  }
  const int found = WiFi.scanComplete();
  if (found == WIFI_SCAN_RUNNING) {
    return false;
  }
  s_scanning = false;
  s_result_count = 0;
  for (int i = 0; i < found; ++i) {
    const String name = WiFi.SSID(i);
    if (name.length() == 0 || name.length() > kSsidMax) {
      continue;
    }
    const int8_t strength = static_cast<int8_t>(WiFi.RSSI(i));
    const bool secured = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;

    // One entry per network name (access points repeat), keeping the strongest.
    size_t existing = s_result_count;
    for (size_t j = 0; j < s_result_count; ++j) {
      if (name.equals(s_results[j].ssid)) {
        existing = j;
        break;
      }
    }
    if (existing < s_result_count) {
      if (strength > s_results[existing].rssi) {
        s_results[existing].rssi = strength;
      }
      continue;
    }
    if (s_result_count < kMaxNetworks) {
      Network& entry = s_results[s_result_count++];
      snprintf(entry.ssid, sizeof(entry.ssid), "%s", name.c_str());
      entry.rssi = strength;
      entry.secured = secured;
    }
  }
  WiFi.scanDelete();
  std::sort(s_results, s_results + s_result_count,
            [](const Network& a, const Network& b) { return a.rssi > b.rssi; });
  return true;
}

size_t scanResults(Network* out, size_t max) {
  const size_t n = std::min(max, s_result_count);
  memcpy(out, s_results, n * sizeof(Network));
  return n;
}

void connectBegin(const char* ssid, const char* password) {
  rememberSavedNetwork();
  ensureStaMode();
  WiFi.persistent(true);  // keep the credentials in flash once they work
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(false, false);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(ssid, (password != nullptr && password[0] != '\0') ? password : nullptr);
  s_state = ConnectState::kConnecting;
  s_started_ms = millis();
}

ConnectState connectPoll() {
  if (s_state != ConnectState::kConnecting) {
    return s_state;
  }
  if (linkUp()) {
    WiFi.setAutoReconnect(true);
    s_state = ConnectState::kConnected;
    return s_state;
  }
  const unsigned long elapsed = millis() - s_started_ms;
  const wl_status_t status = WiFi.status();
  const bool failed = elapsed >= kFailureGraceMs &&
                      (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL);
  if (failed || elapsed >= kConnectTimeoutMs) {
    s_state = ConnectState::kFailed;
    restorePreviousNetwork();
  }
  return s_state;
}

void connectCancel() {
  if (s_state == ConnectState::kConnecting) {
    restorePreviousNetwork();
  }
  s_state = ConnectState::kIdle;
}

bool connected() { return linkUp(); }

void currentSsid(char* out, size_t n) {
  if (n == 0) {
    return;
  }
  out[0] = '\0';
  if (linkUp()) {
    snprintf(out, n, "%s", WiFi.SSID().c_str());
  }
}

void localIp(char* out, size_t n) {
  if (n == 0) {
    return;
  }
  snprintf(out, n, "%s", linkUp() ? WiFi.localIP().toString().c_str() : "");
}

int rssi() { return linkUp() ? WiFi.RSSI() : 0; }

}  // namespace services::wifi
