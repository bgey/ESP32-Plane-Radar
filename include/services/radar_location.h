#pragma once

#include <cstddef>

namespace services::location {

/** Load saved lat/lon and favourites from NVS, or use config defaults. Call once before WiFi setup. */
void init();

/** Current radar position (factory default when nothing is stored; also used for portal prefill). */
double lat();
double lon();

/** Parse portal strings, validate, persist to NVS, update runtime values. */
bool saveFromStrings(const char* lat_str, const char* lon_str);

/** Latitude within [-90, 90] and longitude within [-180, 180]. */
bool validCoordinates(double lat, double lon);

/** Make (lat, lon) the radar position and persist it. False when out of range. */
bool setPosition(double lat, double lon);

/** Clear stored coordinates and favourites (e.g. with the WiFi credential reset). */
void clear();

// ---- Favourite places ------------------------------------------------------

constexpr size_t kMaxFavourites = 6;
/** Including the terminating NUL. */
constexpr size_t kFavouriteNameLen = 16;

struct Favourite {
  char name[kFavouriteNameLen];
  double lat;
  double lon;
};

size_t favouriteCount();
const Favourite& favourite(size_t index);

/** False when the list is full, the name is empty or the coordinates are invalid. */
bool addFavourite(const char* name, double lat, double lon);
bool updateFavourite(size_t index, const char* name, double lat, double lon);
void removeFavourite(size_t index);

/** Index of the favourite at the current radar position, or -1. */
int currentFavouriteIndex();

}  // namespace services::location
