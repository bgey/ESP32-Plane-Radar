#pragma once

#include <cstddef>

#include "services/adsb_client.h"

namespace sim {

/** Replace the aircraft list the UI reads (copied, clamped to kMaxAircraft). */
void setAircraft(const services::adsb::Aircraft* list, size_t count);

}  // namespace sim
