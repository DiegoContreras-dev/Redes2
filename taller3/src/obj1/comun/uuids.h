#pragma once

// UUIDs compartidos por la sensora (servidor) y la controladora (cliente).
// Cada sensor es una característica READ + NOTIFY con texto ASCII (p. ej. "23.5").
// Base propia (no la de Nordic UART) para que el celular no las llame "UART RX/TX".
// Orden: temperatura, humedad, distancia, luz, gas.
#define NOMBRE_SENSORA "Sensora Taller 3"

#define UUID_SERVICIO "3fd90001-0566-4593-9550-f2821670f090"
#define UUID_TEMP     "3fd90002-0566-4593-9550-f2821670f090"
#define UUID_HUM      "3fd90003-0566-4593-9550-f2821670f090"
#define UUID_DIST     "3fd90004-0566-4593-9550-f2821670f090"
#define UUID_LUZ      "3fd90005-0566-4593-9550-f2821670f090"
#define UUID_GAS      "3fd90006-0566-4593-9550-f2821670f090"
