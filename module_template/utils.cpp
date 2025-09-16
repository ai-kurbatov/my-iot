/** utils.hpp implementation **/

#include "utils.hpp"

namespace iot_module {
namespace utils {
String get_formatted_time(int days, int hours, int minutes, int seconds) {
  String days_s = days > 0 ? days + String(" days ") : String("");
  String hours_s = hours >= 10 ? String(hours) : "0" + String(hours);
  String minutes_s = minutes >= 10 ? String(minutes) : "0" + String(minutes);
  String seconds_s = seconds >= 10 ? String(seconds) : "0" + String(seconds);
  return days_s + hours_s + String(":") + minutes_s + String(":") + seconds_s;
}


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