#include "ui/text_input.h"

#include <lgfx/v1/lgfx_fonts.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "config.h"
#include "hardware/display.h"
#include "hardware/display_font.h"
#include "ui/radar_theme.h"

namespace lgfx_fonts = lgfx::v1::fonts;

namespace ui {
namespace {

constexpr int kScreenW = config::kDisplayWidth;
constexpr int kScreenH = config::kDisplayHeight;

// ---- Shared drawing ---------------------------------------------------------

enum class FontSize { kSmall = 0, kMedium = 1, kLarge = 2 };
constexpr int kFontHeights[] = {13, 18, 24};
float s_font_sizes[3] = {0.35f, 0.5f, 0.65f};
bool s_font_ready = false;

void applyFont(FontSize size) {
  displayFontEnsureLoaded(tft);
  if (displayFontIsSmooth()) {
    if (!s_font_ready) {
      for (int i = 0; i < 3; ++i) {
        s_font_sizes[i] = displayFontSmoothSizeForHeight(tft, kFontHeights[i]);
      }
      s_font_ready = true;
    }
    displayFontSetSmoothSize(tft, s_font_sizes[static_cast<int>(size)]);
  } else {
    displayFontSetBitmap(tft, size == FontSize::kLarge ? &lgfx_fonts::FreeSansBold12pt7b
                                                       : &lgfx_fonts::FreeSansBold9pt7b);
  }
}

struct Key {
  char ch;            // character to insert, or 0 for a function key
  char label[8];
  int x, y, w, h;
  enum Kind { kChar, kBackspace, kClear, kOk, kCancel, kShift, kSpace, kLayer } kind;
};

void drawKey(const Key& key, bool pressed, bool highlighted, const char* label,
             FontSize font) {
  const uint16_t fill = pressed ? radar::kColorGrid
                                : (highlighted ? radar::kColorGrid
                                               : radar::kColorFooterBackground);
  tft.fillRoundRect(key.x, key.y, key.w, key.h, 6, fill);
  tft.drawRoundRect(key.x, key.y, key.w, key.h, 6, radar::kColorGrid);
  applyFont(font);
  tft.setTextDatum(textdatum_t::middle_center);
  tft.setTextColor(radar::kColorLabel, fill);
  tft.drawString(label, key.x + key.w / 2, key.y + key.h / 2);
  tft.setTextDatum(textdatum_t::top_left);
}

int hitKey(const Key* keys, size_t count, int x, int y) {
  for (size_t i = 0; i < count; ++i) {
    if (x >= keys[i].x && x < keys[i].x + keys[i].w && y >= keys[i].y &&
        y < keys[i].y + keys[i].h) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

/**
 * Titled text box. Long text shows its tail (the end being typed); reserve_right
 * keeps that many pixels at the right edge free for a button.
 */
void drawTextField(const char* title, const char* text, int x, int y, int w, int h,
                   const char* hint, uint16_t hint_color, int reserve_right = 0) {
  applyFont(FontSize::kSmall);
  tft.setTextDatum(textdatum_t::top_left);
  tft.setTextColor(radar::kColorTagType, radar::kColorBackground);
  tft.fillRect(x, y - 20, w, 18, radar::kColorBackground);
  tft.drawString(title, x, y - 19);
  if (hint != nullptr && hint[0] != '\0') {
    tft.setTextDatum(textdatum_t::top_right);
    tft.setTextColor(hint_color, radar::kColorBackground);
    tft.drawString(hint, x + w, y - 19);
    tft.setTextDatum(textdatum_t::top_left);
  }
  tft.fillRoundRect(x, y, w, h, 6, radar::kColorFooterBackground);
  tft.drawRoundRect(x, y, w, h, 6, radar::kColorGrid);
  applyFont(FontSize::kLarge);
  tft.setTextDatum(textdatum_t::middle_left);
  tft.setTextColor(radar::kColorTagAltitude, radar::kColorFooterBackground);
  char shown[80];
  snprintf(shown, sizeof(shown), "%s_", text);
  const char* start = shown;
  while (start[0] != '\0' && start[1] != '\0' &&
         tft.textWidth(start) > w - 20 - reserve_right) {
    ++start;  // scroll: drop characters from the left until the tail fits
  }
  tft.drawString(start, x + 10, y + h / 2);
  tft.setTextDatum(textdatum_t::top_left);
}

// ---- Number pad -------------------------------------------------------------

constexpr size_t kPadMaxText = 11;

Key s_pad_keys[16];
size_t s_pad_key_count = 0;
bool s_pad_open = false;
char s_pad_text[16] = {};
char s_pad_title[24] = {};
char s_pad_error[32] = {};
double s_pad_min = 0.0;
double s_pad_max = 0.0;
bool s_pad_down = false;
int s_pad_pressed = -1;

void buildPadKeys() {
  s_pad_key_count = 0;
  static const char kGrid[] = "789456123-0.";
  constexpr int kKeyW = 92;
  constexpr int kKeyH = 54;
  constexpr int kOriginX = 12;
  constexpr int kOriginY = 76;
  for (int i = 0; i < 12; ++i) {
    Key& key = s_pad_keys[s_pad_key_count++];
    key.ch = kGrid[i];
    key.label[0] = kGrid[i];
    key.label[1] = '\0';
    key.x = kOriginX + (i % 3) * (kKeyW + 4);
    key.y = kOriginY + (i / 3) * (kKeyH + 4);
    key.w = kKeyW;
    key.h = kKeyH;
    key.kind = Key::kChar;
  }
  struct Side {
    const char* label;
    Key::Kind kind;
  };
  static const Side kSide[] = {{"DEL", Key::kBackspace},
                               {"CLEAR", Key::kClear},
                               {"OK", Key::kOk},
                               {"CANCEL", Key::kCancel}};
  for (int i = 0; i < 4; ++i) {
    Key& key = s_pad_keys[s_pad_key_count++];
    key.ch = 0;
    snprintf(key.label, sizeof(key.label), "%s", kSide[i].label);
    key.x = 324;
    key.y = kOriginY + i * (kKeyH + 4);
    key.w = 144;
    key.h = kKeyH;
    key.kind = kSide[i].kind;
  }
}

void drawPadValue() {
  tft.startWrite();
  drawTextField(s_pad_title, s_pad_text, 12, 30, kScreenW - 24, 34, s_pad_error,
                tft.color565(255, 130, 80));
  tft.endWrite();
}

void drawPadKey(int index) {
  const Key& key = s_pad_keys[index];
  const bool ok_key = key.kind == Key::kOk;
  tft.startWrite();
  drawKey(key, s_pad_down && s_pad_pressed == index, ok_key, key.label,
          key.kind == Key::kChar ? FontSize::kLarge : FontSize::kMedium);
  tft.endWrite();
}

void drawPadAll() {
  tft.startWrite();
  tft.fillScreen(radar::kColorBackground);
  drawPadValue();
  for (size_t i = 0; i < s_pad_key_count; ++i) {
    drawPadKey(static_cast<int>(i));
  }
  tft.endWrite();
}

bool padTextValid(double* value) {
  if (s_pad_text[0] == '\0') {
    return false;
  }
  char* end = nullptr;
  const double v = strtod(s_pad_text, &end);
  if (end == s_pad_text || *end != '\0') {
    return false;
  }
  if (v < s_pad_min || v > s_pad_max) {
    return false;
  }
  *value = v;
  return true;
}

void padInsertChar(char ch) {
  const size_t length = strlen(s_pad_text);
  if (ch == '-') {
    if (s_pad_text[0] == '-') {
      memmove(s_pad_text, s_pad_text + 1, length);  // remove the sign
    } else if (length < kPadMaxText) {
      memmove(s_pad_text + 1, s_pad_text, length + 1);
      s_pad_text[0] = '-';
    }
    return;
  }
  if (length >= kPadMaxText) {
    return;
  }
  if (ch == '.') {
    if (strchr(s_pad_text, '.') != nullptr) {
      return;
    }
    if (length == 0 || (length == 1 && s_pad_text[0] == '-')) {
      s_pad_text[length] = '0';
      s_pad_text[length + 1] = '\0';
      if (length + 1 >= kPadMaxText) {
        return;
      }
    }
  }
  const size_t end = strlen(s_pad_text);
  s_pad_text[end] = ch;
  s_pad_text[end + 1] = '\0';
}

// Returns true when the pad closed with a result.
bool padActivate(int index, InputResult* result) {
  const Key& key = s_pad_keys[index];
  switch (key.kind) {
    case Key::kChar:
      padInsertChar(key.ch);
      s_pad_error[0] = '\0';
      drawPadValue();
      return false;
    case Key::kBackspace: {
      const size_t length = strlen(s_pad_text);
      if (length > 0) {
        s_pad_text[length - 1] = '\0';
      }
      s_pad_error[0] = '\0';
      drawPadValue();
      return false;
    }
    case Key::kClear:
      s_pad_text[0] = '\0';
      s_pad_error[0] = '\0';
      drawPadValue();
      return false;
    case Key::kOk: {
      double value = 0.0;
      if (padTextValid(&value)) {
        s_pad_open = false;
        *result = InputResult::kAccepted;
        return true;
      }
      snprintf(s_pad_error, sizeof(s_pad_error), "Enter %g to %g", s_pad_min, s_pad_max);
      drawPadValue();
      return false;
    }
    case Key::kCancel:
      s_pad_open = false;
      *result = InputResult::kCancelled;
      return true;
    default:
      return false;
  }
}

// ---- Keyboard -----------------------------------------------------------------

constexpr size_t kKbMaxText = 64;
constexpr int kShowButtonW = 76;
constexpr int kShowIndex = -2;  // pseudo key index for the SHOW/HIDE button

Key s_kb_keys[56];
size_t s_kb_key_count = 0;
bool s_kb_open = false;
char s_kb_text[kKbMaxText + 1] = {};
char s_kb_title[48] = {};
size_t s_kb_max_len = kKbMaxText;
bool s_kb_shift = true;
bool s_kb_symbols = false;
bool s_kb_secret = false;
bool s_kb_reveal = false;
bool s_kb_down = false;
int s_kb_pressed = -1;

constexpr int kKbUnit = 46;  // one full key including its gap
constexpr int kKbLeft = 10;

/** Append a key of `half_units` half-key widths at *x, then advance *x. */
void addKbKey(int* x, int y, char ch, const char* label, Key::Kind kind, int half_units) {
  Key& key = s_kb_keys[s_kb_key_count++];
  key.ch = ch;
  snprintf(key.label, sizeof(key.label), "%s", label);
  key.x = *x;
  key.y = y;
  key.w = half_units * kKbUnit / 2 - 2;
  key.h = 47;
  key.kind = kind;
  *x += half_units * kKbUnit / 2;
}

void addKbChars(int y, const char* chars, int start_half_units) {
  int x = kKbLeft + start_half_units * kKbUnit / 2;
  for (const char* p = chars; *p != '\0'; ++p) {
    const char label[2] = {*p, '\0'};
    addKbKey(&x, y, *p, label, Key::kChar, 2);
  }
}

void buildKeyboardKeys() {
  s_kb_key_count = 0;
  int y = 58;
  addKbChars(y, "1234567890", 0);
  y += 52;
  int x = kKbLeft;
  if (!s_kb_symbols) {
    addKbChars(y, "QWERTYUIOP", 0);
    y += 52;
    addKbChars(y, "ASDFGHJKL", 1);
    y += 52;
    addKbKey(&x, y, 0, "SHIFT", Key::kShift, 3);
    for (const char* p = "ZXCVBNM"; *p != '\0'; ++p) {
      const char label[2] = {*p, '\0'};
      addKbKey(&x, y, *p, label, Key::kChar, 2);
    }
    addKbKey(&x, y, 0, "DEL", Key::kBackspace, 3);
    y += 52;
    x = kKbLeft;
    addKbKey(&x, y, 0, "CANCEL", Key::kCancel, 4);
    addKbKey(&x, y, 0, "#+=", Key::kLayer, 3);
    addKbKey(&x, y, '-', "-", Key::kChar, 2);
    addKbKey(&x, y, ' ', "SPACE", Key::kSpace, 5);
    addKbKey(&x, y, '.', ".", Key::kChar, 2);
    addKbKey(&x, y, 0, "OK", Key::kOk, 4);
  } else {
    addKbChars(y, "!@#$%^&*()", 0);
    y += 52;
    addKbChars(y, "-_=+[]{}\\|", 0);
    y += 52;
    addKbChars(y, ";:'\",.<>/?", 0);
    y += 52;
    addKbKey(&x, y, 0, "CANCEL", Key::kCancel, 3);
    addKbKey(&x, y, 0, "ABC", Key::kLayer, 3);
    addKbKey(&x, y, '~', "~", Key::kChar, 2);
    addKbKey(&x, y, '`', "`", Key::kChar, 2);
    addKbKey(&x, y, ' ', "SPACE", Key::kSpace, 4);
    addKbKey(&x, y, 0, "DEL", Key::kBackspace, 3);
    addKbKey(&x, y, 0, "OK", Key::kOk, 3);
  }
}

char keyChar(const Key& key) {
  if (key.kind == Key::kChar && key.ch >= 'A' && key.ch <= 'Z') {
    return s_kb_shift ? key.ch : static_cast<char>(key.ch - 'A' + 'a');
  }
  return key.ch;
}

int showButtonX() { return kScreenW - 10 - kShowButtonW - 4; }

bool inShowButton(int x, int y) {
  return s_kb_secret && x >= showButtonX() && x < showButtonX() + kShowButtonW &&
         y >= 28 && y < 52;
}

void drawShowButton(bool pressed) {
  const uint16_t fill = pressed ? radar::kColorGrid : radar::kColorBackground;
  tft.fillRoundRect(showButtonX(), 28, kShowButtonW, 24, 5, fill);
  tft.drawRoundRect(showButtonX(), 28, kShowButtonW, 24, 5, radar::kColorGrid);
  applyFont(FontSize::kSmall);
  tft.setTextDatum(textdatum_t::middle_center);
  tft.setTextColor(radar::kColorLabel, fill);
  tft.drawString(s_kb_reveal ? "HIDE" : "SHOW", showButtonX() + kShowButtonW / 2, 40);
  tft.setTextDatum(textdatum_t::top_left);
}

void drawKbField() {
  char masked[kKbMaxText + 1];
  const char* shown = s_kb_text;
  if (s_kb_secret && !s_kb_reveal) {
    const size_t length = strlen(s_kb_text);
    memset(masked, '*', length);
    masked[length] = '\0';
    shown = masked;
  }
  tft.startWrite();
  drawTextField(s_kb_title, shown, 10, 26, kScreenW - 20, 28, "", 0,
                s_kb_secret ? kShowButtonW + 8 : 0);
  if (s_kb_secret) {
    drawShowButton(false);
  }
  tft.endWrite();
}

void drawKbKey(int index) {
  const Key& key = s_kb_keys[index];
  char label[8];
  if (key.kind == Key::kChar) {
    snprintf(label, sizeof(label), "%c", keyChar(key));
  } else {
    snprintf(label, sizeof(label), "%s", key.label);
  }
  const bool highlighted = (key.kind == Key::kShift && s_kb_shift) || key.kind == Key::kOk;
  tft.startWrite();
  drawKey(key, s_kb_down && s_kb_pressed == index, highlighted, label,
          key.kind == Key::kChar ? FontSize::kMedium : FontSize::kSmall);
  tft.endWrite();
}

void drawKbAll() {
  tft.startWrite();
  tft.fillScreen(radar::kColorBackground);
  drawKbField();
  for (size_t i = 0; i < s_kb_key_count; ++i) {
    drawKbKey(static_cast<int>(i));
  }
  tft.endWrite();
}

void redrawKbLetters() {
  tft.startWrite();
  for (size_t i = 0; i < s_kb_key_count; ++i) {
    if (s_kb_keys[i].kind == Key::kChar || s_kb_keys[i].kind == Key::kShift) {
      drawKbKey(static_cast<int>(i));
    }
  }
  tft.endWrite();
}

bool kbActivate(int index, InputResult* result) {
  const Key& key = s_kb_keys[index];
  const size_t length = strlen(s_kb_text);
  switch (key.kind) {
    case Key::kChar:
    case Key::kSpace: {
      if (length < s_kb_max_len) {
        s_kb_text[length] = keyChar(key);
        s_kb_text[length + 1] = '\0';
      }
      const bool was_shift = s_kb_shift;
      s_kb_shift = false;  // shift is one-shot
      drawKbField();
      if (was_shift) {
        redrawKbLetters();
      }
      return false;
    }
    case Key::kBackspace:
      if (length > 0) {
        s_kb_text[length - 1] = '\0';
      }
      drawKbField();
      return false;
    case Key::kShift:
      s_kb_shift = !s_kb_shift;
      redrawKbLetters();
      return false;
    case Key::kLayer:
      s_kb_symbols = !s_kb_symbols;
      buildKeyboardKeys();
      drawKbAll();
      return false;
    case Key::kOk:
      s_kb_open = false;
      *result = InputResult::kAccepted;
      return true;
    case Key::kCancel:
      s_kb_open = false;
      *result = InputResult::kCancelled;
      return true;
    default:
      return false;
  }
}

}  // namespace

// ---- Public API ---------------------------------------------------------------

void numberPadOpen(const char* title, const char* initial_text, double min, double max) {
  snprintf(s_pad_title, sizeof(s_pad_title), "%s", title);
  snprintf(s_pad_text, sizeof(s_pad_text), "%s", initial_text != nullptr ? initial_text : "");
  s_pad_error[0] = '\0';
  s_pad_min = min;
  s_pad_max = max;
  s_pad_down = false;
  s_pad_pressed = -1;
  buildPadKeys();
  s_pad_open = true;
  drawPadAll();
}

InputResult numberPadPoll() {
  if (!s_pad_open) {
    return InputResult::kCancelled;
  }
  int32_t x = 0;
  int32_t y = 0;
  const bool down = tft.getTouch(&x, &y);
  InputResult result = InputResult::kOpen;

  if (down) {
    const int hit = hitKey(s_pad_keys, s_pad_key_count, x, y);
    if (!s_pad_down) {
      s_pad_down = true;
      s_pad_pressed = hit;
      if (hit >= 0) {
        drawPadKey(hit);
      }
    } else if (hit != s_pad_pressed && s_pad_pressed >= 0) {
      const int previous = s_pad_pressed;
      s_pad_pressed = -1;  // finger slid off: cancel
      drawPadKey(previous);
    }
  } else if (s_pad_down) {
    s_pad_down = false;
    const int released = s_pad_pressed;
    s_pad_pressed = -1;
    if (released >= 0) {
      drawPadKey(released);
      padActivate(released, &result);
    }
  }
  return result;
}

const char* numberPadText() { return s_pad_text; }

void keyboardOpen(const char* title, const char* initial_text, size_t max_len, bool secret) {
  snprintf(s_kb_title, sizeof(s_kb_title), "%s", title);
  s_kb_max_len = max_len > kKbMaxText ? kKbMaxText : max_len;
  snprintf(s_kb_text, sizeof(s_kb_text), "%.*s", static_cast<int>(s_kb_max_len),
           initial_text != nullptr ? initial_text : "");
  s_kb_secret = secret;
  s_kb_reveal = false;
  s_kb_symbols = false;
  // Passwords are case sensitive, so no auto-capitalisation for them.
  s_kb_shift = !secret && s_kb_text[0] == '\0';
  s_kb_down = false;
  s_kb_pressed = -1;
  buildKeyboardKeys();
  s_kb_open = true;
  drawKbAll();
}

InputResult keyboardPoll() {
  if (!s_kb_open) {
    return InputResult::kCancelled;
  }
  int32_t x = 0;
  int32_t y = 0;
  const bool down = tft.getTouch(&x, &y);
  InputResult result = InputResult::kOpen;

  if (down) {
    const int hit = inShowButton(x, y) ? kShowIndex : hitKey(s_kb_keys, s_kb_key_count, x, y);
    if (!s_kb_down) {
      s_kb_down = true;
      s_kb_pressed = hit;
      if (hit >= 0) {
        drawKbKey(hit);
      } else if (hit == kShowIndex) {
        tft.startWrite();
        drawShowButton(true);
        tft.endWrite();
      }
    } else if (hit != s_kb_pressed && s_kb_pressed != -1) {
      const int previous = s_kb_pressed;
      s_kb_pressed = -1;  // finger slid off: cancel
      if (previous >= 0) {
        drawKbKey(previous);
      } else {
        tft.startWrite();
        drawShowButton(false);
        tft.endWrite();
      }
    }
  } else if (s_kb_down) {
    s_kb_down = false;
    const int released = s_kb_pressed;
    s_kb_pressed = -1;
    if (released == kShowIndex) {
      s_kb_reveal = !s_kb_reveal;
      drawKbField();
    } else if (released >= 0) {
      drawKbKey(released);
      kbActivate(released, &result);
    }
  }
  return result;
}

const char* keyboardText() { return s_kb_text; }

}  // namespace ui
