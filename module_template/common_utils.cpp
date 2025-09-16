/** common_utils.hpp implementation **/

#include "common_utils.hpp"

#include <Arduino.h>

namespace iot_module {
namespace common_utils {
bool check_with_latency(bool sensor_input, unsigned long &next_check_time, unsigned long latency_ms) {
    // Filters out short impulses
  const unsigned long now = millis();
  // First check. Movement detection setting timer
  if (!next_check_time && sensor_input) next_check_time = now + latency_ms;
  // Second check
  if (next_check_time && now >= next_check_time) {
    // Long impulse, real movement detected
    if (sensor_input) return true;
    // Short impus, probably false positive.
    // Also sets timer to 0 after long impuls end
    else next_check_time = 0;
  }
  return false;
}
}
}