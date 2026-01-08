/**A module implementation example**/

#define MODULE_HOSTNAME "auto-watering-iot-device"

#include "module_template.hpp"

const auto WATERING_PIN = D6;
const auto WATERING_DEVICE_STATE_PIN = D7;

// STATE indexes
size_t st_IS_ON_WATERING = -1;

// SETTINGS indexes
size_t se_WATERING_DELAY_S = -1;
size_t se_WATERING_DURATION_S = -1;

// turn on or off watering depanding on time
void update_watering_state();
// turn on or off watering
void set_watering_state(bool is_turn_on);
// is watering device actually on
bool get_watering_device_state();
// turn off watering device at the setup
void init_vatering_device();

void setup_module() {
  st_IS_ON_WATERING = STATE.add("IsOnWatering", false);

  se_WATERING_DELAY_S = SETTINGS.add("WateringDelayS", 3*60*60);
  se_WATERING_DURATION_S = SETTINGS.add("WateringDurationS", 1*60);

  pinMode(WATERING_PIN, OUTPUT);
  digitalWrite(WATERING_PIN, false); // WARNING: May ocassionaly turn on watering device!
  pinMode(WATERING_DEVICE_STATE_PIN, INPUT);

  delay(500);
  init_vatering_device();
}

void loop_module() {
  update_watering_state();
}

void force_update_state_module() {
}

void update_watering_state() {
  const auto now_ms = millis();
  const unsigned long watering_delay_ms = SETTINGS[se_WATERING_DELAY_S].as_int() * 1000;
  static unsigned long previous_watering_start_ms = 0ul - watering_delay_ms; // Turn on watering on startup
  if (now_ms - previous_watering_start_ms > watering_delay_ms) {
    previous_watering_start_ms = now_ms;
    set_watering_state(true);
    return;
  }
  const unsigned long watering_duration_ms = SETTINGS[se_WATERING_DURATION_S].as_int() * 1000;
  if (now_ms - previous_watering_start_ms > watering_duration_ms) {
    set_watering_state(false);
  }
}

void set_watering_state(bool is_turn_on) {
  if (STATE[st_IS_ON_WATERING].as_bool() == is_turn_on)
    return;
  
  // Switch like a button press
  do {
    digitalWrite(WATERING_PIN, true);
    delay(50);
    digitalWrite(WATERING_PIN, false);
    delay(500);
  } while (get_watering_device_state() != is_turn_on);

  STATE[st_IS_ON_WATERING] = is_turn_on;
}

bool get_watering_device_state() {
  return !digitalRead(WATERING_DEVICE_STATE_PIN);
}

void init_vatering_device() {
  // Turn off watering device if it has been turned on during setup
  if (get_watering_device_state()) {
    STATE[st_IS_ON_WATERING] = true;
    set_watering_state(false);
  }
}
