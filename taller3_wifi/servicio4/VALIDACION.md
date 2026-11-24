# Validacion realizada
- Backend C# compilado con el compilador de .NET Framework del equipo.
- Prueba GET data: HTTP 200.
- POST alarm sin sesion: HTTP 401.
- POST telemetry sin clave valida: HTTP 403.
- GET command sin clave valida: HTTP 403.
- POST logout sin sesion: HTTP 403.
- GET alarm no permitido: HTTP 405.
- web.config XML valido y clave de dispositivo coincide con firmware.
- Copias .ino y .cpp identicas.

No validado: firmware compilado, ejecucion real en IIS, login y comandos autenticados en navegador, compatibilidad de LCD, polaridad del rele y consumo electrico. La prueba de HTTP se realizo con un contexto de prueba de ASP.NET, no con un sitio IIS publicado.
