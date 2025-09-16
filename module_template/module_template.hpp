/** Contains a base for all modules: over the air (OTA) update support, web-server

Usage:
Send HTTP POST or GET request, to make the device go to firmware update mode for 120 seconds.
Wait for a few seconds so Arduino IDE can make a connection.
Upload your new firmware. DON'T FORGET to add OTA support to the new firmware for the next updates.  
**/

#pragma once

#include <cstddef>
#include <cstring>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>

#include "parameters.hpp"
#include "secrets.hpp"
#include "common_utils.hpp"

/**==API==**/

// Define BEFORE includeing this file, to set module name
// #define MODULE_HOSTNAME ""

/// HTTP server
ESP8266WebServer SERVER(80);
/*
Provides some default URI handling:
  /settings - use paramters to set values to SETTINGS. Redirects to /state
  /state - get current STATE and SETTINGS
  /firmware_update - start firmware update mode. The divice will wait for update for 120 seconds
  /reset - reset the device
*/

// Use these to sotre current state and settings. 
// Save indexes from .add() and update values with [] 
/// Current system state variables
Parameters STATE;
/// Settings
Parameters SETTINGS;

// This functiouns MUST be implemented in a concrete emoudule
/// Implement this instead of steup() in the module
void setup_module();
/// Implement this instead of loop() in the module
void loop_module();
// Update values in STATE variable. Leave empty if not needed.
// This is only for the ones that have not been (or cannot be) updated in other places.
// Like current time, value from input pins, etc. 
void force_update_state_module();


/**==Implementation==**/

namespace module_template_impl {

const int UPLOAD_FIRMWARE_PORT = 8266;
// When true, module starts to wait for firmware update for 120 seconds
bool IS_FIRMWARE_UPDATE_MODE = false;
// Wait for firmware for this amount of time
constexpr unsigned long FIRMWARE_MODE_DURATION_MS = 120 * 1000;


/* SETTINGS indexes */

/* STATE indexes */
size_t st_MODULE_HOSTNAME = -1;
size_t st_UPTIME = -1;

void init_settings();
void init_state();
void init_wifi();
void init_ota();
void init_http_server();

void handle_state();
void handle_firmware_update();
void handle_not_found();
void handle_reset();
void handle_settings();

void update_firmware_if_needed();
void process_ota_error(ota_error_t error);
// Update STATE variable to current values
void force_update_state();
String get_uptime_str();

void setup() {
  Serial.begin(115200);
  Serial.println("Init started...");

  init_settings();
  init_state();
  init_wifi();
  init_ota();
  init_http_server();

  setup_module();

  Serial.println("Init finished.");
}


void loop() {
  SERVER.handleClient();
  update_firmware_if_needed();

  loop_module();
}

void init_wifi() {
  Serial.println("Wi-fi init started...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Wi-fi connection Failed! Rebooting...");
    delay(1000);
    ESP.restart();
  }

  Serial.println("Wi-fi init finished. IP address: " + WiFi.localIP().toString());
}


void init_ota() {
  Serial.println("OTA init started...");

  ArduinoOTA.setPort(UPLOAD_FIRMWARE_PORT);
  #ifdef MODULE_HOSTNAME
    ArduinoOTA.setHostname(MODULE_HOSTNAME);
  #endif
  STATE[st_MODULE_HOSTNAME] = ArduinoOTA.getHostname();
  Serial.println("Upload port: " + String(UPLOAD_FIRMWARE_PORT) + ", hostname: " + ArduinoOTA.getHostname());
  
  if (std::strlen(UPLOAD_FIRMWARE_PASSWORD_MD5)) {
    ArduinoOTA.setPasswordHash(UPLOAD_FIRMWARE_PASSWORD_MD5);
  }

  ArduinoOTA.onStart([]() {
    String upload_type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      upload_type = "sketch";
    } else { 
      upload_type = "filesystem";
    }
    Serial.println("Start updating " + upload_type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nFirmware upload finished.");
  });
  ArduinoOTA.onProgress([](unsigned int processed, unsigned int total) {
    Serial.printf("\nUpload: %u%%\r", (processed / (total / 100)));
  });
  ArduinoOTA.onError(
    process_ota_error
  );
  ArduinoOTA.begin();

  Serial.println("OTA init finished.");
}

