# Redes II — Talleres prácticos

Repositorio del equipo para almacenar los códigos, documentación y recursos desarrollados durante los talleres prácticos del ramo **Redes de Computadores II**.

## Integrantes

- Marianela Díaz
- Diego Contreras
- Víctor Jopia

## Organización del repositorio

```text
.
├── docs/            # Documentación, diagramas y material de apoyo
├── include/         # Archivos de cabecera del proyecto
├── lib/             # Librerías locales
├── src/             # Código fuente de los talleres
│   ├── esp32/       # Código específico para ESP32
│   └── uno/         # Código específico para Arduino Uno
├── taller2/         # Proyecto independiente del taller 2
├── platformio.ini   # Configuración de PlatformIO
└── TODO.md          # Tareas pendientes
```

Cada taller puede incorporar su propio código, documentación y configuración. Cuando sea necesario, se recomienda mantener una estructura independiente dentro de la carpeta correspondiente al taller.

## Requisitos

- [Visual Studio Code](https://code.visualstudio.com/)
- Extensión [PlatformIO IDE](https://platformio.org/install/ide?version=vs)
- Arduino Uno o ESP32, según el taller
- Cable USB y los componentes indicados en la documentación de cada actividad

## Configuración actual

El archivo `platformio.ini` define dos entornos de compilación:

- `uno`: Arduino Uno, monitor serial a `9600` baudios.
- `esp32`: ESP32 Dev Module, monitor serial a `115200` baudios.

El código específico de cada placa se selecciona mediante `build_src_filter`, evitando compilar archivos destinados a la otra plataforma.

## Uso

Desde la interfaz de PlatformIO se puede seleccionar el entorno correspondiente y ejecutar **Build**, **Upload** o **Monitor**.

También es posible utilizar la terminal:

```bash
# Compilar para Arduino Uno
pio run -e uno

# Compilar para ESP32
pio run -e esp32

# Cargar el programa en la placa
pio run -e uno -t upload
# o
pio run -e esp32 -t upload

# Abrir el monitor serial
pio device monitor -e uno
# o
pio device monitor -e esp32
```

Antes de cargar el programa, verifica que la placa esté conectada y que PlatformIO haya detectado correctamente el puerto serial.

## Flujo de trabajo

1. Crear o actualizar la carpeta del taller correspondiente.
2. Documentar conexiones, dependencias y forma de ejecución en `docs/` o en el taller respectivo.
3. Probar el código en la placa objetivo.
4. Registrar cambios mediante commits descriptivos.
5. Revisar los cambios antes de integrarlos a la rama principal.

## Estado

Repositorio en desarrollo. El contenido se irá ampliando a medida que se realicen los talleres prácticos del ramo.
