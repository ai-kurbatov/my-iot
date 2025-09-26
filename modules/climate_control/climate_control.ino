/**A module that regulates temperature **/

#define MODULE_HOSTNAME "climate-control"

#include <limits>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "module_template.hpp"

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

const uint8_t HEATER_PIN = D5;
const uint8_t CLIMATE_SENSOR_SCL_PIN = D1;
const uint8_t CLIMATE_SENSOR_SDA_PIN = D2;

// TODO: move to STATE
unsigned long HEATING_STARTED_MS = 0;
unsigned long HEATING_STOPPED_MS = 0;


Adafruit_BME280 CLIMATE_SENSOR;

// STATE indexes
size_t st_TEMPERATURE = SIZE_MAX;
size_t st_PRESSURE = SIZE_MAX;
size_t st_HUMIDITY = SIZE_MAX;
size_t st_IS_HEATER_ON = SIZE_MAX;

// SETTINGS indexes
size_t se_TARGET_TEMPERATURE = SIZE_MAX;
size_t se_HEATING_TIME_MIN_M = SIZE_MAX;
size_t se_HEATING_TIME_MAX_M = SIZE_MAX;
size_t se_HETING_DELAY_TIME_M = SIZE_MAX;

void init_state();
void init_settings();
void init_climate_sensor();

void update_heater_state();

void setup_module() {
    init_settings();
    init_state();
    init_climate_sensor();

    pinMode(HEATER_PIN, OUTPUT);
}

void loop_module() {
  update_heater_state();
}

void force_update_state_module() {
  STATE[st_TEMPERATURE] = String(CLIMATE_SENSOR.readTemperature()) + " °C";
  STATE[st_PRESSURE] = String(CLIMATE_SENSOR.readPressure() / 100.0f) + " hPa";
  STATE[st_HUMIDITY] = String(CLIMATE_SENSOR.readHumidity()) + " %";
}

void init_settings() {
  se_TARGET_TEMPERATURE = SETTINGS.add("TargetTemperature", 25); // TODO: replace with float
  // Minimum time for heater to be turned on (minutes)
  se_HEATING_TIME_MIN_M = SETTINGS.add("HeatingTimeMinM", 10);
  // Maximum time for heater to be turned on (minutes)
  se_HEATING_TIME_MAX_M = SETTINGS.add("HeatingTimeMaxM", 120);
  // Minimum time for heater to be turned off between heating sessions (minutes) 
  se_HETING_DELAY_TIME_M = SETTINGS.add("HeatingDelayTime", 5);
}

void init_state() {
  // TODO: replace String values with float values 
  st_TEMPERATURE = STATE.add("Temperature", "");
  st_PRESSURE = STATE.add("Pressure", "");
  st_HUMIDITY = STATE.add("Humidity", "");
  st_IS_HEATER_ON = STATE.add("IsHeaterOn", false);
}

void init_climate_sensor() {
  while(!Serial);
  Serial.println("Climate sensor init started...");

  Wire.begin();
  unsigned status = CLIMATE_SENSOR.begin(0x76, &Wire);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    delay(1000);
    ESP.restart();
  }

  Serial.println("Climate sensor init finished.");
}

void update_heater_state() {
  const unsigned long now_ms = millis();
  const bool is_on = STATE[st_IS_HEATER_ON].as_bool();
  // If heating for too long, turn it off for some time, no matter what current temperature is
  if (is_on && now_ms > HEATING_STARTED_MS + SETTINGS[se_HEATING_TIME_MAX_M].as_int() * 60 * 1000) {
    set_hater_state(false);
    return;
  }
  const float temperature = CLIMATE_SENSOR.readTemperature();
  if (temperature < static_cast<float>(SETTINGS[se_TARGET_TEMPERATURE].as_int())) {
      if (is_on) return;  // Already turned on
      else if (now_ms < HEATING_STOPPED_MS + SETTINGS[se_HETING_DELAY_TIME_M].as_int() * 60 * 1000) return;  // Turned off recently
      else set_hater_state(true); // Turn it on
  }
  else {
      if (!is_on) return; // Already turned off
      else if (now_ms < HEATING_STARTED_MS + SETTINGS[se_HEATING_TIME_MIN_M].as_int() * 60 * 1000) return;  // Turned on recently
      else set_hater_state(false); // Turn it off
  }
}

void set_hater_state(bool is_turn_on) {
  const unsigned long now_ms = millis();
  if (is_turn_on) {
      STATE[st_IS_HEATER_ON] = true;
      HEATING_STARTED_MS = now_ms;
      digitalWrite(HEATER_PIN, HIGH);
      return;
  }
  digitalWrite(HEATER_PIN, LOW);
  STATE[st_IS_HEATER_ON] = false;
  HEATING_STOPPED_MS = now_ms;
}