// Fake Wi-Fi for the simulator: a few networks, and a connection that succeeds when
// the password is "secret123" (or the network is open).

#include <cstdio>
#include <cstring>

#include "services/wifi_control.h"

namespace services::wifi {
namespace {

constexpr unsigned long kScanMs = 1200;
constexpr unsigned long kConnectMs = 1500;

struct Fake {
  const char* ssid;
  int8_t rssi;
  bool secured;
};
const Fake kNetworks[] = {
    {"HomeNet", -45, true},      {"Office WiFi", -55, true}, {"Cafe Guest", -60, false},
    {"Neighbour_5G", -70, true}, {"Linksys", -82, true},     {"Guest lobby", -84, false},
    {"Ziggo1234567", -88, true},
};

unsigned long s_scan_started = 0;
bool s_scanning = false;
bool s_scan_fails = false;

char s_ssid[kSsidMax + 1] = "HomeNet";
bool s_connected = true;
ConnectState s_state = ConnectState::kIdle;
unsigned long s_connect_started = 0;
bool s_will_succeed = false;
char s_target[kSsidMax + 1] = {};

}  // namespace

void scanStart() {
  s_scanning = true;
  s_scan_started = millis();
}

bool scanDone() {
  if (s_scanning && millis() - s_scan_started >= kScanMs) {
    s_scanning = false;
  }
  return !s_scanning;
}

bool scanFailed() { return s_scan_fails; }

size_t scanResults(Network* out, size_t max) {
  if (s_scan_fails) {
    return 0;
  }
  size_t n = 0;
  for (const Fake& fake : kNetworks) {
    if (n >= max) {
      break;
    }
    std::snprintf(out[n].ssid, sizeof(out[n].ssid), "%s", fake.ssid);
    out[n].rssi = fake.rssi;
    out[n].secured = fake.secured;
    ++n;
  }
  return n;
}

void connectBegin(const char* ssid, const char* password) {
  std::snprintf(s_target, sizeof(s_target), "%s", ssid);
  bool secured = true;
  for (const Fake& fake : kNetworks) {
    if (std::strcmp(fake.ssid, ssid) == 0) {
      secured = fake.secured;
    }
  }
  s_will_succeed = !secured || std::strcmp(password, "secret123") == 0;
  s_state = ConnectState::kConnecting;
  s_connect_started = millis();
}

ConnectState connectPoll() {
  if (s_state == ConnectState::kConnecting && millis() - s_connect_started >= kConnectMs) {
    if (s_will_succeed) {
      std::snprintf(s_ssid, sizeof(s_ssid), "%s", s_target);
      s_connected = true;
      s_state = ConnectState::kConnected;
    } else {
      s_state = ConnectState::kFailed;
    }
  }
  return s_state;
}

void connectCancel() { s_state = ConnectState::kIdle; }

// A failing scan models a device whose radio is stuck retrying an unreachable network.
bool up() { return s_connected && !s_scan_fails; }

bool connected() { return up(); }

void currentSsid(char* out, size_t n) {
  std::snprintf(out, n, "%s", up() ? s_ssid : "");
}

void localIp(char* out, size_t n) {
  std::snprintf(out, n, "%s", up() ? "192.168.1.23" : "");
}

int rssi() { return up() ? -52 : 0; }

}  // namespace services::wifi

namespace sim {
void setWifiScanFails(bool fails) { services::wifi::s_scan_fails = fails; }
}  // namespace sim