void init_settings() {}

void init_state() {
  st_MODULE_HOSTNAME = STATE.add("Module host name", "");
  STATE.add("Firmware update", {String(__DATE__) + " " + __TIME__});
  st_UPTIME = STATE.add("Uptime", {""});
}

void init_http_server() {
  Serial.println("HTTP server init started...");

  SERVER.on("/settings", HTTP_GET, handle_settings);
  SERVER.on("/state", HTTP_GET, handle_state);
  SERVER.on("/firmware_update", HTTP_ANY, handle_firmware_update);
  SERVER.on("/reset", HTTP_ANY, handle_reset);
  SERVER.onNotFound(handle_not_found);
  SERVER.begin();

  Serial.println("HTTP server init finished.");
}

void handle_state() {
  force_update_state();
  String payload = String("{\"State\":") + STATE.to_json() + ",\"Settings\":" + SETTINGS.to_json() + "}";
  SERVER.send(200, "application/json", payload);
}

void handle_not_found() {
  String page = ("404: Not Found"
    "\nURI: " + SERVER.uri()
  );
  SERVER.send(404, "text/plain", page);
}

void handle_reset() {
  ESP.restart();
}

void handle_firmware_update() {
  IS_FIRMWARE_UPDATE_MODE = true;
  SERVER.send(200, "text/plain", "Firmware update mode started...");
}

void handle_settings() {
  const size_t args_sz = SERVER.args();
  // Updates paramter only if it already exists in SETTINGS
  for (size_t i = 0; i < args_sz; ++i) {
    // Arg names are CASE SENSITIVE!
    SETTINGS.try_set_from_str(SERVER.argName(i), SERVER.arg(i));
  }
  // redirect to /state
  String page = String("<head><meta http-equiv=\"refresh\" content=\"0; url=/state\"></head>");
  SERVER.send(200, "text/html", page);
}

void update_firmware_if_needed() {
  if (!IS_FIRMWARE_UPDATE_MODE)
    return;
  const unsigned long fu_mode_end = millis() + FIRMWARE_MODE_DURATION_MS;
  while (IS_FIRMWARE_UPDATE_MODE && millis() < fu_mode_end) {
    ArduinoOTA.handle();
    delay(10);
  }
  IS_FIRMWARE_UPDATE_MODE = false;
}

void process_ota_error(ota_error_t error) {
  Serial.printf("\nOTA error %u: ", error);
  switch (error) {
      case OTA_AUTH_ERROR:
        Serial.println("Authentication failed");
        break;
      case OTA_BEGIN_ERROR:
        Serial.println("Begin failed");
        break;
      case OTA_CONNECT_ERROR:
        Serial.println("Connect failed");
        break;
      case OTA_RECEIVE_ERROR:
        Serial.println("Receive failed");
        break;
      case OTA_END_ERROR:
        Serial.println("End failed");
        break;
      default:
        Serial.println("Other error");
        break;
  } 
}

void force_update_state() {
  STATE[st_UPTIME] = get_uptime_str();

  force_update_state_module();
}

String get_uptime_str() {
  const unsigned long now_s = millis() / 1000UL;
  const unsigned long seconds = now_s % 60UL;
  const unsigned long minutes = now_s / 60UL % 60UL;
  const unsigned long hours = now_s / 3600UL % 24UL;
  const unsigned long days = now_s / 86400UL;
  return iot_module::common_utils::get_formatted_time(days, hours, minutes, seconds);
}

}

/// Don't use setup() in actual module. Use setup_module() instead!
void setup() 
{
  module_template_impl::setup();
}
/// Don't use loop() in actual module. Use loop_module() instead!
void loop() {
  module_template_impl::loop();
}
