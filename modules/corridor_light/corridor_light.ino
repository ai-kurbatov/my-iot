/**Implements a module for a hallway. It has two move sensors and which control 
main light and night light for depending on time of the day.**/

#include <ESP8266WiFi.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <dhcpserver.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define MODULE_HOSTNAME "corridor-light"
#include "module_template.hpp"
#include "common_utils.hpp"

using iot_module::common_utils::check_with_latency;

#define DEBUG 0

enum Mode {
  AUTO, // Control light by motion sensors
  FORCE_ON, // Always on
  FORCE_OFF // Always off
};

const int NAPT = 1000;
const int NAPT_PORT = 10;
const int TZ_OFFSET_SEC = 3 * 60 * 60;
const unsigned long TIME_UPDATE_INTERVAL_MS = 24 * 60 * 1000;

const int LED_PIN = D1;
const int MAIN_LIGHT_PIN = D2;
const int SENSOR_1_PIN = D5;
const int SENSOR_2_PIN = D6;

/* SETTINGS indexes */
size_t se_SENSOR_LATENCY_MS = -1;
size_t se_MODE = -1;
// Use led instead of main light from this time
size_t se_NIGHT_MODE_BEGIN_H = -1;
size_t se_NIGHT_MODE_BEGIN_M = -1;
// to this time
size_t se_NIGHT_MODE_END_H = -1;
size_t se_NIGHT_MODE_END_M = -1;
  // Light stays turned on for this amount of time after a movement being detected
size_t se_LIGHT_TIME_S = -1;

/* STATE indexes */
// Current light state
size_t st_IS_CURRENTLY_TURNED_ON = -1;
size_t st_SENSOR_1_STATE = -1;
size_t st_SENSOR_2_STATE = -1;
size_t st_IS_NIGHT_MODE = -1;
// Current time string. Updates only on /settings request
size_t st_TIME = -1;

WiFiUDP NTP_UDP;
NTPClient TIME_CLIENT(NTP_UDP, "pool.ntp.org", TZ_OFFSET_SEC, TIME_UPDATE_INTERVAL_MS);

void init_settings();
void init_state();
void init_time_server();
void init_light();
void init_http_server();

bool is_movement_detected();
void set_light_state(bool is_movement_detected);
void set_led_state(bool is_turn_on);
void set_main_light_state(bool is_turn_on);
int convert_bool_to_pin_signal(bool value);
int convert_to_minutes(int hours, int minutes);
bool is_night_mode();
void set_force_light();

void handle_root();
void handle_not_found();
void handle_reset();
String get_formatted_time(int days, int hours, int minutes, int seconds);

void setup_module() {
  init_settings();
  init_state();
  init_http_server();
  init_time_server();
  init_light();
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(MAIN_LIGHT_PIN, OUTPUT);
  pinMode(SENSOR_1_PIN, INPUT_PULLUP);
  pinMode(SENSOR_2_PIN, INPUT_PULLUP);
  
  #if DEBUG
    Serial.println(String("Sensor_1,Sensor_2,x\n"));
  #endif
}

void loop_module() {
  TIME_CLIENT.update();

  set_light_state(is_movement_detected());
}

void force_update_state_module() {
  STATE[st_SENSOR_1_STATE] = static_cast<bool>(digitalRead(SENSOR_1_PIN));
  STATE[st_SENSOR_2_STATE] = static_cast<bool>(digitalRead(SENSOR_2_PIN));
  STATE[st_IS_NIGHT_MODE] = is_night_mode();
  STATE[st_TIME] = String("") + TIME_CLIENT.getFormattedTime() + " (" + TIME_CLIENT.getEpochTime() + ")";
}

void init_settings() {
  se_MODE = SETTINGS.add("Mode", Mode::AUTO);
  se_SENSOR_LATENCY_MS = SETTINGS.add("Sensor latency (ms)", 500);  // Stores as int! It will overflow on big values
  se_NIGHT_MODE_BEGIN_H = SETTINGS.add("Night mode begin, h", 23);
  se_NIGHT_MODE_BEGIN_M = SETTINGS.add("Night mode begin, m", 0);
  se_NIGHT_MODE_END_H = SETTINGS.add("Night mode end, h", 8);
  se_NIGHT_MODE_END_M = SETTINGS.add("Night mode end, m", 0);
  se_LIGHT_TIME_S = SETTINGS.add("Light time, s",
    #if DEBUG
      10
    #else
      60
    #endif
  );
}

void init_state() {
  st_TIME = STATE.add("Time", {});
  st_IS_CURRENTLY_TURNED_ON = STATE.add("Lights state", false);
  st_IS_NIGHT_MODE = STATE.add("Night mode", false);
  st_SENSOR_1_STATE = STATE.add("Sensor 1", false);
  st_SENSOR_2_STATE = STATE.add("Sensor 2", false);
}

void init_time_server() {
  TIME_CLIENT.begin();
  TIME_CLIENT.forceUpdate();

  Serial.println("Time server init finished.");
}

