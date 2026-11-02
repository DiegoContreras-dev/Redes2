#include <Arduino.h>
#include <NimBLEDevice.h>
#include "uuids.h"

const int LED_PIN = 2;

const unsigned long REPORT_INTERVAL_MS = 2000;
const unsigned long BLINK_GAS_MS = 150;
const unsigned long BLINK_DIST_MS = 400;
const unsigned long RECONNECT_SCAN_S = 5;

int lumThreshold = 300;
int gasThreshold = 600;
int distThreshold = 20;
int tempThreshold = 30;   // alerta si la temperatura llega a este valor o mas (C)
int humThreshold = 75;    // alerta si la humedad supera este valor (%)

// Ultimos valores recibidos por notificacion. Se escriben desde la tarea de
// NimBLE y se leen desde loop(); cada uno cabe en una palabra de 32 bits, asi
// que la lectura no queda a medias. -1 en temperatura, humedad y distancia
// significa "sin dato".
volatile float temperatura = -1;
volatile float humedad = -1;
volatile float distancia = -1;
volatile int luminosidad = 0;
volatile int gas = 0;
volatile bool hayDatos = false;

volatile bool conectado = false;
bool encontrada = false;
std::string direccionSensora;

NimBLEClient *cliente = nullptr;

unsigned long lastReportTime = 0;
unsigned long lastBlinkToggle = 0;
bool blinkOn = false;

String inputBuffer = "";

class ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
  void onResult(NimBLEAdvertisedDevice *dispositivo) override {
    if (!dispositivo->isAdvertisingService(NimBLEUUID(UUID_SERVICIO))) return;
    direccionSensora = dispositivo->getAddress().toString();
    encontrada = true;
    NimBLEDevice::getScan()->stop();
  }
};

class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient *c) override {
    Serial.println("BLE: conectado a la sensora");
  }

  void onDisconnect(NimBLEClient *c) override {
    conectado = false;
    hayDatos = false;
    Serial.println("BLE: sensora desconectada, buscando de nuevo");
  }
};

ScanCallbacks scanCallbacks;
ClientCallbacks clientCallbacks;

// Cada caracteristica llega como texto ASCII (p. ej. "23.5"). Se copia a un
// buffer con terminador porque los datos de la notificacion no lo traen.
void alRecibir(NimBLERemoteCharacteristic *caracteristica, uint8_t *datos, size_t largo, bool esNotificacion) {
  char texto[16];
  size_t n = min(largo, sizeof(texto) - 1);
  memcpy(texto, datos, n);
  texto[n] = '\0';

  std::string uuid = caracteristica->getUUID().toString();
  if (uuid == UUID_TEMP) temperatura = atof(texto);
  else if (uuid == UUID_HUM) humedad = atof(texto);
  else if (uuid == UUID_LUZ) luminosidad = atoi(texto);
  else if (uuid == UUID_DIST) distancia = atof(texto);
  else if (uuid == UUID_GAS) {
    gas = atoi(texto);
    hayDatos = true;
  }
}

bool suscribe(NimBLERemoteService *servicio, const char *uuid) {
  NimBLERemoteCharacteristic *caracteristica = servicio->getCharacteristic(uuid);
  if (caracteristica == nullptr || !caracteristica->canNotify()) return false;
  return caracteristica->subscribe(true, alRecibir);
}

bool conecta() {
  cliente = NimBLEDevice::createClient();
  cliente->setClientCallbacks(&clientCallbacks, false);
  cliente->setConnectTimeout(5);

  if (!cliente->connect(NimBLEAddress(direccionSensora))) {
    NimBLEDevice::deleteClient(cliente);
    cliente = nullptr;
    return false;
  }

  NimBLERemoteService *servicio = cliente->getService(UUID_SERVICIO);
  bool ok = servicio != nullptr
            && suscribe(servicio, UUID_TEMP)
            && suscribe(servicio, UUID_HUM)
            && suscribe(servicio, UUID_LUZ)
            && suscribe(servicio, UUID_DIST)
            && suscribe(servicio, UUID_GAS);
  if (!ok) {
    Serial.println("BLE: la sensora no expone el servicio esperado");
    cliente->disconnect();
    NimBLEDevice::deleteClient(cliente);
    cliente = nullptr;
    return false;
  }

  conectado = true;
  return true;
}

