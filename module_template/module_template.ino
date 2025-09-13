/**A module implementation example**/

#define MODULE_HOSTNAME "yet-another-iot-device"

#include "module_template.hpp"

size_t SWITCH_COUNT_ST = -1;
size_t D5_ST = -1;

size_t BLINKING_INTERVAL_SE = -1;

void setup_module() {
  pinMode(D5, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  SERVER.on("/hello", HTTP_ANY, [](){SERVER.send(200, "text/plain", "Hello world!");});

  SWITCH_COUNT_ST = STATE.add("Switch count", 0);
  D5_ST = STATE.add("D5 pin state", static_cast<bool>(digitalRead(D5)));

  BLINKING_INTERVAL_SE = SETTINGS.add("Blinking interval", 500);
}

void loop_module() {
  delay(SETTINGS[BLINKING_INTERVAL_SE].as_int());
  digitalWrite(LED_BUILTIN, HIGH);
  delay(SETTINGS[BLINKING_INTERVAL_SE].as_int());
  digitalWrite(LED_BUILTIN, LOW);

  STATE[SWITCH_COUNT_ST] = STATE[SWITCH_COUNT_ST].as_int() + 1;
}

void force_update_state_module() {
  // Update the value to see the current state in /state
  STATE[D5_ST] = static_cast<bool>(digitalRead(D5));
}
