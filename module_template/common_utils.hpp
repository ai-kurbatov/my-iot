/** Contains some common utility functions **/

#pragma once

#include <Arduino.h>

namespace iot_module {
namespace common_utils {
/**
  @brief Get formatted time String like "69 days 16:42:34"
  @param days a number of days
  @param hours a number of hours
  @param minutes a number of mintues
  @param seconds a number of seconds
  @returns a formatted time stirng
**/
String get_formatted_time(int days, int hours, int minutes, int seconds);

/**
  @brief Checks if sensor input is true. Filters out signals shorter then latency_ms. Should be called periodically for proper latency handling.
  @param sensor_input input signal from sensor, button, etc.
  @param next_check_time the fuction manages this variable. Store it (as a static variable, global varibale, etc.) and
  pass with each call for the sensor. Before the first callinitialize it with 0 if sensor_input is false or 
  millis() + latency_ms if sensor_input is true. Or just with 0 first time, and it will update it to actual on scenod call
  @param latency_ms sensor_input must be true for this amount of time (ms) for function to return true
  @returns true, if input stays true for latency time, false otherwise.
**/
bool check_with_latency(bool sensor_input, unsigned long &next_check_time, unsigned long latency_ms);
}
}