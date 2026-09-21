#include "ui/wifi_settings.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "config.h"
#include "hardware/display.h"
#include "services/wifi_control.h"
#include "ui/radar_theme.h"
#include "ui/text_input.h"
#include "ui/ui_font.h"

namespace ui {
namespace {

namespace wf = services::wifi;

constexpr int kScreenW = config::kDisplayWidth;
constexpr int kScreenH = config::kDisplayHeight;
constexpr int kRowH = 34;
constexpr int kMargin = 12;
constexpr int kHeaderH = 44;
constexpr int kButtonH = 26;
constexpr int kScanButtonW = 72;
constexpr int kPhoneButtonW = 150;
constexpr int kBootFooterH = 40;

enum class Mode { kTab, kSsidKeyboard, kPasswordKeyboard, kConnecting, kResult };

Mode s_mode = Mode::kTab;
bool s_boot = false;
int s_top = 47;

wf::Network s_nets[wf::kMaxNetworks];
size_t s_net_count = 0;
bool s_scanning = false;
bool s_scanned = false;

char s_ssid[wf::kSsidMax + 1] = {};
char s_password[wf::kPasswordMax + 1] = {};
bool s_ssid_secured = true;

unsigned long s_anim_ms = 0;
int s_anim = 0;
bool s_result_ok = false;
unsigned long s_result_ms = 0;

// What the tab last drew, to notice a connection change while it is showing.
bool s_drawn_connected = false;
char s_drawn_ssid[wf::kSsidMax + 1] = {};

uint16_t colorGood() { return tft.color565(80, 200, 90); }
uint16_t colorBad() { return tft.color565(255, 130, 80); }
uint16_t colorDim() { return tft.color565(90, 110, 130); }

/** Copy text, shortening it with ".." so it fits in max_w pixels (font already set). */
void fitText(const char* text, int max_w, char* out, size_t n) {
  snprintf(out, n, "%s", text);
  size_t length = strlen(out);
  if (tft.textWidth(out) <= max_w) {
    return;
  }
  while (length > 3) {
    --length;
    out[length] = '\0';
    char probe[64];
    snprintf(probe, sizeof(probe), "%s..", out);
    if (tft.textWidth(probe) <= max_w) {
      snprintf(out, n, "%s", probe);
      return;
    }
  }
}

void drawSignal(int x, int cy, int rssi) {
  const int level = rssi > -55 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : 1;
  for (int i = 0; i < 4; ++i) {
    const int h = 4 + i * 4;
    tft.fillRect(x + i * 6, cy + 8 - h, 4, h, i < level ? colorGood() : colorDim());
  }
}

void drawLock(int x, int cy, uint16_t color) {
  tft.fillRect(x, cy - 1, 11, 9, color);
  tft.drawRect(x + 2, cy - 7, 7, 7, color);
}

// ---- Tab -----------------------------------------------------------------------

enum class TabKind { kNone, kScan, kNetwork, kOther };

struct TabTarget {
  TabKind kind = TabKind::kNone;
  int index = 0;
  bool operator==(const TabTarget& o) const {
    return kind == o.kind && (kind == TabKind::kNone || index == o.index);
  }
};

bool s_tab_down = false;
TabTarget s_tab_pressed;

int netRows() {
  return static_cast<int>(std::min<size_t>(s_net_count, s_boot ? 4 : 5));
}
int rowCount() { return 3 + netRows(); }
int rowTop(int row) { return s_top + row * kRowH; }
int scanButtonX() { return kScreenW - kMargin - kScanButtonW; }

TabTarget hitTab(int x, int y) {
  TabTarget target;
  if (y < s_top) {
    return target;
  }
  const int row = (y - s_top) / kRowH;
  if (row >= rowCount()) {
    return target;
  }
  if (row == 0) {
    if (!s_scanning && x >= scanButtonX() - 6) {
      target.kind = TabKind::kScan;
    }
  } else if (row >= 2 && row < 2 + netRows()) {
    target.kind = TabKind::kNetwork;
    target.index = row - 2;
  } else if (row == 2 + netRows()) {
    target.kind = TabKind::kOther;
  }
  return target;
}

int rowOf(const TabTarget& target) {
  switch (target.kind) {
    case TabKind::kScan:
      return 0;
    case TabKind::kNetwork:
      return 2 + target.index;
    case TabKind::kOther:
      return 2 + netRows();
    default:
      return -1;
  }
}

void drawTabRow(int row, bool pressed) {
  const int top = rowTop(row);
  const int cy = top + kRowH / 2;
  const uint16_t bg = pressed ? radar::kColorFooterBackground : radar::kColorBackground;

  char current[wf::kSsidMax + 1];
  wf::currentSsid(current, sizeof(current));
  const bool is_connected = wf::connected();

  tft.startWrite();
  tft.fillRect(0, top, kScreenW, kRowH, bg);
  tft.drawFastHLine(kMargin, top + kRowH - 1, kScreenW - 2 * kMargin, radar::kColorGrid);
  tft.setTextDatum(textdatum_t::middle_left);

  if (row == 0) {
    char text[64];
    char fitted[64];
    uiApplyFont(16);
    if (is_connected) {
      snprintf(text, sizeof(text), "Connected: %s", current);
    } else {
      snprintf(text, sizeof(text), "Not connected");
    }
    fitText(text, scanButtonX() - kMargin - 12, fitted, sizeof(fitted));
    tft.setTextColor(is_connected ? colorGood() : colorBad(), bg);
    tft.drawString(fitted, kMargin, cy);
    if (s_scanning) {
      uiApplyFont(13);
      tft.setTextDatum(textdatum_t::middle_right);
      tft.setTextColor(colorDim(), bg);
      tft.drawString("Scanning...", kScreenW - kMargin, cy);
    } else {
      uiDrawButton(scanButtonX(), cy - kButtonH / 2, kScanButtonW, kButtonH, "SCAN", 13,
                   false, false);
    }
  } else if (row == 1) {
    uiApplyFont(13);
    tft.setTextColor(colorDim(), bg);
    char text[96];
    if (is_connected) {
      char ip[20];
      wf::localIp(ip, sizeof(ip));
      snprintf(text, sizeof(text), "IP %s    signal %d dBm", ip, wf::rssi());
    } else if (s_scanning) {
      snprintf(text, sizeof(text), "Looking for networks...");
    } else if (s_scanned && s_net_count == 0) {
      snprintf(text, sizeof(text), "No networks found. Tap SCAN to try again.");
    } else {
      snprintf(text, sizeof(text), "Tap a network to connect to it.");
    }
    tft.drawString(text, kMargin, cy);
  } else if (row < 2 + netRows()) {
    const wf::Network& net = s_nets[row - 2];
    const bool is_current = is_connected && strcmp(net.ssid, current) == 0;
    if (is_current) {
      tft.fillCircle(kMargin + 6, cy, 6, colorGood());
    } else {
      tft.drawCircle(kMargin + 6, cy, 6, radar::kColorGrid);
    }
    uiApplyFont(16);
    char fitted[40];
    fitText(net.ssid, kScreenW - kMargin - 22 - 80 - kMargin, fitted, sizeof(fitted));
    tft.setTextColor(radar::kColorLabel, bg);
    tft.drawString(fitted, kMargin + 22, cy);
    drawSignal(kScreenW - kMargin - 24, cy, net.rssi);
    if (net.secured) {
      drawLock(kScreenW - kMargin - 24 - 22, cy, colorDim());
    }
  } else {
    uiApplyFont(16);
    tft.setTextColor(radar::kColorTagType, bg);
    tft.drawString("+  Other network (type its name)", kMargin, cy);
  }
  tft.setTextDatum(textdatum_t::top_left);
  tft.endWrite();
}

void drawTab() {
  // In boot mode the bottom strip holds the PHONE SETUP button; leave it alone.
  const int bottom = s_boot ? kScreenH - kBootFooterH : kScreenH;
  tft.startWrite();
  tft.fillRect(0, s_top, kScreenW, bottom - s_top, radar::kColorBackground);
  for (int row = 0; row < rowCount(); ++row) {
    drawTabRow(row, false);
  }
  tft.endWrite();
  s_drawn_connected = wf::connected();
  wf::currentSsid(s_drawn_ssid, sizeof(s_drawn_ssid));
}

void beginScan() {
  wf::scanStart();
  s_scanning = true;
}

/** Advance the scan and notice connection changes; redraws the tab when needed. */
void tick() {
  if (s_scanning && wf::scanDone()) {
    s_net_count = wf::scanResults(s_nets, wf::kMaxNetworks);
    s_scanning = false;
    s_scanned = true;
    drawTab();
    return;
  }
  char current[wf::kSsidMax + 1];
  wf::currentSsid(current, sizeof(current));
  if (wf::connected() != s_drawn_connected || strcmp(current, s_drawn_ssid) != 0) {
    drawTab();
  }
}

// ---- Password / connecting / result screens -------------------------------------------

void openPasswordKeyboard() {
  char title[48];
  snprintf(title, sizeof(title), "Password for %s", s_ssid);
  keyboardOpen(title, "", wf::kPasswordMax, true);
  s_mode = Mode::kPasswordKeyboard;
}

void drawConnecting() {
  tft.startWrite();
  tft.fillScreen(radar::kColorBackground);
  tft.setTextDatum(textdatum_t::middle_center);
  uiApplyFont(16);
  tft.setTextColor(colorDim(), radar::kColorBackground);
  tft.drawString("Connecting to", kScreenW / 2, 96);
  uiApplyFont(24);
  char fitted[40];
  fitText(s_ssid, kScreenW - 40, fitted, sizeof(fitted));
  tft.setTextColor(radar::kColorLabel, radar::kColorBackground);
  tft.drawString(fitted, kScreenW / 2, 134);
  tft.setTextDatum(textdatum_t::top_left);
  uiDrawButton(170, 236, 140, 48, "CANCEL", 16, false, false);
  tft.endWrite();
}

void drawConnectingDots() {
  static const char* const kDots[] = {".", "..", "...", "...."};
  tft.startWrite();
  tft.fillRect(0, 160, kScreenW, 40, radar::kColorBackground);
  uiApplyFont(24);
  tft.setTextDatum(textdatum_t::middle_center);
  tft.setTextColor(radar::kColorTagAltitude, radar::kColorBackground);
  tft.drawString(kDots[s_anim % 4], kScreenW / 2, 178);
  tft.setTextDatum(textdatum_t::top_left);
  tft.endWrite();
}

void startConnect() {
  wf::connectBegin(s_ssid, s_password);
  s_mode = Mode::kConnecting;
  s_anim = 0;
  s_anim_ms = millis();
  drawConnecting();
  drawConnectingDots();
}

void drawResult() {
  tft.startWrite();
  tft.fillScreen(radar::kColorBackground);
  tft.setTextDatum(textdatum_t::middle_center);
  uiApplyFont(24);
  tft.setTextColor(s_result_ok ? colorGood() : colorBad(), radar::kColorBackground);
  tft.drawString(s_result_ok ? "Connected" : "Could not connect", kScreenW / 2, 84);
  uiApplyFont(16);
  char fitted[40];
  fitText(s_ssid, kScreenW - 40, fitted, sizeof(fitted));
  tft.setTextColor(radar::kColorLabel, radar::kColorBackground);
  tft.drawString(fitted, kScreenW / 2, 128);
  uiApplyFont(13);
  tft.setTextColor(colorDim(), radar::kColorBackground);
  if (s_result_ok) {
    char ip[20];
    char text[40];
    wf::localIp(ip, sizeof(ip));
    snprintf(text, sizeof(text), "IP %s", ip);
    tft.drawString(text, kScreenW / 2, 160);
  } else {
    tft.drawString("Check the password and the signal, then try again.", kScreenW / 2, 160);
  }
  tft.setTextDatum(textdatum_t::top_left);
  if (!s_result_ok) {
    uiDrawButton(60, 236, 170, 48, "TRY AGAIN", 16, false, true);
    uiDrawButton(250, 236, 170, 48, "BACK", 16, false, false);
  }
  tft.endWrite();
}

void showResult(bool ok) {
  s_result_ok = ok;
  s_result_ms = millis();
  s_mode = Mode::kResult;
  drawResult();
}

void returnToTab() {
  s_mode = Mode::kTab;
  s_tab_down = false;
  s_tab_pressed = TabTarget{};
}

bool s_modal_touch_down = false;

/** True once per press (rising edge) of the touch panel, with its position. */
bool modalPress(int32_t* x, int32_t* y) {
  const bool down = tft.getTouch(x, y);
  const bool pressed = down && !s_modal_touch_down;
  s_modal_touch_down = down;
  return pressed;
}

void connectingPoll() {
  const wf::ConnectState state = wf::connectPoll();
  if (state == wf::ConnectState::kConnected) {
    showResult(true);
    return;
  }
  if (state == wf::ConnectState::kFailed) {
    showResult(false);
    return;
  }
  if (millis() - s_anim_ms >= 400) {
    s_anim_ms = millis();
    ++s_anim;
    drawConnectingDots();
  }
  int32_t x = 0;
  int32_t y = 0;
  if (modalPress(&x, &y) && x >= 170 && x < 310 && y >= 236 && y < 284) {
    wf::connectCancel();
    returnToTab();
  }
}

void resultPoll() {
  int32_t x = 0;
  int32_t y = 0;
  const bool pressed = modalPress(&x, &y);
  if (s_result_ok) {
    if (pressed || millis() - s_result_ms >= 1800) {
      returnToTab();
      s_scanned = false;  // the list is worth refreshing after a network change
    }
    return;
  }
  if (!pressed || y < 236 || y >= 284) {
    return;
  }
  if (x >= 60 && x < 230) {
    openPasswordKeyboard();  // try again
  } else if (x >= 250 && x < 420) {
    returnToTab();
  }
}

void activateTab(const TabTarget& target) {
  switch (target.kind) {
    case TabKind::kScan:
      beginScan();
      drawTab();
      break;
    case TabKind::kNetwork: {
      const wf::Network& net = s_nets[target.index];
      snprintf(s_ssid, sizeof(s_ssid), "%s", net.ssid);
      s_ssid_secured = net.secured;
      s_password[0] = '\0';
      if (net.secured) {
        openPasswordKeyboard();
      } else {
        startConnect();
      }
      break;
    }
    case TabKind::kOther:
      keyboardOpen("Network name", "", wf::kSsidMax, false);
      s_mode = Mode::kSsidKeyboard;
      break;
    case TabKind::kNone:
      break;
  }
}

}  // namespace

// ---- Public API ------------------------------------------------------------------

void wifiTabDraw(int top) {
  s_top = top;
  s_tab_down = false;
  s_tab_pressed = TabTarget{};
  if (!s_scanned && !s_scanning) {
    beginScan();
  }
  drawTab();
}

void wifiTabTouch(bool down, int x, int y) {
  if (s_mode != Mode::kTab) {
    return;
  }
  tick();

  if (down) {
    const TabTarget hit = hitTab(x, y);
    if (!s_tab_down) {
      s_tab_down = true;
      s_tab_pressed = hit;
      if (hit.kind != TabKind::kNone) {
        drawTabRow(rowOf(hit), true);
      }
    } else if (!(hit == s_tab_pressed) && s_tab_pressed.kind != TabKind::kNone) {
      const TabTarget previous = s_tab_pressed;
      s_tab_pressed = TabTarget{};  // finger slid off: cancel
      drawTabRow(rowOf(previous), false);
    }
    return;
  }
  if (!s_tab_down) {
    return;
  }
  s_tab_down = false;
  const TabTarget released = s_tab_pressed;
  s_tab_pressed = TabTarget{};
  if (released.kind == TabKind::kNone) {
    return;
  }
  s_modal_touch_down = true;  // the release that got us here is not a new press
  activateTab(released);
}

bool wifiModalActive() { return s_mode != Mode::kTab; }

void wifiModalPoll() {
  switch (s_mode) {
    case Mode::kTab:
      break;
    case Mode::kSsidKeyboard: {
      const InputResult result = keyboardPoll();
      if (result == InputResult::kAccepted && keyboardText()[0] != '\0') {
        snprintf(s_ssid, sizeof(s_ssid), "%s", keyboardText());
        s_ssid_secured = true;
        s_password[0] = '\0';
        openPasswordKeyboard();
      } else if (result != InputResult::kOpen) {
        returnToTab();
      }
      break;
    }
    case Mode::kPasswordKeyboard: {
      const InputResult result = keyboardPoll();
      if (result == InputResult::kAccepted) {
        snprintf(s_password, sizeof(s_password), "%s", keyboardText());
        s_modal_touch_down = true;
        startConnect();
      } else if (result == InputResult::kCancelled) {
        returnToTab();
      }
      break;
    }
    case Mode::kConnecting:
      connectingPoll();
      break;
    case Mode::kResult:
      resultPoll();
      break;
  }
}

bool wifiBootSetup(bool* phone_requested, void (*idle)()) {
  *phone_requested = false;
  if (!radar::kSideBands) {
    *phone_requested = true;  // no touch screen: the phone portal is the only way
    return false;
  }

  s_boot = true;
  s_mode = Mode::kTab;
  s_scanned = false;
  s_scanning = false;
  s_net_count = 0;
  s_top = 47;
  const int footer_top = kScreenH - kBootFooterH;

  auto draw_screen = [&]() {
    tft.startWrite();
    tft.fillScreen(radar::kColorBackground);
    tft.fillRect(0, 0, kScreenW, kHeaderH, radar::kColorFooterBackground);
    tft.drawFastHLine(0, kHeaderH - 1, kScreenW, radar::kColorGrid);
    uiApplyFont(18);
    tft.setTextDatum(textdatum_t::middle_left);
    tft.setTextColor(radar::kColorLabel, radar::kColorFooterBackground);
    tft.drawString("WI-FI SETUP", kMargin, kHeaderH / 2);
    uiApplyFont(13);
    tft.setTextDatum(textdatum_t::middle_right);
    tft.setTextColor(colorDim(), radar::kColorFooterBackground);
    tft.drawString("Choose your network", kScreenW - kMargin, kHeaderH / 2);
    tft.setTextDatum(textdatum_t::middle_left);
    tft.setTextColor(colorDim(), radar::kColorBackground);
    tft.drawString("Or set it up from your phone:", kMargin, footer_top + kBootFooterH / 2);
    tft.setTextDatum(textdatum_t::top_left);
    uiDrawButton(kScreenW - kMargin - kPhoneButtonW, footer_top + 5, kPhoneButtonW, 30,
                 "PHONE SETUP", 13, false, false);
    tft.endWrite();
    wifiTabDraw(s_top);
  };
  draw_screen();

  bool phone_down = false;
  bool result = false;
  for (;;) {
    if (idle != nullptr) {
      idle();
    }
    if (s_mode == Mode::kTab) {
      int32_t x = 0;
      int32_t y = 0;
      const bool down = tft.getTouch(&x, &y);
      const bool in_footer = y >= footer_top;
      if (down && in_footer && x >= kScreenW - kMargin - kPhoneButtonW - 6) {
        phone_down = true;
      } else if (!down && phone_down) {
        *phone_requested = true;
        break;
      } else if (!down) {
        phone_down = false;
      }
      wifiTabTouch(down && y >= kHeaderH && !in_footer, x, y);
    } else {
      wifiModalPoll();
      if (s_mode == Mode::kTab) {
        if (wf::connected()) {
          result = true;
          break;
        }
        draw_screen();
      }
    }
    delay(10);
  }
  s_boot = false;
  return result;
}

}  // namespace ui