void buscaYConecta() {
  if (cliente != nullptr) {
    NimBLEDevice::deleteClient(cliente);
    cliente = nullptr;
  }

  encontrada = false;
  NimBLEScan *escaneo = NimBLEDevice::getScan();
  escaneo->clearResults();
  escaneo->start(RECONNECT_SCAN_S, false);

  if (encontrada) conecta();
}

void actualizaLed(bool alarmaGas, bool alarmaDistancia, bool alarmaLuz) {
  if (!conectado || !hayDatos) {
    digitalWrite(LED_PIN, LOW);
    return;
  }

  unsigned long now = millis();
  unsigned long periodo = alarmaGas ? BLINK_GAS_MS : BLINK_DIST_MS;
  if (now - lastBlinkToggle >= periodo) {
    lastBlinkToggle = now;
    blinkOn = !blinkOn;
  }

  // Prioridad: gas (parpadeo rapido) > distancia (parpadeo lento) > poca luz (fijo).
  if (alarmaGas || alarmaDistancia) digitalWrite(LED_PIN, blinkOn);
  else digitalWrite(LED_PIN, alarmaLuz);
}

void imprimeReporte(bool alarmaGas, bool alarmaDistancia, bool alarmaLuz) {
  Serial.println("--- Reporte sensora ---");
  Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" C");
  Serial.print("Humedad: "); Serial.print(humedad); Serial.println(" %");
  Serial.print("Distancia: "); Serial.print(distancia); Serial.println(" cm");
  Serial.print("Luminosidad: "); Serial.println(luminosidad);
  Serial.print("Gas: "); Serial.println(gas);

  if (alarmaGas) {
    Serial.print("ALARMA: gas alto (> "); Serial.print(gasThreshold); Serial.println(")");
  }
  if (alarmaDistancia) {
    Serial.print("ALARMA: objeto cercano (< "); Serial.print(distThreshold); Serial.println(" cm)");
  }
  if (alarmaLuz) {
    Serial.print("ALARMA: poca luz (< "); Serial.print(lumThreshold); Serial.println(")");
  }
  // -1 significa que el DHT22 todavia no ha dado ninguna lectura buena.
  if (temperatura != -1 && temperatura >= tempThreshold) {
    Serial.print("ALERTA: temperatura alta (>= "); Serial.print(tempThreshold); Serial.println(" C)");
  }
  if (humedad != -1 && humedad > humThreshold) {
    Serial.print("ALERTA: humedad alta (> "); Serial.print(humThreshold); Serial.println(" %)");
  }
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

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  NimBLEDevice::init("Controlador-T3");
  NimBLEScan *escaneo = NimBLEDevice::getScan();
  escaneo->setAdvertisedDeviceCallbacks(&scanCallbacks, false);
  escaneo->setActiveScan(true);

  Serial.println("Controlador listo, buscando " NOMBRE_SENSORA);
  Serial.println("Umbrales por teclado: L, G, D, T o H + valor (p. ej. G700).");
}

void loop() {
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

  if (!conectado) {
    actualizaLed(false, false, false);
    buscaYConecta();
    return;
  }

  bool alarmaGas = gas > gasThreshold;
  bool alarmaDistancia = distancia >= 0 && distancia < distThreshold;
  bool alarmaLuz = luminosidad < lumThreshold;
  actualizaLed(alarmaGas, alarmaDistancia, alarmaLuz);

  unsigned long now = millis();
  if (hayDatos && now - lastReportTime >= REPORT_INTERVAL_MS) {
    lastReportTime = now;
    imprimeReporte(alarmaGas, alarmaDistancia, alarmaLuz);
  }
}
