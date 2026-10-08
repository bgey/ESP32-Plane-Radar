#pragma once

#include <cstddef>
#include <cstdint>

namespace services::wifi {

/**
 * Wi-Fi operations for the on-device setup screens: scan for networks and connect
 * to one. Everything is non-blocking; poll it from the UI loop. The desktop
 * simulator provides a fake implementation.
 */

constexpr size_t kSsidMax = 32;
constexpr size_t kPasswordMax = 63;
constexpr size_t kMaxNetworks = 12;

struct Network {
  char ssid[kSsidMax + 1];
  int8_t rssi;
  bool secured;
};

/** Start an asynchronous scan (results replace the previous ones). */
void scanStart();
/** True once the scan started by scanStart() has finished. */
bool scanDone();
/**
 * True when the last scan could not run at all (the radio stayed busy after a few
 * retries), as opposed to running and finding nothing.
 */
bool scanFailed();
/** Copy the scan results, strongest first, one entry per network name. */
size_t scanResults(Network* out, size_t max);

enum class ConnectState { kIdle, kConnecting, kConnected, kFailed };

/**
 * Start connecting to a network (password may be empty for an open network). The
 * credentials are saved once the connection succeeds. When it fails, the previously
 * saved network is restored.
 */
void connectBegin(const char* ssid, const char* password);
ConnectState connectPoll();
/** Abandon an attempt in progress and restore the previous network. */
void connectCancel();

bool connected();
/** Name of the network we are connected to; empty when not connected. */
void currentSsid(char* out, size_t n);
void localIp(char* out, size_t n);
int rssi();

}  // namespace services::wifi
