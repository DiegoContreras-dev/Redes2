# Taller WiFi

Se usa taller3_wifi para distinguir este trabajo del taller3 existente de Bluetooth.

## Servicios
1. Access point externo y sitios web IIS.
2. ESP32 como access point y servidor web, sensores y LCD I2C.
3. ESP32 como servidor web conectado a un access point externo.
4. IIS publica mediciones del ESP32 cada 30 segundos, con alarma remota y acceso privilegiado.

## Servicio 1
El archivo servicio1/web/index.html permite probar IIS desde otros equipos de la red.
Configurar un sitio IIS con esta carpeta como ruta física, HTTP, dirección IP no asignada y puerto 8081, sin nombre de host.
Desde otro equipo conectado al mismo AP: http://IP_DEL_PC:8081
Para sitios adicionales, crear otras carpetas y usar puertos diferentes, por ejemplo 8082.
El sitio es una prueba estática; no implementa todavía los servicios con sensores.
