#include "services/radar_location.h"

#include <Preferences.h>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "config.h"

namespace services::location {

namespace {

constexpr char kPrefsNamespace[] = "radar";
constexpr char kKeyLat[] = "lat";
constexpr char kKeyLon[] = "lon";
constexpr char kKeyFavCount[] = "favCnt";
constexpr char kKeyFavData[] = "favs";

/** Two positions closer than this (degrees, about a metre) count as the same place. */
constexpr double kSamePlaceDeg = 1e-5;

double s_lat = config::kDefaultRadarLat;
double s_lon = config::kDefaultRadarLon;

Favourite s_favourites[kMaxFavourites];
size_t s_favourite_count = 0;

bool parseCoord(const char* text, double* out) {
  if (text == nullptr || text[0] == '\0') {
    return false;
  }
  char* end = nullptr;
  const double v = strtod(text, &end);
  if (end == text || (end != nullptr && *end != '\0')) {
    return false;
  }
  *out = v;
  return true;
}

void persist(double lat, double lon) {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.putDouble(kKeyLat, lat);
  prefs.putDouble(kKeyLon, lon);
  prefs.end();
  s_lat = lat;
  s_lon = lon;
}

void saveFavourites() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, false)) {
    return;
  }
  prefs.putUChar(kKeyFavCount, static_cast<uint8_t>(s_favourite_count));
  if (s_favourite_count > 0) {
    prefs.putBytes(kKeyFavData, s_favourites,
                   s_favourite_count * sizeof(Favourite));
  }
  prefs.end();
}

/** Copy a name, trimming surrounding spaces and truncating to fit. */
void copyName(char* out, const char* name) {
  out[0] = '\0';
  if (name == nullptr) {
    return;
  }
  while (*name == ' ') {
    ++name;
  }
  size_t length = strnlen(name, kFavouriteNameLen - 1);
  while (length > 0 && name[length - 1] == ' ') {
    --length;
  }
  memcpy(out, name, length);
  out[length] = '\0';
}

void loadFavourites() {
  s_favourite_count = 0;
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, true)) {
    return;
  }
  const size_t stored = prefs.getUChar(kKeyFavCount, 0);
  if (stored > 0 && stored <= kMaxFavourites) {
    Favourite loaded[kMaxFavourites];
    const size_t bytes = stored * sizeof(Favourite);
    if (prefs.getBytes(kKeyFavData, loaded, bytes) == bytes) {
      for (size_t i = 0; i < stored; ++i) {
        loaded[i].name[kFavouriteNameLen - 1] = '\0';
        if (loaded[i].name[0] != '\0' &&
            validCoordinates(loaded[i].lat, loaded[i].lon)) {
          s_favourites[s_favourite_count++] = loaded[i];
        }
      }
    }
  }
  prefs.end();
}

}  // namespace

bool validCoordinates(double lat, double lon) {
  return lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
}

void init() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, true);
  if (prefs.isKey(kKeyLat) && prefs.isKey(kKeyLon)) {
    const double lat = prefs.getDouble(kKeyLat, config::kDefaultRadarLat);
    const double lon = prefs.getDouble(kKeyLon, config::kDefaultRadarLon);
    if (validCoordinates(lat, lon)) {
      s_lat = lat;
      s_lon = lon;
    }
  }
  prefs.end();
  loadFavourites();
}

double lat() { return s_lat; }

double lon() { return s_lon; }

bool saveFromStrings(const char* lat_str, const char* lon_str) {
  double lat = 0.0;
  double lon = 0.0;
  if (!parseCoord(lat_str, &lat) || !parseCoord(lon_str, &lon)) {
    return false;
  }
  return setPosition(lat, lon);
}

bool setPosition(double lat, double lon) {
  if (!validCoordinates(lat, lon)) {
    return false;
  }
  persist(lat, lon);
  Serial.printf("Radar location saved: %.6f, %.6f\n", lat, lon);
  return true;
}

void clear() {
  Preferences prefs;
  prefs.begin(kPrefsNamespace, false);
  prefs.remove(kKeyLat);
  prefs.remove(kKeyLon);
  prefs.remove(kKeyFavCount);
  prefs.remove(kKeyFavData);
  prefs.end();
  s_lat = config::kDefaultRadarLat;
  s_lon = config::kDefaultRadarLon;
  s_favourite_count = 0;
}

size_t favouriteCount() { return s_favourite_count; }

const Favourite& favourite(size_t index) { return s_favourites[index]; }

bool addFavourite(const char* name, double lat, double lon) {
  if (s_favourite_count >= kMaxFavourites || !validCoordinates(lat, lon)) {
    return false;
  }
  Favourite entry = {};
  copyName(entry.name, name);
  if (entry.name[0] == '\0') {
    return false;
  }
  entry.lat = lat;
  entry.lon = lon;
  s_favourites[s_favourite_count++] = entry;
  saveFavourites();
  return true;
}

bool updateFavourite(size_t index, const char* name, double lat, double lon) {
  if (index >= s_favourite_count || !validCoordinates(lat, lon)) {
    return false;
  }
  Favourite entry = {};
  copyName(entry.name, name);
  if (entry.name[0] == '\0') {
    return false;
  }
  entry.lat = lat;
  entry.lon = lon;
  s_favourites[index] = entry;
  saveFavourites();
  return true;
}

void removeFavourite(size_t index) {
  if (index >= s_favourite_count) {
    return;
  }
  for (size_t i = index; i + 1 < s_favourite_count; ++i) {
    s_favourites[i] = s_favourites[i + 1];
  }
  --s_favourite_count;
  saveFavourites();
}

int currentFavouriteIndex() {
  for (size_t i = 0; i < s_favourite_count; ++i) {
    if (std::fabs(s_favourites[i].lat - s_lat) < kSamePlaceDeg &&
        std::fabs(s_favourites[i].lon - s_lon) < kSamePlaceDeg) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace services::location
