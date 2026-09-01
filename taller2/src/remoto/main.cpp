#include <Arduino.h>
#include <SoftwareSerial.h>
#include <dht.h>

const int SS_RX_PIN = 10;
const int SS_TX_PIN = 11;
SoftwareSerial controlSerial(SS_RX_PIN, SS_TX_PIN);

const int TRIG_PIN = 8;
const int ECHO_PIN = 12;

const int DHT_PIN = 7;
dht DHT11Sensor;

const int LED_R_PIN = 6;
const int LED_G_PIN = 5;
const int LED_B_PIN = 3;

const int LDR_PIN = A0;
const int MQ5_PIN = A1;

const unsigned long REPORT_INTERVAL_MS = 10000;
const unsigned long ALARM_CHECK_INTERVAL_MS = 150;
const unsigned long BLINK_INTERVAL_MS = 300;

unsigned long lastReportTime = 0;
unsigned long lastAlarmCheckTime = 0;
unsigned long lastBlinkToggle = 0;
bool blinkOn = false;

float lastDistanceCm = -1;
int lastGasRaw = 0;

int gasThreshold = 600;
int distThreshold = 20;

int commandR = 0;
int commandG = 0;
int commandB = 0;

String remoteBuffer = "";

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

void aplicarColorLed(int r, int g, int b) {
  analogWrite(LED_R_PIN, r);
  analogWrite(LED_G_PIN, g);
  analogWrite(LED_B_PIN, b);
}

void apagarLed() {
  aplicarColorLed(0, 0, 0);
}

bool esColorAlarma(int r, int g, int b, int rEsperado, int gEsperado, int bEsperado) {
  return r == rEsperado && g == gEsperado && b == bEsperado;
}

void actualizarLed(bool alarmaGas, bool alarmaDistancia) {
  unsigned long now = millis();
  if (now - lastBlinkToggle >= BLINK_INTERVAL_MS) {
    lastBlinkToggle = now;
    blinkOn = !blinkOn;
  }

  if (alarmaGas) {
    blinkOn ? aplicarColorLed(255, 0, 0) : apagarLed();
  } 
  
  else if (alarmaDistancia) {
    blinkOn ? aplicarColorLed(255, 255, 0) : apagarLed();
  } 
  
  else if (esColorAlarma(commandR, commandG, commandB, 255, 0, 0) || esColorAlarma(commandR, commandG, commandB, 255, 255, 0)) {
    blinkOn ? aplicarColorLed(commandR, commandG, commandB) : apagarLed();
  } 
  
  else {
    aplicarColorLed(commandR, commandG, commandB);
  }
}

void procesarMensaje(const String &linea) {
  if (linea.startsWith("TH,")) {
    int p1 = linea.indexOf(',', 3);
    int p2 = linea.indexOf(',', p1 + 1);
    if (p1 > 0 && p2 > 0) {
      gasThreshold = linea.substring(p1 + 1, p2).toInt();
      distThreshold = linea.substring(p2 + 1).toInt();
    }
    return;
  }

  int p1 = linea.indexOf(',');
  int p2 = linea.indexOf(',', p1 + 1);
  if (p1 > 0 && p2 > 0) {
    commandR = linea.substring(0, p1).toInt();
    commandG = linea.substring(p1 + 1, p2).toInt();
    commandB = linea.substring(p2 + 1).toInt();
  }
}

void setup() {
  Serial.begin(9600);
  controlSerial.begin(9600);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
}

void loop() {
  while (controlSerial.available()) {
    char c = controlSerial.read();
    
    if (c == '\n') {
      remoteBuffer.trim();
      
      if (remoteBuffer.length() > 0) {
        procesarMensaje(remoteBuffer);
      }

      remoteBuffer = "";
    }

    else if (c != '\r') {
      remoteBuffer += c;
    }
  }

  unsigned long now = millis();

  if (now - lastAlarmCheckTime >= ALARM_CHECK_INTERVAL_MS) {
    lastAlarmCheckTime = now;
    lastGasRaw = analogRead(MQ5_PIN);
    float distancia = mideDist();
    
    if (distancia >= 0) {
      lastDistanceCm = distancia;
    }
  }

  bool alarmaGas = lastGasRaw > gasThreshold;
  bool alarmaDistancia = (lastDistanceCm >= 0) && (lastDistanceCm < distThreshold);
  actualizarLed(alarmaGas, alarmaDistancia);

  if (now - lastReportTime >= REPORT_INTERVAL_MS) {
    lastReportTime = now;

    bool dhtOk = DHT11Sensor.read11(DHT_PIN) == DHTLIB_OK;
    int luminosidad = analogRead(LDR_PIN);

    controlSerial.print("D,");
    controlSerial.print(dhtOk ? DHT11Sensor.temperature : -1);
    controlSerial.print(",");
    controlSerial.print(dhtOk ? DHT11Sensor.humidity : -1);
    controlSerial.print(",");
    controlSerial.print(lastDistanceCm);
    controlSerial.print(",");
    controlSerial.print(luminosidad);
    controlSerial.print(",");
    controlSerial.println(lastGasRaw);
  }
}
