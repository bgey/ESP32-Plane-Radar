#include "ui/location_settings.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include "config.h"
#include "hardware/display.h"
#include "services/radar_location.h"
#include "ui/radar_theme.h"
#include "ui/text_input.h"
#include "ui/ui_font.h"

namespace ui {
namespace {

namespace loc = services::location;

constexpr int kScreenW = config::kDisplayWidth;
constexpr int kRowH = 34;
constexpr int kMargin = 12;
constexpr int kButtonW = 58;
constexpr int kButtonH = 26;
constexpr int kHeaderH = 44;

enum class Mode { kTab, kEditor, kPad, kKeyboard };
enum class Field { kName, kLat, kLon };

Mode s_mode = Mode::kTab;
bool s_changed = false;
int s_top = 47;

struct Draft {
  char name[loc::kFavouriteNameLen];
  char lat[16];
  char lon[16];
  int index;  // favourite being edited, or -1 for a new place
};
Draft s_draft = {};
Field s_field = Field::kName;
char s_message[40] = {};
bool s_message_good = false;

uint16_t colorGood() { return radar::kColorGood; }
uint16_t colorBad() { return radar::kColorBad; }
uint16_t colorDim() { return radar::kColorDim; }

// ---- Coordinate text ----------------------------------------------------------

void formatCoord(double value, char* out, size_t n) {
  snprintf(out, n, "%.6f", value);
  size_t length = strlen(out);
  while (length > 0 && out[length - 1] == '0') {
    out[--length] = '\0';
  }
  if (length > 0 && out[length - 1] == '.') {
    out[--length] = '\0';
  }
}

bool parseCoord(const char* text, double lo, double hi, double* out) {
  if (text[0] == '\0') {
    return false;
  }
  char* end = nullptr;
  const double v = strtod(text, &end);
  if (end == text || *end != '\0' || v < lo || v > hi) {
    return false;
  }
  *out = v;
  return true;
}

// Buttons come from the shared helper.
void drawButton(int x, int y, int w, int h, const char* label, int font_px,
                bool pressed, bool accent) {
  uiDrawButton(x, y, w, h, label, font_px, pressed, accent);
}

// ---- Tab (list of places) -----------------------------------------------------

enum class TabKind { kNone, kSaveCurrent, kUse, kEdit, kNew };

struct TabTarget {
  TabKind kind = TabKind::kNone;
  int index = 0;
  bool operator==(const TabTarget& o) const {
    return kind == o.kind && (kind == TabKind::kNone || index == o.index);
  }
};

bool s_tab_down = false;
TabTarget s_tab_pressed;

int rowCount() { return 1 + static_cast<int>(loc::favouriteCount()) + 1; }
int rowTop(int row) { return s_top + row * kRowH; }
int buttonX() { return kScreenW - kMargin - kButtonW; }
int coordsRightX() { return buttonX() - 8; }

bool currentIsSaved() { return loc::currentFavouriteIndex() >= 0; }

TabTarget hitTab(int x, int y) {
  TabTarget target;
  if (y < s_top) {
    return target;
  }
  const int row = (y - s_top) / kRowH;
  if (row >= rowCount()) {
    return target;
  }
  const int favourites = static_cast<int>(loc::favouriteCount());
  if (row == 0) {
    if (!currentIsSaved() && favourites < static_cast<int>(loc::kMaxFavourites) &&
        x >= buttonX() - 6) {
      target.kind = TabKind::kSaveCurrent;
    }
  } else if (row <= favourites) {
    target.index = row - 1;
    target.kind = x >= buttonX() - 6 ? TabKind::kEdit : TabKind::kUse;
  } else {
    target.kind = TabKind::kNew;
  }
  return target;
}

int rowOf(const TabTarget& target) {
  switch (target.kind) {
    case TabKind::kSaveCurrent:
      return 0;
    case TabKind::kUse:
    case TabKind::kEdit:
      return target.index + 1;
    case TabKind::kNew:
      return 1 + static_cast<int>(loc::favouriteCount());
    default:
      return -1;
  }
}

void drawTabRow(int row, bool pressed) {
  const int top = rowTop(row);
  const int cy = top + kRowH / 2;
  const int favourites = static_cast<int>(loc::favouriteCount());
  const uint16_t bg = pressed ? radar::kColorFooterBackground : radar::kColorBackground;

  tft.startWrite();
  tft.fillRect(0, top, kScreenW, kRowH, bg);
  tft.drawFastHLine(kMargin, top + kRowH - 1, kScreenW - 2 * kMargin, radar::kColorGrid);
  tft.setTextDatum(textdatum_t::middle_left);

  char coords[32];
  if (row == 0) {
    const int saved = loc::currentFavouriteIndex();
    uiApplyFont(16);
    tft.setTextColor(radar::kColorLabel, bg);
    tft.drawString(saved >= 0 ? loc::favourite(saved).name : "Current position",
                   kMargin, cy);
    char lat[16];
    char lon[16];
    formatCoord(loc::lat(), lat, sizeof(lat));
    formatCoord(loc::lon(), lon, sizeof(lon));
    snprintf(coords, sizeof(coords), "%s, %s", lat, lon);
    uiApplyFont(13);
    tft.setTextDatum(textdatum_t::middle_right);
    tft.setTextColor(radar::kColorTagAltitude, bg);
    tft.drawString(coords, coordsRightX(), cy);
    if (saved < 0 && favourites < static_cast<int>(loc::kMaxFavourites)) {
      drawButton(buttonX(), cy - kButtonH / 2, kButtonW, kButtonH, "SAVE", 13, false,
                 false);
    } else if (saved >= 0) {
      uiApplyFont(13);
      tft.setTextDatum(textdatum_t::middle_right);
      tft.setTextColor(colorGood(), bg);
      tft.drawString("SAVED", kScreenW - kMargin, cy);
    }
  } else if (row <= favourites) {
    const loc::Favourite& place = loc::favourite(row - 1);
    const bool current = loc::currentFavouriteIndex() == row - 1;
    if (current) {
      tft.fillCircle(kMargin + 6, cy, 6, colorGood());
    } else {
      tft.drawCircle(kMargin + 6, cy, 6, radar::kColorGrid);
    }
    uiApplyFont(16);
    tft.setTextDatum(textdatum_t::middle_left);
    tft.setTextColor(radar::kColorLabel, bg);
    tft.drawString(place.name, kMargin + 22, cy);
    char lat[16];
    char lon[16];
    formatCoord(place.lat, lat, sizeof(lat));
    formatCoord(place.lon, lon, sizeof(lon));
    snprintf(coords, sizeof(coords), "%s, %s", lat, lon);
    uiApplyFont(13);
    tft.setTextDatum(textdatum_t::middle_right);
    tft.setTextColor(colorDim(), bg);
    tft.drawString(coords, coordsRightX(), cy);
    drawButton(buttonX(), cy - kButtonH / 2, kButtonW, kButtonH, "EDIT", 13, false,
               false);
  } else {
    uiApplyFont(16);
    tft.setTextDatum(textdatum_t::middle_left);
    tft.setTextColor(radar::kColorTagType, bg);
    tft.drawString("+  Enter new position", kMargin, cy);
  }
  tft.setTextDatum(textdatum_t::top_left);
  tft.endWrite();
}

void drawTab() {
  tft.startWrite();
  tft.fillRect(0, s_top, kScreenW, config::kDisplayHeight - s_top,
               radar::kColorBackground);
  for (int row = 0; row < rowCount(); ++row) {
    drawTabRow(row, false);
  }
  const int hint_y = rowTop(rowCount()) + 6;
  if (hint_y + 14 < config::kDisplayHeight) {
    uiApplyFont(13);
    tft.setTextDatum(textdatum_t::top_left);
    tft.setTextColor(colorDim(), radar::kColorBackground);
    tft.drawString("Tap a place to use it. EDIT renames or deletes it.", kMargin, hint_y);
  }
  tft.endWrite();
}

// ---- Editor -------------------------------------------------------------------

enum class EditKind { kNone, kBack, kName, kLat, kLon, kUse, kSave, kDelete };

bool s_edit_down = false;
EditKind s_edit_pressed = EditKind::kNone;

constexpr int kFieldLabelX = kMargin;
constexpr int kFieldX = 124;
constexpr int kFieldW = 344;
constexpr int kFieldH = 40;
constexpr int kFieldTop = 56;
constexpr int kFieldPitch = 50;
constexpr int kButtonTop = 232;
constexpr int kActionH = 52;
constexpr int kActionW = 148;

int fieldY(EditKind kind) {
  return kFieldTop + (static_cast<int>(kind) - static_cast<int>(EditKind::kName)) * kFieldPitch;
}

int actionX(EditKind kind) {
  switch (kind) {
    case EditKind::kUse:
      return 12;
    case EditKind::kSave:
      return 166;
    default:
      return 320;
  }
}

void setMessage(const char* text, bool good) {
  snprintf(s_message, sizeof(s_message), "%s", text);
  s_message_good = good;
}

void drawEditorItem(EditKind kind, bool pressed) {
  tft.startWrite();
  switch (kind) {
    case EditKind::kBack:
      drawButton(kScreenW - 100, 5, 94, 34, "BACK", 16, pressed, false);
      break;
    case EditKind::kName:
    case EditKind::kLat:
    case EditKind::kLon: {
      const int y = fieldY(kind);
      const char* label = kind == EditKind::kName ? "Name"
                          : kind == EditKind::kLat ? "Latitude"
                                                   : "Longitude";
      const char* value = kind == EditKind::kName ? s_draft.name
                          : kind == EditKind::kLat ? s_draft.lat
                                                   : s_draft.lon;
      tft.fillRect(0, y - 4, kScreenW, kFieldH + 8, radar::kColorBackground);
      uiApplyFont(16);
      tft.setTextDatum(textdatum_t::middle_left);
      tft.setTextColor(radar::kColorLabel, radar::kColorBackground);
      tft.drawString(label, kFieldLabelX, y + kFieldH / 2);
      const uint16_t fill = pressed ? radar::kColorPressed : radar::kColorFooterBackground;
      tft.fillRoundRect(kFieldX, y, kFieldW, kFieldH, 6, fill);
      tft.drawRoundRect(kFieldX, y, kFieldW, kFieldH, 6, radar::kColorGrid);
      tft.setTextColor(value[0] != '\0' ? radar::kColorTagAltitude : colorDim(), fill);
      tft.drawString(value[0] != '\0' ? value : "tap to enter", kFieldX + 10,
                     y + kFieldH / 2);
      tft.setTextDatum(textdatum_t::top_left);
      break;
    }
    case EditKind::kUse:
      drawButton(actionX(kind), kButtonTop, kActionW, kActionH, "USE NOW", 16, pressed,
                 false);
      break;
    case EditKind::kSave:
      drawButton(actionX(kind), kButtonTop, kActionW, kActionH, "SAVE & USE", 16, pressed,
                 true);
      break;
    case EditKind::kDelete:
      if (s_draft.index >= 0) {
        drawButton(actionX(kind), kButtonTop, kActionW, kActionH, "DELETE", 16, pressed,
                   false);
      }
      break;
    default:
      break;
  }
  tft.endWrite();
}

void drawEditorMessage() {
  tft.startWrite();
  tft.fillRect(0, 202, kScreenW, 26, radar::kColorBackground);
  if (s_message[0] != '\0') {
    uiApplyFont(16);
    tft.setTextDatum(textdatum_t::middle_center);
    tft.setTextColor(s_message_good ? colorGood() : colorBad(), radar::kColorBackground);
    tft.drawString(s_message, kScreenW / 2, 215);
    tft.setTextDatum(textdatum_t::top_left);
  }
  tft.endWrite();
}

void drawEditor() {
  tft.startWrite();
  tft.fillScreen(radar::kColorBackground);
  tft.fillRect(0, 0, kScreenW, kHeaderH, radar::kColorFooterBackground);
  tft.drawFastHLine(0, kHeaderH - 1, kScreenW, radar::kColorGrid);
  uiApplyFont(18);
  tft.setTextDatum(textdatum_t::middle_left);
  tft.setTextColor(radar::kColorLabel, radar::kColorFooterBackground);
  tft.drawString(s_draft.index < 0 ? "NEW PLACE" : "EDIT PLACE", kMargin, kHeaderH / 2);
  tft.setTextDatum(textdatum_t::top_left);
  for (EditKind kind : {EditKind::kBack, EditKind::kName, EditKind::kLat, EditKind::kLon,
                        EditKind::kUse, EditKind::kSave, EditKind::kDelete}) {
    drawEditorItem(kind, false);
  }
  drawEditorMessage();
  tft.endWrite();
}

void openEditor(int index, const char* name, const char* lat, const char* lon) {
  snprintf(s_draft.name, sizeof(s_draft.name), "%s", name);
  snprintf(s_draft.lat, sizeof(s_draft.lat), "%s", lat);
  snprintf(s_draft.lon, sizeof(s_draft.lon), "%s", lon);
  s_draft.index = index;
  s_message[0] = '\0';
  s_edit_down = false;
  s_edit_pressed = EditKind::kNone;
  s_mode = Mode::kEditor;
  drawEditor();
}

EditKind hitEditor(int x, int y) {
  if (y < kHeaderH) {
    return x >= kScreenW - 100 ? EditKind::kBack : EditKind::kNone;
  }
  for (EditKind kind : {EditKind::kName, EditKind::kLat, EditKind::kLon}) {
    const int fy = fieldY(kind);
    if (y >= fy - 4 && y < fy + kFieldH + 4) {
      return kind;
    }
  }
  if (y >= kButtonTop && y < kButtonTop + kActionH) {
    for (EditKind kind : {EditKind::kUse, EditKind::kSave, EditKind::kDelete}) {
      if (x >= actionX(kind) && x < actionX(kind) + kActionW) {
        return (kind == EditKind::kDelete && s_draft.index < 0) ? EditKind::kNone : kind;
      }
    }
  }
  return EditKind::kNone;
}

/** Validate the coordinates; on success fill lat/lon, otherwise show why not. */
bool draftCoordinates(double* lat, double* lon) {
  if (s_draft.lat[0] == '\0' || s_draft.lon[0] == '\0') {
    setMessage("Enter latitude and longitude", false);
    return false;
  }
  if (!parseCoord(s_draft.lat, -90.0, 90.0, lat) ||
      !parseCoord(s_draft.lon, -180.0, 180.0, lon)) {
    setMessage("Coordinates out of range", false);
    return false;
  }
  return true;
}

void closeEditor() {
  s_mode = Mode::kTab;
  s_tab_down = false;
  s_tab_pressed = TabTarget{};
}

void editorActivate(EditKind kind) {
  double lat = 0.0;
  double lon = 0.0;
  switch (kind) {
    case EditKind::kBack:
      closeEditor();
      return;
    case EditKind::kName:
      s_field = Field::kName;
      s_mode = Mode::kKeyboard;
      keyboardOpen("Place name", s_draft.name, loc::kFavouriteNameLen - 1);
      return;
    case EditKind::kLat:
      s_field = Field::kLat;
      s_mode = Mode::kPad;
      numberPadOpen("Latitude (-90 to 90)", s_draft.lat, -90.0, 90.0);
      return;
    case EditKind::kLon:
      s_field = Field::kLon;
      s_mode = Mode::kPad;
      numberPadOpen("Longitude (-180 to 180)", s_draft.lon, -180.0, 180.0);
      return;
    case EditKind::kUse:
      if (!draftCoordinates(&lat, &lon)) {
        drawEditorMessage();
        return;
      }
      loc::setPosition(lat, lon);
      s_changed = true;
      closeEditor();
      return;
    case EditKind::kSave: {
      if (s_draft.name[0] == '\0') {
        setMessage("Enter a name first", false);
        drawEditorMessage();
        return;
      }
      if (!draftCoordinates(&lat, &lon)) {
        drawEditorMessage();
        return;
      }
      const bool saved = s_draft.index < 0
                             ? loc::addFavourite(s_draft.name, lat, lon)
                             : loc::updateFavourite(static_cast<size_t>(s_draft.index),
                                                    s_draft.name, lat, lon);
      if (!saved) {
        setMessage("List is full (6 places)", false);
        drawEditorMessage();
        return;
      }
      loc::setPosition(lat, lon);
      s_changed = true;
      closeEditor();
      return;
    }
    case EditKind::kDelete:
      if (s_draft.index >= 0) {
        loc::removeFavourite(static_cast<size_t>(s_draft.index));
      }
      closeEditor();
      return;
    default:
      return;
  }
}

void editorPoll() {
  int32_t x = 0;
  int32_t y = 0;
  const bool down = tft.getTouch(&x, &y);
  if (down) {
    const EditKind hit = hitEditor(x, y);
    if (!s_edit_down) {
      s_edit_down = true;
      s_edit_pressed = hit;
      if (hit != EditKind::kNone) {
        drawEditorItem(hit, true);
      }
    } else if (hit != s_edit_pressed && s_edit_pressed != EditKind::kNone) {
      const EditKind previous = s_edit_pressed;
      s_edit_pressed = EditKind::kNone;  // finger slid off: cancel
      drawEditorItem(previous, false);
    }
  } else if (s_edit_down) {
    s_edit_down = false;
    const EditKind released = s_edit_pressed;
    s_edit_pressed = EditKind::kNone;
    if (released != EditKind::kNone) {
      drawEditorItem(released, false);
      editorActivate(released);
    }
  }
}

}  // namespace

// ---- Public API ---------------------------------------------------------------

void locationTabDraw(int top) {
  s_top = top;
  s_tab_down = false;
  s_tab_pressed = TabTarget{};
  drawTab();
}

void locationTabTouch(bool down, int x, int y) {
  if (s_mode != Mode::kTab) {
    return;
  }
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
  char lat[16];
  char lon[16];
  switch (released.kind) {
    case TabKind::kUse: {
      const loc::Favourite& place = loc::favourite(static_cast<size_t>(released.index));
      loc::setPosition(place.lat, place.lon);
      s_changed = true;
      drawTab();
      break;
    }
    case TabKind::kEdit: {
      const loc::Favourite& place = loc::favourite(static_cast<size_t>(released.index));
      formatCoord(place.lat, lat, sizeof(lat));
      formatCoord(place.lon, lon, sizeof(lon));
      openEditor(released.index, place.name, lat, lon);
      break;
    }
    case TabKind::kSaveCurrent:
      formatCoord(loc::lat(), lat, sizeof(lat));
      formatCoord(loc::lon(), lon, sizeof(lon));
      openEditor(-1, "", lat, lon);
      break;
    case TabKind::kNew:
      openEditor(-1, "", "", "");
      break;
    case TabKind::kNone:
      break;
  }
}

bool locationModalActive() { return s_mode != Mode::kTab; }

void locationModalPoll() {
  switch (s_mode) {
    case Mode::kTab:
      break;
    case Mode::kEditor:
      editorPoll();
      break;
    case Mode::kPad: {
      const InputResult result = numberPadPoll();
      if (result == InputResult::kOpen) {
        break;
      }
      if (result == InputResult::kAccepted) {
        char text[16];
        formatCoord(strtod(numberPadText(), nullptr), text, sizeof(text));
        snprintf(s_field == Field::kLat ? s_draft.lat : s_draft.lon,
                 sizeof(s_draft.lat), "%s", text);
        s_message[0] = '\0';
      }
      s_mode = Mode::kEditor;
      drawEditor();
      break;
    }
    case Mode::kKeyboard: {
      const InputResult result = keyboardPoll();
      if (result == InputResult::kOpen) {
        break;
      }
      if (result == InputResult::kAccepted) {
        snprintf(s_draft.name, sizeof(s_draft.name), "%s", keyboardText());
        s_message[0] = '\0';
      }
      s_mode = Mode::kEditor;
      drawEditor();
      break;
    }
  }
}

bool locationTakeChanged() {
  const bool changed = s_changed;
  s_changed = false;
  return changed;
}

}  // namespace ui
