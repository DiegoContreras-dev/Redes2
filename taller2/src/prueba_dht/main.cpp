#include <Arduino.h>
#include <DHT.h>

#define DHTPIN 7
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();

  Serial.println("=== Prueba sensor DHT22 ===");
  Serial.println("Iniciando sensor...");
  delay(2000);
}

void loop() {
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();

  // Verificar si la lectura fue correcta
  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println("ERROR: No se pudo leer el DHT22.");
  } else {
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println(" °C");

    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.println(" %");

    Serial.println("-------------------------");
  }

  delay(2000);
}
