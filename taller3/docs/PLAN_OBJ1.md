# Plan: Taller 3, Objetivo I (BLE con sensores en ESP32)

## Context
El enunciado (`taller3/docs/Taller bluetooth 2026.docx`) pide publicar en tiempo real temperatura, humedad, luminosidad, distancia y gas desde una ESP32 con batería.
1. Un celular debe mostrar los datos sin interpretarlos.
2. Una segunda ESP32 (controlador) debe mostrarlos en consola y generar al menos una alarma por valores fuera de rango.
Se entrega además un informe. Solo se hace el Objetivo I; `taller3/src/obj2` queda vacío.

Decisiones del usuario: BLE, nRF Connect/LightBlue como app, 2 ESP32 físicas con sensores.

## Arquitectura
- **Sensora (ESP32 #1, BLE servidor GATT, periférico):** lee los 5 valores y los publica por notificaciones.
- **Celular (nRF Connect):** se conecta y activa las notificaciones; ve los valores crudos.
- **Controlador (ESP32 #2, BLE cliente, central):** escanea por nombre/UUID, se suscribe, imprime por Serial y dispara alarmas.
- Un servicio custom con UUIDs propios y una característica por sensor (READ + NOTIFY), cada una con texto ASCII (p. ej. "23.5"). Así nRF Connect las muestra legibles. Alternativa que se descarta: un solo paquete CSV, menos legible en la app.
- Publicación cada ~2 s. Con batería se usa un intervalo de advertising moderado, y se puede evaluar light sleep entre lecturas.

## Archivos (patrón del taller 2: PlatformIO, un env por placa)
- `taller3/platformio.ini`: envs `sensora` y `controlador`, board `esp32dev`, framework arduino, lib `NimBLE-Arduino` (más liviana que la BLE nativa) y Adafruit DHT como en `taller2/platformio.ini`. `build_src_filter` separa cada env, igual que en el taller 2. Puertos en `platformio_local.ini` (ignorado por git, con `.example`).
- `taller3/src/obj1/sensora/main.cpp`
- `taller3/src/obj1/controlador/main.cpp`
- `taller3/src/obj1/comun/uuids.h`: UUIDs compartidos.
- Reutilizar de `taller2/src/remoto/main.cpp`: `mideDist()` (HC-SR04, `pulseIn` con timeout), la lógica de DHT22 con caché de la última lectura buena (NaN) y la lectura de LDR/MQ-5. De `taller2/src/controlador/main.cpp`: umbrales por defecto (luz 300, gas 600, dist 20 cm, temp 30 °C, hum 75 %).

## Ajustes por pasar de UNO a ESP32
- El ADC de la ESP32 es de 12 bits (0-4095), no de 10. Hay que ajustar los umbrales de LDR y MQ-5 (o escalar a 0-1023 para conservar los del taller 2).
- Los pines ADC deben ser del ADC1 (GPIO32-39), porque ADC2 no funciona con radio activa.
- El eco del HC-SR04 es de 5 V: se necesita divisor de tensión hacia la ESP32 (3,3 V).
- El MQ-5 necesita 5 V para el calentador; con batería hay que verificar la alimentación.
- Pines propuestos (a confirmar con el cableado real): DHT22 GPIO4, TRIG GPIO5, ECHO GPIO18 (con divisor), LDR GPIO34, MQ-5 GPIO35.

## Alarmas del controlador
Al menos una, extendiendo la lógica de umbrales del taller 2: gas alto, objeto cercano, luz baja (y opcionalmente temperatura/humedad). Cada alarma se muestra en consola con una línea de aviso, y se enciende un LED integrado o externo (GPIO2). La alarma de gas tiene prioridad. Se puede conservar el asistente interactivo de umbrales por Serial.

## Orden de trabajo
1. Crear `platformio.ini` y la estructura, y compilar con firmware vacío.
2. Sensora: lectura de sensores y log por Serial.
3. Sensora: servicio BLE y notificaciones, verificado con nRF Connect (cumple la etapa 1).
4. Controlador: cliente BLE y consola.
5. Controlador: alarmas por umbral (cumple la etapa 2).
6. Prueba con batería y borrador de informe (diseño, justificación BLE vs clásico, resultados, capturas).

## Verificación
- `pio run -e sensora` y `pio run -e controlador` compilan.
- nRF Connect: se ve el servicio, las 5 características y las notificaciones con valores que cambian (tapar el LDR, acercar la mano al HC-SR04, etc.).
- Consola del controlador: imprime los 5 valores cada ciclo y muestra la alarma al cruzar cada umbral.
- Reconexión: apagar la sensora y volver a encenderla; el controlador debe reconectar solo.
- Funcionamiento con batería sin USB.

## Riesgos
- No hay hardware en el repo; el cableado y los pines deben confirmarse con el usuario antes de fijar `main.cpp`.
- Wokwi no simula BLE, así que toda la validación es en físico.
