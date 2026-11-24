# Servicio 4: monitoreo en IIS y alarma remota

Estado: propuesta para revisar antes de implementar.

| Modulo | Responsabilidad | Depende de |
|---|---|---|
| adquisicion | ESP32 lee DHT22 y LDR del servicio 3, agrega gas en GPIO 35 y envia mediciones por HTTP POST cada 30 segundos | API de telemetria |
| api-telemetria | IIS recibe, valida y publica las ultimas mediciones mediante HTTP GET | ASP.NET 4.x en IIS |
| control-alarma | Operador autenticado activa/desactiva alarma; ESP32 consulta la orden y acciona el dispositivo sonoro | API de telemetria, rele en GPIO 25 |
| panel | Consulta publica de datos y acceso separado de operador para los comandos POST | API de telemetria, control de alarma |

Orden: API de telemetria -> adquisicion -> control de alarma -> panel -> prueba integrada.

## Referencias confirmadas
- WiFi en modo estacion, conectado al AP externo del servicio 3.
- DHT22 en GPIO 4, LDR en GPIO 34, MQ-5 en GPIO 35.
- LCD SDA GPIO 21 y SCL GPIO 22.
- Mantener divisores y alimentacion del montaje anterior.
- Actualizacion de lecturas en IIS cada 30 segundos.
- Pagina publica de consulta; activacion manual restringida a operador.

## Propuesta tecnica
- Backend ASP.NET Framework en IIS, para no instalar un servidor Python o Node adicional.
- ESP32 envia POST al PC; el navegador consulta GET al mismo sitio IIS.
- La ESP32 consulta periodicamente el estado solicitado de la alarma: no necesita aceptar conexiones entrantes.
- Clave de dispositivo para aceptar telemetria y credenciales de operador comprobadas por el backend.
- No guardar credenciales de operador en el HTML ni limitarse a ocultar botones.
- Alarma permanece apagada al iniciar y se desactiva si se pierde contacto con el servidor.

## Pendiente confirmar
- Disponibilidad de ASP.NET 4.x en el Windows del servidor.
- Rele: entrada IN conectada a GPIO 25. Confirmar polaridad activa LOW/HIGH y tension nominal de la alarma.
- Sensor de gas: mantener GPIO 35 del taller previo salvo confirmacion de cambio fisico.
- IP y puerto del sitio IIS que se usara.

## Pruebas previstas
- Lecturas llegan y se actualizan cada 30 segundos.
- Visitante sin credenciales puede ver datos pero no activar alarma.
- Operador puede activar y apagar la alarma mediante POST.
- Dispositivo suena fisicamente junto a la ESP32.
- Perdida de WiFi/servidor no deja la alarma encendida indefinidamente.

