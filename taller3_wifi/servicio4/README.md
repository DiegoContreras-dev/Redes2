# Servicio 4 - IIS, ESP32 y rele

## Archivos
- `servicio4/servicio4.ino`: abrir en Arduino IDE.
- `iis/`: copiar COMPLETA al PC servidor (index.html, api.ashx y web.config).
- `prueba_rele/prueba_rele.ino`: prueba independiente de polaridad.
- `../../src/servicio4/main.cpp`: copia para PlatformIO, entorno servicio4.

## Conexiones
Se conservan DHT22 DATA=GPIO4, LDR AO=GPIO34, MQ AO=GPIO35, LCD SDA=21/SCL=22. Rele IN=GPIO25. La bornera COM/NO/NC no va al GPIO. Mantener divisor de tension del MQ (equivalente 10k/20k) y niveles I2C compatibles con 3.3V.

El programa usa inicialmente RELE ACTIVO EN HIGH, porque se observo sonido con activo LOW y orden OFF, pero eso NO confirma la polaridad: tambien podria ser NC. Cambiar `RELAY_ACTIVE_HIGH` segun prueba SIN carga. Alarma positiva a NO, alimentacion positiva a COM, negativa a retorno de su fuente, respetando etiquetas y tension nominal. No asumir que USB soporte alarma+MQ+rele: primero probar la comunicacion SIN alarma conectada. El codigo no puede corregir falta de corriente ni garantizar el estado durante reset. El nivel de reposo durante arranque puede requerir hardware adicional.

## 1. Preparar IIS en el otro PC
1. Activar o desactivar caracteristicas de Windows -> Internet Information Services -> Servicios World Wide Web -> Caracteristicas de desarrollo de aplicaciones -> ASP.NET 4.8 (o 4.x disponible), Extensibilidad .NET y extensiones/filtros ISAPI requeridos. Aceptar dependencias. No basta con IIS de contenido estatico.
2. Copiar los tres archivos de `iis` a una carpeta del PC, por ejemplo C:\inetpub\servicio4.
3. Editar web.config: reemplazar CAMBIAR-CLAVE-ADMIN por una clave propia (sin caracteres XML especiales, o escaparlos). Usuario inicial admin. NO cambiar DeviceKey sin cambiar tambien DEVICE_KEY en el sketch. web.config no se entrega como archivo publico por IIS.
4. IIS -> Sitios -> Agregar sitio: Servicio4, ruta de la carpeta, HTTP puerto 8084, IP todas no asignadas, nombre de host vacio.
5. Grupo de aplicaciones del sitio: .NET CLR v4.0, modo Integrado, un solo proceso trabajador (predeterminado). Autenticacion anonima habilitada: la API gestiona el login. IUSR necesita lectura de la carpeta; el grupo de aplicaciones necesita acceso a su configuracion.
6. Probar http://localhost:8084 y http://localhost:8084/api.ashx?op=data. El segundo debe devolver JSON con datos null, no codigo fuente ni HTML. Si aparece 404.3/404.17, revisar instalacion ASP.NET y grupo de aplicaciones. Si 500.19, revisar web.config y caracteristicas instaladas.
7. CMD -> ipconfig: tomar IPv4 del adaptador conectado al AP. Reservar esa IP en DHCP si es posible.
8. Firewall avanzado: nueva regla de entrada de puerto TCP 8084, para el perfil de red utilizado y preferiblemente restringida a la subred local. No desactivar el firewall.

## 2. ESP32
1. Abrir servicio4/servicio4.ino. Bibliotecas: DHT sensor library, Adafruit Unified Sensor y hd44780. Igual que servicio2.
2. Editar SSID y PASSWORD para el AP externo.
3. Editar SERVER_URL a http://IP_DEL_PC:8084. No localhost ni IP del AP. La clave DEVICE_KEY ya coincide con web.config entregado.
4. Confirmar GAS_PIN=35 y RELAY_ACTIVE_HIGH. El programa usa el DHT22 ya probado. No cambiar a DHT11 salvo que se cambie el sensor real.
5. Subir, monitor serial 115200. Primero conectar con la carga de la alarma desconectada.
6. Debe aparecer POST lecturas: HTTP 200. Hay un primer envio al disponer de datos, despues cada 30 segundos. Errores -1 suelen indicar servidor no alcanzable; 403 indica clave de dispositivo distinta; 404 indica ruta de sitio equivocada.

## 3. Celular y operador
Conectar al mismo AP. Abrir http://IP_DEL_PC:8084 (el sitio se aloja en IIS, no en la ESP32). Los valores publicos se actualizan al llegar cada muestra. El panel refresca su consulta cada 2 segundos.
Abrir Acceso de administrador, usar admin y la clave elegida en web.config. Activar manualmente. GPIO25 recibe la orden en la siguiente consulta, aproximadamente cada segundo mas latencia. Se requiere medicion reciente para activar. La alarma no se enciende automaticamente por gas. OFF puede enviarse aunque no haya datos recientes.
La orden ON vence tras 60 segundos; la ESP32 manda OFF si pierde WiFi o deja de recibir respuesta valida durante 5 segundos (mas latencia de operaciones HTTP). El panel muestra orden solicitada, no confirmacion fisica del contacto o sonido.

## GET y POST
GET api.ashx?op=data: consulta publica.
POST api.ashx?op=telemetry: JSON temperatura/humedad/luz/gas, con cabecera X-Device-Key.
GET api.ashx?op=command: 1/0 con clave de dispositivo.
POST api.ashx?op=login: usuario/clave, crea sesion y token CSRF.
POST api.ashx?op=alarm: activa true/false, requiere sesion y token CSRF.
POST api.ashx?op=logout: requiere sesion y token.

## Limites y pruebas
El estado se guarda en memoria: al reiniciar o reciclar IIS se borran lecturas y orden (reposo OFF). Sesion de administrador vence tras 20 minutos de inactividad. No hay historico persistente. La muestra se considera antigua despues de 90 segundos. Lectura de gas relativa, no ppm. HTTP transmite credenciales sin cifrar: usar solo la LAN del taller; HTTPS es necesario para redes no confiables. No hay bloqueo de intentos de login.

Verificar: visitante ve lecturas pero POST de alarma sin sesion devuelve 401; credenciales incorrectas fallan; ON/OFF con sesion funcionan; POST con clave de dispositivo erronea devuelve 403; desconectar WiFi y comprobar reposo fisico segun polaridad correcta. Probar tambien reset y perdida de alimentacion.

Pruebas pendientes en equipo real: compilacion del sketch, ejecucion de ASP.NET en IIS, consumo de alimentacion y polaridad/estado fisico del rele.
