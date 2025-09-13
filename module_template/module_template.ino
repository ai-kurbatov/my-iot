/**A module implementation example**/

#define MODULE_HOSTNAME "yet-another-iot-device"

#include "module_template.hpp"

size_t SWITCH_COUNT_ST = -1;

size_t BLINKING_INTERVAL_SE = -1;

void setup_module() {
  SERVER.on("/hello", HTTP_ANY, [](){SERVER.send(200, "text/plain", "Hello world!");});

  SWITCH_COUNT_ST = STATE.add("Switch count", 0);

  BLINKING_INTERVAL_SE = SETTINGS.add("Blinking interval", 500);

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop_module() {
  delay(SETTINGS[BLINKING_INTERVAL_SE].asInt());
  digitalWrite(LED_BUILTIN, HIGH);
  delay(SETTINGS[BLINKING_INTERVAL_SE].asInt());
  digitalWrite(LED_BUILTIN, LOW);

  STATE[SWITCH_COUNT_ST] = STATE[SWITCH_COUNT_ST].asInt() + 1;
}

void force_update_state_module() {}
