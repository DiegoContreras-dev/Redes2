# Servicio 3 - ESP32 en red de infraestructura

Abrir servicio3/servicio3.ino en Arduino IDE. El mismo programa se conserva en src/servicio3/main.cpp para PlatformIO (entorno servicio3).

Bibliotecas: DHT sensor library de Adafruit, Adafruit Unified Sensor y hd44780 de Bill Perry. Placa ESP32 Dev Module.

Conexiones: DHT22 datos GPIO4, LDR AO GPIO34, LCD SDA GPIO21 y SCL GPIO22. LCD asumido 16x2. Conservar alimentacion y adaptacion de niveles del servicio 2.

Red configurada: TallerRedesII, clave TallerII2026. Editar SSID y PASSWORD si el AP tiene otros datos. El AP debe ofrecer 2.4 GHz y permitir comunicacion entre clientes.

Cargar por USB, abrir monitor a 115200 y reiniciar. Al conectar se imprime la IP. Conectar PC o celular al mismo AP y abrir http://IP_DE_LA_ESP32 (puerto 80). No usar localhost ni 192.168.4.1 del servicio 2. IIS no interviene.

El programa no crea una red WiFi propia. Si no hay AP, sigue leyendo sensores y actualizando el LCD mientras reintenta la conexion.

Lecturas y pagina se actualizan aproximadamente cada 2 segundos. No probado en hardware desde este entorno.
