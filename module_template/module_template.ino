/**A module implementation example**/

#define MODULE_HOSTNAME "yet-another-iot-device"

#include "module_template.hpp"

// STATE indexes
size_t st_SWITCH_COUNT = -1;
size_t st_D5 = -1;

// SETTINGS indexes
size_t se_BLINKING_INTERVAL = -1;

void setup_module() {
  // It's better to add STATE and SETTINGS values at the beginning of the setup
  // so the other functions can use them
  st_SWITCH_COUNT = STATE.add("Switch count", 0);
  st_D5 = STATE.add("D5 pin state", static_cast<bool>(digitalRead(D5)));

  se_BLINKING_INTERVAL = SETTINGS.add("Blinking interval", 500);

  pinMode(D5, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  SERVER.on("/hello", HTTP_ANY, [](){SERVER.send(200, "text/plain", "Hello world!");});
}

void loop_module() {
  delay(SETTINGS[se_BLINKING_INTERVAL].as_int());
  digitalWrite(LED_BUILTIN, HIGH);
  delay(SETTINGS[se_BLINKING_INTERVAL].as_int());
  digitalWrite(LED_BUILTIN, LOW);

  STATE[st_SWITCH_COUNT] = STATE[st_SWITCH_COUNT].as_int() + 1;
}

void force_update_state_module() {
  // Update the value to see the current state in /state
  STATE[st_D5] = static_cast<bool>(digitalRead(D5));
}