void init_light() {
  set_main_light_state(false);
  set_led_state(false);
  STATE[st_IS_CURRENTLY_TURNED_ON] = false;
  Serial.println("Light init finished.");
}

void init_http_server() {
  SERVER.on("/", HTTP_GET, handle_root);
  Serial.println("Module HTTP server init finished.");
}

bool is_movement_detected() {
  static unsigned long sensor_1_next_check = 0, sensor_2_next_check = 0;

  const bool sensor_1_input = digitalRead(SENSOR_1_PIN);
  const bool sensor_2_input = digitalRead(SENSOR_2_PIN);
  #if DEBUG
    Serial.println(String(sensor_1_input) + "," + String(sensor_2_input * 1.1) + ",0\n");
  #endif
  const unsigned long latency_ms = SETTINGS[se_SENSOR_LATENCY_MS].as_int();
  const bool sensor_1 = check_with_latency(sensor_1_input, sensor_1_next_check, latency_ms);
  const bool sensor_2 = check_with_latency(sensor_2_input, sensor_2_next_check, latency_ms);
  return sensor_1 || sensor_2;
}

void set_light_state(bool is_movement_detected) {
  static int be_on_up_to = 0;  // The light is turned on to this timestamp

  if (SETTINGS[se_MODE].as_int() == Mode::FORCE_OFF || SETTINGS[se_MODE].as_int() == Mode::FORCE_ON) {
    set_force_light();
    return;
  }

  int now = TIME_CLIENT.getEpochTime();
  if (is_movement_detected)
    be_on_up_to = now + SETTINGS[se_LIGHT_TIME_S].as_int();
  
  bool is_light_finished = now > be_on_up_to;

  // State have not changed
  if ((STATE[st_IS_CURRENTLY_TURNED_ON].as_bool() && is_movement_detected)
      || (!STATE[st_IS_CURRENTLY_TURNED_ON].as_bool() && !is_movement_detected)
      || (!is_light_finished && STATE[st_IS_CURRENTLY_TURNED_ON].as_bool())
      || (is_light_finished && !STATE[st_IS_CURRENTLY_TURNED_ON].as_bool()))
      return;
  
  STATE[st_IS_CURRENTLY_TURNED_ON] = is_movement_detected;

  // Turn off everything just in case
  if (!STATE[st_IS_CURRENTLY_TURNED_ON].as_bool()) {
    set_led_state(false);
    set_main_light_state(false);
    return;
  }
  // Turn on only one thing, turn off the other
  STATE[st_IS_NIGHT_MODE] = is_night_mode();
  if (STATE[st_IS_NIGHT_MODE].as_bool()) {
    set_main_light_state(false);
    set_led_state(true);
    return;
  }
  set_main_light_state(true);
  set_led_state(false);
}

void set_force_light() {
  if (SETTINGS[se_MODE].as_int() == Mode::FORCE_OFF) {
    STATE[st_IS_CURRENTLY_TURNED_ON] = false;
    set_main_light_state(false);
    set_led_state(false);
    return;
  }
  if (SETTINGS[se_MODE].as_int() == Mode::FORCE_ON) {
    STATE[st_IS_CURRENTLY_TURNED_ON] = true;
    set_main_light_state(true);
    set_led_state(false);
  }
}

bool is_night_mode() {
  const int night_mode_begin = convert_to_minutes(SETTINGS[se_NIGHT_MODE_BEGIN_H].as_int(), SETTINGS[se_NIGHT_MODE_BEGIN_M].as_int());
  const int night_mode_end = convert_to_minutes(SETTINGS[se_NIGHT_MODE_END_H].as_int(), SETTINGS[se_NIGHT_MODE_END_M].as_int());
  const int current_min = convert_to_minutes(TIME_CLIENT.getHours(), TIME_CLIENT.getMinutes());
  return night_mode_begin <= current_min || current_min < night_mode_end;
}

void set_led_state(bool is_turn_on) {
  digitalWrite(LED_PIN, convert_bool_to_pin_signal(is_turn_on));
}

void set_main_light_state(bool is_turn_on) {
  digitalWrite(MAIN_LIGHT_PIN, convert_bool_to_pin_signal(is_turn_on));
}

int convert_bool_to_pin_signal(bool value) {
  return value ? HIGH : LOW;
}

int convert_to_minutes(int hours, int minutes) {
  return hours * 60 + minutes;
}

void handle_root() {
  String page = (
    String("<!doctype html>")
    + "<head><style>"
    + " body{background:#0b0b0c;color:#dcdcdc;}"
    + " a:link { color: #5fb3ff; } a:visited { color: #3b8ed6; } a { text-decoration: underline; } a:hover, a:focus { color: #1f6fb0; }"
    + "</style></head><body>"
    + "It's a " + MODULE_HOSTNAME + " module."
    + "<p><a href=/state>/state</a> - current system state"
    + "<p><a href=/settings>/settings</a> - pass parameters to change settings"
    + "<p><a href=/firmware_update>/firmware_update</a> - to enter firmware update for a few minutes"
    + "<p><a href=/reset>/reset</a> - reset the device"
    + "</body></html>");
  SERVER.send(200, "text/html", page);
}

