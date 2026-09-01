#include <Arduino.h>

const int POT_R_PIN = A0;
const int POT_G_PIN = A1;
const int POT_B_PIN = A2;

const int LED_R_PIN = 11;
const int LED_G_PIN = 6;
const int LED_B_PIN = 10;

const int BUTTON_PIN = 5;
const int BUTTON_LED_PIN = 7;

const unsigned long IntervaloMS = 10000;
unsigned long UltimoIntervalo = 0;

void setup() {
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);

  // INPUT_PULLUP invierte la lógica: presionado = LOW, suelto = HIGH
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_LED_PIN, OUTPUT);

  Serial.begin(9600);
}

void loop() {
  int red = analogRead(POT_R_PIN) / 4;
  int green = analogRead(POT_G_PIN) / 4;
  int blue = analogRead(POT_B_PIN) / 4;

  // DEBUG TEMPORAL: valor crudo de A0 para diagnosticar el pot rojo
  analogWrite(LED_R_PIN, 255 - red);
  analogWrite(LED_G_PIN, 255 - green);
  analogWrite(LED_B_PIN, 255 - blue);

  bool pressed = digitalRead(BUTTON_PIN) == LOW;
  digitalWrite(BUTTON_LED_PIN, pressed ? HIGH : LOW);

  unsigned long now = millis();
  if (now - UltimoIntervalo >= IntervaloMS) {
    UltimoIntervalo = now;
    Serial.print("Potenciometro R (A0): ");
    Serial.println(red);
  }
}
