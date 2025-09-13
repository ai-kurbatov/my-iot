/**Implements a module for a hallway. It has two move sensors and which control 
main light and night light for depending on time of the day.**/

#include <ESP8266WiFi.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <dhcpserver.h>
#include "NTPClient.h"
#include "WiFiUdp.h"
#include <ESP8266WebServer.h>

#include "secrets.hpp" // const char WIFI_NAME[] = ..., WIFI_PASSWORD[] = ...

#define DEBUG 0

const int NAPT = 1000;
const int NAPT_PORT = 10;
const int TZ_OFFSET_SEC = 3 * 60 * 60;
const int TIME_UPDATE_INTERVAL_MS = 24 * 60 * 1000;

const int LED_PIN = D1;
const int MAIN_LIGHT_PIN = D2;
const int SENSOR_1_PIN = D5;
const int SENSOR_2_PIN = D6;

// Current light state
bool IS_CURRENTLY_TURNED_ON = false;

// If true, disable auto mode, turn off any light
bool IS_FORCE_TURN_OFF = false;
// If true, disable auto mode, turn on main light
bool IS_FORCE_TURN_ON = false;
// Use led instead of main light from this time
int NIGHT_MODE_BEGIN_H = 23;
int NIGHT_MODE_BEGIN_M = 0;
// to this time
int NIGHT_MODE_END_H = 9;
int NIGHT_MODE_END_M = 0;
// Light stays turned on for this amount of time after a movement being detected
int LIGHT_TIME_S = 60;

WiFiUDP NTP_UDP;
NTPClient TIME_CLIENT(NTP_UDP, "pool.ntp.org", TZ_OFFSET_SEC, TIME_UPDATE_INTERVAL_MS);
ESP8266WebServer SERVER(80);

void(* reset_device) (void) = 0;
void init_wifi();
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
void handle_set();
void handle_reset();
bool set_arg_if_passed(const String& arg_name, int& param);
bool set_arg_if_passed(const String& arg_name, bool& param);
String get_uptime_str();
String get_formatted_time(int days, int hours, int minutes, int seconds);
String get_current_params();

void setup() {
  Serial.begin(115200);

  init_wifi();
  init_time_server();
  init_light();
  init_http_server();
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(MAIN_LIGHT_PIN, OUTPUT);
  pinMode(SENSOR_1_PIN, INPUT_PULLUP);
  pinMode(SENSOR_2_PIN, INPUT_PULLUP);
  Serial.println("Init finished");
  
  #if DEBUG
    Serial.println(String("Sensor_1,Sensor_2,x\n"));
  #endif
}

void loop() {
  TIME_CLIENT.update();
  SERVER.handleClient();

  set_light_state(is_movement_detected());
}

void init_wifi() {
  delay(100);
  // first, connect to STA so we can get a proper local DNS server
  WiFi.mode(WIFI_STA);
  const unsigned long max_timeout = 100 * 1000;
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED && millis() < max_timeout) {
    delay(100);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("WiFi.Status(): %d\n", WiFi.status());
    reset_device();
  }

  // give DNS servers to AP side
  dhcps_set_dns(0, WiFi.dnsIP(0));
  dhcps_set_dns(1, WiFi.dnsIP(1));

  WiFi.softAPConfig(  // enable AP, with android-compatible google domain
    IPAddress(172, 217, 28, 254),
    IPAddress(172, 217, 28, 254),
    IPAddress(255, 255, 255, 0));


  err_t ret = ip_napt_init(NAPT, NAPT_PORT);
  if (ret == ERR_OK) {
    ret = ip_napt_enable_no(SOFTAP_IF, 1);
  }

  Serial.print("\nWi-Fi init finished, IP: ");
  Serial.println(WiFi.localIP());
}

void init_time_server() {
  TIME_CLIENT.begin();
  TIME_CLIENT.forceUpdate();
  if (TIME_CLIENT.getEpochTime() < 1604143340) {
    Serial.println("Resetting because of time bug");
    reset_device();
  }
  Serial.println("Time server init finished");
}

void init_light() {
  set_main_light_state(false);
  set_led_state(false);
  IS_CURRENTLY_TURNED_ON = false;
  Serial.println("Light init finished");
}

void init_http_server() {
  SERVER.on("/", HTTP_GET, handle_root);
  SERVER.on("/set", HTTP_ANY, handle_set);
  SERVER.on("/reset", HTTP_ANY, handle_reset);
  
  SERVER.onNotFound(handle_not_found);
  SERVER.begin();
  Serial.println("HTTP init finished");
}

bool is_movement_detected() {
  bool sensor_1 = digitalRead(SENSOR_1_PIN);
  bool sensor_2 = digitalRead(SENSOR_2_PIN);
  #if DEBUG
    Serial.println(String(sensor_1) + "," + String(sensor_2 * 1.1) + ",0\n");
  #endif
  return sensor_1 || sensor_2;
}

void set_light_state(bool is_movement_detected) {
  static int be_on_up_to = 0;  // The light is turned on to this timestamp

  if (IS_FORCE_TURN_OFF || IS_FORCE_TURN_ON) {
    set_force_light();
    return;
  }

  int now = TIME_CLIENT.getEpochTime();
  if (is_movement_detected)
    be_on_up_to = now + LIGHT_TIME_S;
  
  bool is_light_finished = now > be_on_up_to;

  // State have not changed
  if ((IS_CURRENTLY_TURNED_ON && is_movement_detected)
      || (!IS_CURRENTLY_TURNED_ON && !is_movement_detected)
      || (!is_light_finished && IS_CURRENTLY_TURNED_ON)
      || (is_light_finished && !IS_CURRENTLY_TURNED_ON))
      return;
  
  IS_CURRENTLY_TURNED_ON = is_movement_detected;

  // Turn off everything just in case
  if (!IS_CURRENTLY_TURNED_ON) {
    set_led_state(false);
    set_main_light_state(false);
    return;
  }
  // Turn on only one thing, turn off the other
  if (is_night_mode()) {
    set_main_light_state(false);
    set_led_state(true);
    return;
  }
  set_main_light_state(true);
  set_led_state(false);
}

