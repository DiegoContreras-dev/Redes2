#include <Arduino.h>

const int BUTTON_PIN = 2;
const int LED_PIN = 11;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // INPUT_PULLUP invierte la lógica: presionado = LOW, suelto = HIGH
  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  digitalWrite(LED_PIN, pressed ? HIGH : LOW);
}
