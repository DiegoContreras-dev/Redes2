#include <Arduino.h>

const int POT_R_PIN = A0;
const int POT_G_PIN = A1;
const int POT_B_PIN = A2;

const int LED_R_PIN = 6;
const int LED_G_PIN = 10;
const int LED_B_PIN = 11;

void setup() {
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
}

void loop() {
  int red = analogRead(POT_R_PIN) / 4;
  int green = analogRead(POT_G_PIN) / 4;
  int blue = analogRead(POT_B_PIN) / 4;

  analogWrite(LED_R_PIN, red);
  analogWrite(LED_G_PIN, green);
  analogWrite(LED_B_PIN, blue);
}