void set_force_light() {
  if (IS_FORCE_TURN_OFF && IS_CURRENTLY_TURNED_ON) {
    IS_CURRENTLY_TURNED_ON = false;
    set_main_light_state(IS_CURRENTLY_TURNED_ON);
    set_led_state(false);
    return;
  }
  if (IS_FORCE_TURN_ON && !IS_CURRENTLY_TURNED_ON) {
    IS_CURRENTLY_TURNED_ON = true;
    set_main_light_state(IS_CURRENTLY_TURNED_ON);
    set_led_state(false);
  }
}

bool is_night_mode() {
  const int night_mode_begin = convert_to_minutes(NIGHT_MODE_BEGIN_H, NIGHT_MODE_BEGIN_M);
  const int night_mode_end = convert_to_minutes(NIGHT_MODE_END_H, NIGHT_MODE_END_M);
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

String get_current_params() {
  String current_params = (
    String("Current state:")
    + "\n  State:              " + (IS_CURRENTLY_TURNED_ON ? String("on") : String("off"))
    + "\n  Time:               " + TIME_CLIENT.getFormattedTime() + String(" (") + TIME_CLIENT.getEpochTime() + String(")")
    + "\n  Uptime:             " + get_uptime_str()
    + "\n  Firmware update:    " + __DATE__ + " " + __TIME__
    + "\n  Sensor 1 state:     " + digitalRead(SENSOR_1_PIN)
    + "\n  Sensor 2 state:     " + digitalRead(SENSOR_2_PIN)
    + "\nSettings:"
    + "\n  NIGHT_MODE_BEGIN_H: " + NIGHT_MODE_BEGIN_H
    + "\n  NIGHT_MODE_BEGIN_M: " + NIGHT_MODE_BEGIN_M
    + "\n  NIGHT_MODE_END_H:   " + NIGHT_MODE_END_H
    + "\n  NIGHT_MODE_END_M:   " + NIGHT_MODE_END_M
    + "\n  LIGHT_TIME_S:       " + LIGHT_TIME_S 
    + "\n  IS_FORCE_TURN_OFF:  " + IS_FORCE_TURN_OFF
    + "\n  IS_FORCE_TURN_ON:   " + IS_FORCE_TURN_ON
    + "\n"
    + "\nUse /set?PARAM_NAME=42 to change it."
  );
  return current_params;
} 

void handle_root() {
  String page = get_current_params();
  SERVER.send(200, "text/plain", page);
}

void handle_reset() {
  reset_device();
}

String get_uptime_str() {
  const unsigned long now_s = millis() / 1000UL;
  const unsigned long seconds = now_s % 60UL;
  const unsigned long minutes = now_s / 60UL % 60UL;
  const unsigned long hours = now_s / 3600UL % 24UL;
  const unsigned long days = now_s / 86400UL;
  return get_formatted_time(days, hours, minutes, seconds);
}

String get_formatted_time(int days, int hours, int minutes, int seconds) {
  String days_s = days > 0 ? days + String(" days ") : String("");
  String hours_s = hours >= 10 ? String(hours) : "0" + String(hours);
  String minutes_s = minutes >= 10 ? String(minutes) : "0" + String(minutes);
  String seconds_s = seconds >= 10 ? String(seconds) : "0" + String(seconds);
  return days_s + hours_s + String(":") + minutes_s + String(":") + seconds_s;
}

void handle_set() {
  set_arg_if_passed("NIGHT_MODE_BEGIN_H", NIGHT_MODE_BEGIN_H);
  set_arg_if_passed("NIGHT_MODE_BEGIN_M", NIGHT_MODE_BEGIN_M);
  set_arg_if_passed("NIGHT_MODE_END_H", NIGHT_MODE_END_H);
  set_arg_if_passed("NIGHT_MODE_END_M", NIGHT_MODE_END_M);
  set_arg_if_passed("LIGHT_TIME_S", LIGHT_TIME_S);
  const bool is_force_off_set = set_arg_if_passed("IS_FORCE_TURN_OFF", IS_FORCE_TURN_OFF);
  if (is_force_off_set && IS_FORCE_TURN_OFF)
    IS_FORCE_TURN_ON = false;
  const bool is_force_on_set = set_arg_if_passed("IS_FORCE_TURN_ON", IS_FORCE_TURN_ON);
  if (is_force_on_set && IS_FORCE_TURN_ON) {
    IS_FORCE_TURN_OFF = false;
    // workaround to fix bug, when night light is already on and "force turn on" is pressed
    IS_CURRENTLY_TURNED_ON = false;
  }

  // redirect to root
  String page = String("<head><meta http-equiv=\"refresh\" content=\"0; url=/\"></head>");
  SERVER.send(200, "text/html", page);
}

void handle_not_found() {
  String page = ("404: Not Found"
    "\nURI: " + SERVER.uri()
  );
  SERVER.send(404, "text/plain", page);
}

bool set_arg_if_passed(const String& arg_name, int& param) {
  if (!SERVER.hasArg(arg_name))
    return false;
  param = String(SERVER.arg(arg_name)).toInt();
  return true;
}

bool set_arg_if_passed(const String& arg_name, bool& param) {
  int tmp = -1;
  if (!set_arg_if_passed(arg_name, tmp))
    return false;
  param = static_cast<bool>(tmp);
  return true;
}
