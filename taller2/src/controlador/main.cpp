#include <Arduino.h>
#include <SoftwareSerial.h>

const int SS_RX_PIN = 10;
const int SS_TX_PIN = 11;
SoftwareSerial remoteSerial(SS_RX_PIN, SS_TX_PIN);

int lumThreshold = 300;
int gasThreshold = 600;
int distThreshold = 20;

String inputBuffer = "";
String remoteBuffer = "";

void enviarUmbrales() {
  remoteSerial.print("TH,");
  remoteSerial.print(gasThreshold);
  remoteSerial.print(",");
  remoteSerial.println(distThreshold);
}

void procesarComandoTeclado(const String &linea) {
  if (linea.length() < 2) return;
  char tipo = linea.charAt(0);
  int valor = linea.substring(1).toInt();

  if (tipo == 'L' || tipo == 'l') {    
    lumThreshold = valor;
    Serial.print("Umbral de luminosidad actualizado: ");
    Serial.println(lumThreshold);

  } 
  
  else if (tipo == 'G' || tipo == 'g') {    
    gasThreshold = valor;
    enviarUmbrales();
    Serial.print("Umbral de gas actualizado: ");
    Serial.println(gasThreshold);
  } 
  
  else if (tipo == 'D' || tipo == 'd') {
    distThreshold = valor;
    enviarUmbrales();
    Serial.print("Umbral de distancia actualizado: ");
    Serial.println(distThreshold);
  } 

  else {    
    Serial.println("Comando no reconocido. Usa L<valor>, G<valor> o D<valor>.");
  }
}

void procesarReporteRemoto(const String &linea) {
  if (!linea.startsWith("D,")) return;

  String partes[5];
  int campo = 0;
  int start = 2;
  for (unsigned int i = 2; i <= linea.length() && campo < 5; i++) {
    if (i == linea.length() || linea.charAt(i) == ',') {
      partes[campo] = linea.substring(start, i);
      campo++;
      start = i + 1;
    }
  }
  if (campo < 5) return;

  float temperatura = partes[0].toFloat();
  float humedad = partes[1].toFloat();
  float distancia = partes[2].toFloat();
  int luminosidad = partes[3].toInt();
  int gas = partes[4].toInt();

  Serial.println("--- Reporte estacion remota ---");
  Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" C");
  Serial.print("Humedad: "); Serial.print(humedad); Serial.println(" %");
  Serial.print("Distancia: "); Serial.print(distancia); Serial.println(" cm");
  Serial.print("Luminosidad: "); Serial.println(luminosidad);
  Serial.print("Gas: "); Serial.println(gas);

  int r, g, b;
  if (luminosidad < lumThreshold) {
    r = 255; g = 255; b = 255;
  } else {
    r = 0; g = 0; b = 0;
  }

  remoteSerial.print(r);
  remoteSerial.print(",");
  remoteSerial.print(g);
  remoteSerial.print(",");
  remoteSerial.println(b);
}

void setup() {
  Serial.begin(9600);
  remoteSerial.begin(9600);
  Serial.println("Controlador listo. Comandos: L<val> luminosidad, G<val> gas, D<val> distancia.");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      inputBuffer.trim();
      if (inputBuffer.length() > 0) {
        procesarComandoTeclado(inputBuffer);
      }
      inputBuffer = "";
    } else if (c != '\r') {
      inputBuffer += c;
    }
  }

  while (remoteSerial.available()) {
    char c = remoteSerial.read();
    
    if (c == '\n') {
      remoteBuffer.trim();
      
      if (remoteBuffer.length() > 0) {
        procesarReporteRemoto(remoteBuffer);
      }

      remoteBuffer = "";
    } 
    
    else if (c != '\r') {
      remoteBuffer += c;
    }
  }
}
