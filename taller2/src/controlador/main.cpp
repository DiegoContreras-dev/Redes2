#include <Arduino.h>
#include <SoftwareSerial.h>

const int SS_RX_PIN = 10;
const int SS_TX_PIN = 11;
SoftwareSerial remoteSerial(SS_RX_PIN, SS_TX_PIN);

int lumThreshold = 300;
int gasThreshold = 600;
int distThreshold = 20;
int tempThreshold = 30;   // alerta si la temperatura llega a este valor o mas (C)
int humThreshold = 75;    // alerta si la humedad supera este valor (%)

String inputBuffer = "";
String remoteBuffer = "";

// Preguntas de umbrales por terminal. No bloquea: cada linea que llega del
// teclado responde la pregunta actual, y mientras tanto se siguen recibiendo
// los reportes de la estacion remota.
enum PasoConfig { PIDE_LUZ, PIDE_GAS, PIDE_DIST, PIDE_TEMP, PIDE_HUM, CONFIG_LISTA };
PasoConfig pasoConfig = PIDE_LUZ;

void enviarUmbrales() {
  remoteSerial.print("TH,");
  remoteSerial.print(gasThreshold);
  remoteSerial.print(",");
  remoteSerial.println(distThreshold);
}

void preguntarUmbral() {
  Serial.println();
  if (pasoConfig == PIDE_LUZ) {
    Serial.print("Umbral de luminosidad 0-1023 (actual ");
    Serial.print(lumThreshold);
  } else if (pasoConfig == PIDE_GAS) {
    Serial.print("Umbral de gas 0-1023 (actual ");
    Serial.print(gasThreshold);
  } else if (pasoConfig == PIDE_DIST) {
    Serial.print("Umbral de distancia en cm (actual ");
    Serial.print(distThreshold);
  } else if (pasoConfig == PIDE_TEMP) {
    Serial.print("Umbral de temperatura en C (actual ");
    Serial.print(tempThreshold);
  } else if (pasoConfig == PIDE_HUM) {
    Serial.print("Umbral de humedad en % (actual ");
    Serial.print(humThreshold);
  }
  Serial.print(", Enter para mantener): ");
}

bool esNumero(const String &texto) {
  if (texto.length() == 0) return false;
  for (unsigned int i = 0; i < texto.length(); i++) {
    if (!isDigit(texto.charAt(i))) return false;
  }
  return true;
}

void responderPregunta(const String &linea) {
  if (linea.length() > 0) {
    int maximo = 1023;
    if (pasoConfig == PIDE_DIST) maximo = 400;
    else if (pasoConfig == PIDE_TEMP) maximo = 80;
    else if (pasoConfig == PIDE_HUM) maximo = 100;
    int valor = linea.toInt();
    if (!esNumero(linea) || valor > maximo) {
      Serial.print("Valor invalido, debe ser un numero entre 0 y ");
      Serial.println(maximo);
      preguntarUmbral();
      return;
    }
    if (pasoConfig == PIDE_LUZ) lumThreshold = valor;
    else if (pasoConfig == PIDE_GAS) gasThreshold = valor;
    else if (pasoConfig == PIDE_DIST) distThreshold = valor;
    else if (pasoConfig == PIDE_TEMP) tempThreshold = valor;
    else if (pasoConfig == PIDE_HUM) humThreshold = valor;
  }

  pasoConfig = (PasoConfig)(pasoConfig + 1);
  if (pasoConfig != CONFIG_LISTA) {
    preguntarUmbral();
    return;
  }

  enviarUmbrales();
  Serial.println();
  Serial.println("Umbrales configurados:");
  Serial.print("  Luminosidad: "); Serial.println(lumThreshold);
  Serial.print("  Gas: "); Serial.println(gasThreshold);
  Serial.print("  Distancia: "); Serial.print(distThreshold); Serial.println(" cm");
  Serial.print("  Temperatura: "); Serial.print(tempThreshold); Serial.println(" C");
  Serial.print("  Humedad: "); Serial.print(humThreshold); Serial.println(" %");
  Serial.println("Escribe U para volver a configurarlos (o L, G, D, T, H + valor).");
  Serial.println("Mostrando datos de la estacion remota cada 10 s...");
}

void procesarComandoTeclado(const String &linea) {
  if (linea == "U" || linea == "u") {
    pasoConfig = PIDE_LUZ;
    preguntarUmbral();
    return;
  }
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

  else if (tipo == 'T' || tipo == 't') {
    tempThreshold = valor;
    Serial.print("Umbral de temperatura actualizado: ");
    Serial.println(tempThreshold);
  }

  else if (tipo == 'H' || tipo == 'h') {
    humThreshold = valor;
    Serial.print("Umbral de humedad actualizado: ");
    Serial.println(humThreshold);
  }

  else {    
    Serial.println("Comando no reconocido. Usa U, L, G, D, T o H + valor.");
  }
}

void procesarReporteRemoto(const String &linea) {
  if (!linea.startsWith("D,")) return;
  // Mientras se preguntan los umbrales no se muestran datos, para no mezclarlos
  // con las preguntas. El reporte se descarta y el LED queda como estaba.
  if (pasoConfig != CONFIG_LISTA) return;

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

  // -1 significa que el DHT22 todavia no ha dado ninguna lectura buena.
  if (temperatura != -1 && temperatura >= tempThreshold) {
    Serial.print("ALERTA: temperatura alta (>= "); Serial.print(tempThreshold); Serial.println(" C)");
  }
  if (humedad != -1 && humedad > humThreshold) {
    Serial.print("ALERTA: humedad alta (> "); Serial.print(humThreshold); Serial.println(" %)");
  }

  int r, g, b;
  if (luminosidad < lumThreshold) {
    r = 255; g = 0; b = 255;   // poca luz: morado
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
  Serial.println("Controlador listo. Configura los umbrales:");
  preguntarUmbral();
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      inputBuffer.trim();
      if (pasoConfig != CONFIG_LISTA) {
        responderPregunta(inputBuffer);
      } else if (inputBuffer.length() > 0) {
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
