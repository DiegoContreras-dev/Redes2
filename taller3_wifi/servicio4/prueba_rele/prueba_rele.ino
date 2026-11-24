#include <Arduino.h>

constexpr uint8_t RELAY_PIN = 25;
// Suposicion inicial: modulo activo en LOW. Verificar con la carga desconectada.
bool activeLow = true;
bool testing = false;
unsigned long testStarted = 0;
String command;

void relay(bool on) {
  digitalWrite(RELAY_PIN, on ? (activeLow ? LOW : HIGH)
                           : (activeLow ? HIGH : LOW));
}

void stopTest() {
  testing = false;
  relay(false);
  Serial.println("Orden de apagado enviada. Verificar estado fisico del rele.");
}

void setup() {
  // Precargar el nivel antes de habilitar la salida reduce transitorios
  // del programa; no garantiza el nivel durante reset/arranque del chip.
  digitalWrite(RELAY_PIN, HIGH);
  pinMode(RELAY_PIN, OUTPUT);
  relay(false);
  Serial.begin(115200);
  Serial.println("Prueba de rele en GPIO 25. Polaridad inicial: activa LOW.");
  Serial.println("Probar primero SIN alarma conectada a COM/NO/NC.");
  Serial.println("Comandos (con salto de linea):");
  Serial.println("ON: prueba de 3 segundos. OFF: terminar prueba.");
  Serial.println("HIGH: cambiar a activo HIGH. LOW: cambiar a activo LOW.");
  Serial.println("STATUS: mostrar configuracion, no estado fisico.");
}

void process(String text) {
  text.trim();
  text.toUpperCase();
  if (text == "ON") {
    relay(true);
    testing = true;
    testStarted = millis();
    Serial.println("Orden de activacion enviada por 3 segundos.");
  } else if (text == "OFF") {
    stopTest();
  } else if (text == "HIGH" || text == "LOW") {
    activeLow = text == "LOW";
    stopTest();
    Serial.print("Nueva polaridad activa: ");
    Serial.println(activeLow ? "LOW" : "HIGH");
  } else if (text == "STATUS") {
    Serial.printf("GPIO=%u | Activo=%s | Orden=%s\n", RELAY_PIN,
                  activeLow ? "LOW" : "HIGH", testing ? "ON" : "OFF");
  } else if (text.length()) {
    Serial.println("Comandos: ON, OFF, LOW, HIGH, STATUS.");
  }
}

void loop() {
  if (testing && millis() - testStarted >= 3000) stopTest();
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      process(command);
      command = "";
    } else if (c != '\r') {
      if (command.length() < 24) command += c;
      else command = "";
    }
  }
}
