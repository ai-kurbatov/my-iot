/**A module implementation example**/

#define MODULE_HOSTNAME "yet-another-iot-device"

#include "module_template.hpp"

void setup_module() {
  SERVER.on("/hello", HTTP_ANY, [](){SERVER.send(200, "text/plain", "Hello world!");});

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop_module() {
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);

  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
}

void force_update_state_module() {}
