#include <Arduino.h>
#include <DHT.h>
#include <NimBLEDevice.h>
#include "uuids.h"

const int DHT_PIN = 4;
DHT dhtSensor(DHT_PIN, DHT22);

const int TRIG_PIN = 5;
const int ECHO_PIN = 18;

const int LDR_PIN = 34;
const int MQ5_PIN = 35;

// GPIO2 es el LED integrado de la DevKit V1, asi que la alarma funciona sin
// cablear nada.
const int LED_PIN = 2;

const unsigned long PUBLISH_INTERVAL_MS = 2000;
const unsigned long BLINK_GAS_MS = 150;
const unsigned long BLINK_DIST_MS = 400;
unsigned long lastPublishTime = 0;
unsigned long lastBlinkToggle = 0;
bool blinkOn = false;

// Umbrales por defecto del taller 2. Con una sola ESP32 la alarma se evalua
// aqui mismo; si se suma la controladora, ella tomaria esta tarea.
int lumThreshold = 300;
int gasThreshold = 600;
int distThreshold = 20;
int tempThreshold = 30;   // alerta si la temperatura llega a este valor o mas (C)
int humThreshold = 75;    // alerta si la humedad supera este valor (%)

int lastLuminosidad = 0;
float lastDistanceCm = -1;
int lastGas = 0;
bool hayDatos = false;

String inputBuffer = "";

// Si una lectura del DHT22 falla (la libreria devuelve NaN), como temperatura y
// humedad casi no cambian entre publicaciones, se manda la ultima lectura buena
// en vez de publicar un valor invalido. -1 significa que aun no hay ninguna.
float lastTemperatureC = -1;
float lastHumidityPct = -1;

NimBLECharacteristic *charTemp;
NimBLECharacteristic *charHum;
NimBLECharacteristic *charLuz;
NimBLECharacteristic *charDist;
NimBLECharacteristic *charGas;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *server) override {
    Serial.println("BLE: cliente conectado");
  }

  // El servidor vuelve a anunciarse solo al desconectarse (comportamiento
  // por defecto de NimBLE), asi que aqui solo se deja constancia.
  void onDisconnect(NimBLEServer *server) override {
    Serial.println("BLE: cliente desconectado, anunciando de nuevo");
  }
};

ServerCallbacks serverCallbacks;

float mideDist() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duracion = pulseIn(ECHO_PIN, HIGH, 30000UL);
  if (duracion == 0) return -1;
  return duracion / 58.0;
}

// El ADC de la ESP32 es de 12 bits (0-4095). Se escala a 0-1023 para conservar
// los umbrales del taller 2 y poder comparar con las lecturas del Arduino UNO.
int escalaA10Bits(int lectura12Bits) {
  return lectura12Bits / 4;
}

// AOUT del MQ-5 llega a 5 V y pasa por un divisor 10k/20k, asi que la ESP32
// solo ve dos tercios del valor real. Se compensa el factor 3/2.
int leeGas() {
  int lectura = escalaA10Bits(analogRead(MQ5_PIN));
  return min(lectura * 3 / 2, 1023);
}

void publica(NimBLECharacteristic *caracteristica, const char *texto) {
  caracteristica->setValue(std::string(texto));
  caracteristica->notify();
}

// Cada caracteristica lleva un nombre legible (descriptor 0x2901) y se declara
// como texto UTF-8 (descriptor 0x2904), asi nRF Connect muestra "6.0" en vez
// de bytes en hexadecimal.
NimBLECharacteristic *creaCaracteristica(NimBLEService *servicio, const char *uuid, const char *nombre) {
  NimBLECharacteristic *c =
      servicio->createCharacteristic(uuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  c->createDescriptor("2901", NIMBLE_PROPERTY::READ)->setValue(std::string(nombre));
  NimBLE2904 *formato = (NimBLE2904 *)c->createDescriptor("2904");
  formato->setFormat(NimBLE2904::FORMAT_UTF8);
  return c;
}

void procesarComandoTeclado(const String &linea) {
  if (linea.length() < 2) {
    Serial.println("Comando no reconocido. Usa L, G, D, T o H + valor (p. ej. G700).");
    return;
  }
  char tipo = toupper(linea.charAt(0));
  int valor = linea.substring(1).toInt();

  if (tipo == 'L') {
    lumThreshold = valor;
    Serial.print("Umbral de luminosidad actualizado: "); Serial.println(lumThreshold);
  } else if (tipo == 'G') {
    gasThreshold = valor;
    Serial.print("Umbral de gas actualizado: "); Serial.println(gasThreshold);
  } else if (tipo == 'D') {
    distThreshold = valor;
    Serial.print("Umbral de distancia actualizado: "); Serial.println(distThreshold);
  } else if (tipo == 'T') {
    tempThreshold = valor;
    Serial.print("Umbral de temperatura actualizado: "); Serial.println(tempThreshold);
  } else if (tipo == 'H') {
    humThreshold = valor;
    Serial.print("Umbral de humedad actualizado: "); Serial.println(humThreshold);
  } else {
    Serial.println("Comando no reconocido. Usa L, G, D, T o H + valor (p. ej. G700).");
  }
}

void leeTeclado() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      inputBuffer.trim();
      if (inputBuffer.length() > 0) procesarComandoTeclado(inputBuffer);
      inputBuffer = "";
    } else if (c != '\r') {
      inputBuffer += c;
    }
  }
}

