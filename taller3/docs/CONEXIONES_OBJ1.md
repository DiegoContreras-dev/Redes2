# Conexiones y pines, Taller 3 Objetivo I

Diagrama de referencia: `wokwi/taller3/diagram.json` (impórtalo en Wokwi). Wokwi no simula BLE, así que el diagrama sirve para validar el cableado, no el enlace entre las dos ESP32.

Placas: ESP32 DevKit V1 (30 pines). Si la placa real es otra, hay que revisar los números de GPIO.

## Resumen de pines

### ESP32 remota (`esp_sens`, lleva los sensores)

| Función | GPIO | Dirección | Notas |
|---|---|---|---|
| DHT22 (datos) | 4 | Entrada/salida digital | Pin SDA del módulo |
| HC-SR04 TRIG | 5 | Salida digital | Pulso de 10 µs |
| HC-SR04 ECHO | 18 | Entrada digital | Pasa por divisor 1k/2k |
| LDR (AO) | 34 | Entrada analógica (ADC1) | Solo entrada |
| MQ-5 (AOUT) | 35 | Entrada analógica (ADC1) | Pasa por divisor 10k/20k, solo entrada |

Se usan pines del **ADC1** (GPIO32-39) para las lecturas analógicas, porque el ADC2 no funciona con el radio Bluetooth/WiFi activo.

### ESP32 controladora (`esp_ctrl`)

| Función | GPIO | Notas |
|---|---|---|
| LED de alarma | 2 | Con resistencia de 220 Ω a la entrada, cátodo a GND |

## Alimentación (ESP32 remota)

La protoboard tiene dos pares de rieles:

| Riel | Origen | Alimenta |
|---|---|---|
| Superior + (`tp`) | 3V3 de la ESP32 | DHT22, LDR |
| Superior − (`tn`) | GND.1 de la ESP32 | DHT22, LDR |
| Inferior + (`bp`) | VIN (5 V) de la ESP32 | HC-SR04, MQ-5 |
| Inferior − (`bn`) | GND.2 de la ESP32 | HC-SR04, MQ-5, divisores |

Con batería, VIN debe recibir 5 V para alimentar el HC-SR04 y el calentador del MQ-5 (el MQ-5 consume bastante). Si se alimenta por USB, VIN entrega los 5 V del puerto.

## Conexión por sensor

### DHT22 (temperatura y humedad)
| Pin del sensor | Conectar a |
|---|---|
| VCC | Riel 3V3 |
| GND | Riel GND superior |
| SDA | GPIO 4 |

### HC-SR04 (distancia)
| Pin del sensor | Conectar a |
|---|---|
| VCC | Riel 5 V |
| GND | Riel GND inferior |
| TRIG | GPIO 5 |
| ECHO | Divisor de tensión (ver abajo), el punto medio va a GPIO 18 |

### LDR (luminosidad)
| Pin del módulo | Conectar a |
|---|---|
| VCC | Riel 3V3 |
| GND | Riel GND superior |
| AO | GPIO 34 |

### MQ-5 (gas)
| Pin del módulo | Conectar a |
|---|---|
| VCC | Riel 5 V |
| GND | Riel GND inferior |
| AOUT | Divisor de tensión (ver abajo), el punto medio va a GPIO 35 |

## Divisores de tensión

Los pines de la ESP32 trabajan a 3,3 V y no toleran 5 V. Las salidas del HC-SR04 (ECHO) y del MQ-5 (AOUT) llegan a 5 V, por eso cada una pasa por un divisor 1:2. El punto medio entrega 5 V × 2/3 ≈ 3,3 V.

| Señal | R superior (a la señal) | R inferior (a GND) | Punto medio a |
|---|---|---|---|
| ECHO | R1 = 1 kΩ | R2 = 2 kΩ | GPIO 18 |
| AOUT | R1 = 10 kΩ | R2 = 20 kΩ | GPIO 35 |

La ESP32 lee dos tercios del valor real en estas dos señales. En el firmware hay que compensar el factor 3/2 en la lectura de gas (el ECHO es solo un pulso, así que no importa su amplitud). Alternativa: ajustar el umbral de gas.

## Controlador: LED de alarma

GPIO 2 → resistencia 220 Ω → ánodo del LED → cátodo → GND.1 de la ESP32 controladora.

## Notas para el firmware

- El ADC de la ESP32 es de 12 bits (0-4095), a diferencia de los 10 bits (0-1023) del Arduino UNO del taller 2. Los umbrales del taller 2 hay que reescalarlos.
- Los GPIO 34 y 35 son solo de entrada y no tienen pull-up interno. No hace falta configurarlos como salida.
- GPIO 2 es también el LED integrado de muchas DevKit. Puede encenderse al arrancar.
- Los números de hueco de los rieles del diagrama (por ejemplo `tp.25`) son referencias de Wokwi y no importan en el montaje físico.