bool alarmaGas() {
  return lastGas > gasThreshold;
}

bool alarmaDistancia() {
  return lastDistanceCm >= 0 && lastDistanceCm < distThreshold;
}

bool alarmaLuz() {
  return lastLuminosidad < lumThreshold;
}

// Prioridad: gas (parpadeo rapido) > distancia (parpadeo lento) > poca luz (fijo).
void actualizaLed() {
  if (!hayDatos) {
    digitalWrite(LED_PIN, LOW);
    return;
  }

  unsigned long now = millis();
  unsigned long periodo = alarmaGas() ? BLINK_GAS_MS : BLINK_DIST_MS;
  if (now - lastBlinkToggle >= periodo) {
    lastBlinkToggle = now;
    blinkOn = !blinkOn;
  }

  if (alarmaGas() || alarmaDistancia()) digitalWrite(LED_PIN, blinkOn);
  else digitalWrite(LED_PIN, alarmaLuz());
}

void imprimeAlertas() {
  if (alarmaGas()) {
    Serial.print("ALARMA: gas alto (> "); Serial.print(gasThreshold); Serial.println(")");
  }
  if (alarmaDistancia()) {
    Serial.print("ALARMA: objeto cercano (< "); Serial.print(distThreshold); Serial.println(" cm)");
  }
  if (alarmaLuz()) {
    Serial.print("ALARMA: poca luz (< "); Serial.print(lumThreshold); Serial.println(")");
  }
  // -1 significa que el DHT22 todavia no ha dado ninguna lectura buena.
  if (lastTemperatureC != -1 && lastTemperatureC >= tempThreshold) {
    Serial.print("ALERTA: temperatura alta (>= "); Serial.print(tempThreshold); Serial.println(" C)");
  }
  if (lastHumidityPct != -1 && lastHumidityPct > humThreshold) {
    Serial.print("ALERTA: humedad alta (> "); Serial.print(humThreshold); Serial.println(" %)");
  }
}

void setup() {
  Serial.begin(115200);
  dhtSensor.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  NimBLEDevice::init(NOMBRE_SENSORA);
  NimBLEServer *servidor = NimBLEDevice::createServer();
  servidor->setCallbacks(&serverCallbacks);

  NimBLEService *servicio = servidor->createService(UUID_SERVICIO);
  charTemp = creaCaracteristica(servicio, UUID_TEMP, "Temperatura (C)");
  charHum = creaCaracteristica(servicio, UUID_HUM, "Humedad (%)");
  charDist = creaCaracteristica(servicio, UUID_DIST, "Distancia (cm)");
  charLuz = creaCaracteristica(servicio, UUID_LUZ, "Luz (0-1023)");
  charGas = creaCaracteristica(servicio, UUID_GAS, "Gas (0-1023)");

  // Valor inicial para que un Read desde el celular nunca devuelva vacio.
  publica(charTemp, "-1");
  publica(charHum, "-1");
  publica(charLuz, "0");
  publica(charDist, "-1");
  publica(charGas, "0");

  servicio->start();

  NimBLEAdvertising *anuncio = NimBLEDevice::getAdvertising();
  anuncio->addServiceUUID(UUID_SERVICIO);
  anuncio->setScanResponse(true);
  anuncio->start();

  Serial.println("Sensora lista, anunciando como " NOMBRE_SENSORA);
  Serial.println("Umbrales por teclado: L, G, D, T o H + valor (p. ej. G700).");
}

void publicaLecturas() {
  float humedad = dhtSensor.readHumidity();
  float temperatura = dhtSensor.readTemperature();
  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println("DHT22: ERROR lectura (NaN), se reutiliza la ultima buena");
  } else {
    lastTemperatureC = temperatura;
    lastHumidityPct = humedad;
  }

  int luminosidad = escalaA10Bits(analogRead(LDR_PIN));
  float distancia = mideDist();
  int gas = leeGas();

  lastLuminosidad = luminosidad;
  lastDistanceCm = distancia;
  lastGas = gas;
  hayDatos = true;

  char buffer[16];

  snprintf(buffer, sizeof(buffer), "%.1f", lastTemperatureC);
  publica(charTemp, buffer);
  snprintf(buffer, sizeof(buffer), "%.1f", lastHumidityPct);
  publica(charHum, buffer);
  snprintf(buffer, sizeof(buffer), "%d", luminosidad);
  publica(charLuz, buffer);
  snprintf(buffer, sizeof(buffer), "%.1f", distancia);
  publica(charDist, buffer);
  snprintf(buffer, sizeof(buffer), "%d", gas);
  publica(charGas, buffer);

  Serial.printf("T=%.1f C  H=%.1f %%  Luz=%d  Dist=%.1f cm  Gas=%d\n",
                lastTemperatureC, lastHumidityPct, luminosidad, distancia, gas);
  imprimeAlertas();
}

void loop() {
  leeTeclado();

  unsigned long now = millis();
  if (now - lastPublishTime >= PUBLISH_INTERVAL_MS) {
    lastPublishTime = now;
    publicaLecturas();
  }

  actualizaLed();
}
