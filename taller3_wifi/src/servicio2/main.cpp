#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <DHT.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

// Se conserva el DHT22 y el GPIO del taller Bluetooth.
constexpr int DHT_PIN = 4;
constexpr int LDR_PIN = 34;
constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;
constexpr int LCD_COLS = 16;
constexpr int LCD_ROWS = 2;
const char *SSID = "TallerWiFi-ESP32";
const char *PASSWORD = "TallerWiFi2026";
DHT dht(DHT_PIN, DHT22);
hd44780_I2Cexp lcd; // Detecta direccion y conexion del adaptador I2C.
WebServer server(80);
bool lcdOk = false;
bool sampled = false;
float temperature = NAN;
float humidity = NAN;
int light = 0;
unsigned long lastSample = 0;

const char PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#183b56">
<title>Estacion ambiental - ESP32</title>
<style>
:root{color-scheme:light;--ink:#183b56;--muted:#52687a}
*{box-sizing:border-box}body{margin:0;background:#eef3f7;color:var(--ink);font-family:Segoe UI,Arial,sans-serif;line-height:1.5}
header{background:var(--ink);color:white;padding:22px max(24px,calc((100vw - 1000px)/2));display:flex;align-items:center;justify-content:space-between;gap:16px}
.brand{font-weight:700;font-size:18px}.badge{font-size:13px;border:1px solid #7593aa;padding:5px 12px;border-radius:30px;white-space:nowrap}
main{max-width:1050px;margin:auto;padding:55px 24px 28px}h1{font-size:clamp(30px,5vw,46px);line-height:1.15;letter-spacing:-1.4px;margin:0 0 14px}
.intro{color:var(--muted);margin:0;max-width:580px}.connection{display:flex;align-items:center;gap:9px;margin:28px 0 22px;font-size:14px}
.dot{width:9px;height:9px;border-radius:50%;background:#8796a2;flex-shrink:0}.online .dot{background:#187554}.offline .dot{background:#ae3b36}
.grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:18px}.card{background:white;border:1px solid #d9e3ea;border-radius:14px;overflow:hidden}
.card header{background:transparent;color:var(--ink);padding:23px 23px 0;justify-content:flex-start;gap:12px}.icon{width:40px;height:40px;display:grid;place-items:center;background:var(--tint);color:var(--accent);border-radius:10px}
svg{width:23px;height:23px;fill:none;stroke:currentColor;stroke-width:1.7;stroke-linecap:round;stroke-linejoin:round}h2{font-size:16px;font-weight:600;margin:0}
.value{font-size:clamp(28px,4vw,42px);font-weight:650;letter-spacing:-1.5px;margin:25px 23px 8px;min-height:63px;font-variant-numeric:tabular-nums;overflow-wrap:anywhere}.note{font-size:13px;color:var(--muted);margin:0 23px 24px}
.bar{height:4px;background:var(--tint)}.bar span{display:block;height:100%;background:var(--accent);width:0}
.details{margin-top:26px;padding:19px 0;border-top:1px solid #d4dfe7;display:flex;justify-content:space-between;gap:12px;flex-wrap:wrap;color:var(--muted);font-size:13px}
footer{margin-top:32px;color:var(--muted);font-size:12px}.offline .value{color:#657582}
@media(max-width:650px){header{padding:18px 20px}.brand{font-size:16px}main{padding:34px 20px}.grid{grid-template-columns:1fr}.value{font-size:40px;margin-top:15px;min-height:54px}.card header{padding-top:19px}.note{margin-bottom:19px}}
</style></head><body>
<header><span class="brand">Estacion ambiental</span><span class="badge">ESP32 / WiFi local</span></header>
<main><h1>Tu entorno, en tiempo real.</h1><p class="intro">Temperatura, humedad y luz medidas desde la estacion. Los datos se actualizan automaticamente cada 2 segundos.</p>
<div class="connection" id="connection"><span class="dot" aria-hidden="true"></span><span id="status" role="status">Conectando con la estacion...</span></div>
<section class="grid" aria-label="Mediciones ambientales">
<article class="card" style="--accent:#b74d35;--tint:#faeee8"><header><span class="icon" aria-hidden="true"><svg viewBox="0 0 24 24"><path d="M9 14V5a3 3 0 0 1 6 0v9a5 5 0 1 1-6 0Z"/><path d="M12 8v10"/></svg></span><h2>Temperatura</h2></header><p class="value" id="t">—</p><p class="note">Temperatura del ambiente</p></article>
<article class="card" style="--accent:#226ca4;--tint:#e9f2fa"><header><span class="icon" aria-hidden="true"><svg viewBox="0 0 24 24"><path d="M12 3S5 11 5 15a7 7 0 0 0 14 0c0-4-7-12-7-12Z"/><path d="M8 15a4 4 0 0 0 4 4"/></svg></span><h2>Humedad relativa</h2></header><p class="value" id="h">—</p><p class="note">Humedad del aire</p></article>
<article class="card" style="--accent:#9b700b;--tint:#fbf3db"><header><span class="icon" aria-hidden="true"><svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="4"/><path d="M12 2v2m0 16v2M2 12h2m16 0h2M5 5l1.5 1.5m11 11L19 19M5 19l1.5-1.5m11-11L19 5"/></svg></span><h2>Luminosidad</h2></header><p class="value" id="l">—</p><p class="note">Nivel relativo / escala de 0 a 1023</p><div class="bar"><span id="lightbar"></span></div></article>
</section><div class="details"><span>Actualizacion: <strong id="updated">Esperando datos</strong></span><span>Servicio 2 · Redes de Computadores II</span></div>
<footer>Conexion directa con la ESP32. No requiere acceso a Internet.</footer></main>
<script>
async function refresh(){
 try{
  const r=await fetch('/datos',{cache:'no-store'});
  if(!r.ok)throw new Error();
  const d=await r.json();
  document.body.classList.remove('offline');
  document.getElementById('connection').className='connection online';
  document.getElementById('t').textContent=d.temperatura===null?'Sin lectura valida':d.temperatura+' °C';
  document.getElementById('h').textContent=d.humedad===null?'Sin lectura valida':d.humedad+' %';
  document.getElementById('l').textContent=d.luz===null?'Esperando...':d.luz;
  document.getElementById('lightbar').style.width=d.luz===null?'0%':Math.max(0,Math.min(100,d.luz/1023*100))+'%';
  document.getElementById('updated').textContent=new Date().toLocaleTimeString('es-CL');
  document.getElementById('status').textContent='Estacion conectada';
 }catch(e){document.body.classList.add('offline');document.getElementById('connection').className='connection offline';document.getElementById('status').textContent='Sin conexion. Se conservan los ultimos valores recibidos.';}
 setTimeout(refresh,2000);
}refresh();
</script></body></html>)HTML";

void lcdLine(int row, const String &text) {
  lcd.setCursor(0, row);
  String line = text.substring(0, LCD_COLS);
  while (line.length() < LCD_COLS) line += ' ';
  lcd.print(line);
}

void sampleSensors() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  light = analogRead(LDR_PIN) / 4;
  sampled = true;
  Serial.printf("Temperatura: %.1f C | Humedad: %.1f %% | Luz: %d\n",
                temperature, humidity, light);
  if (lcdOk) {
    String t = isnan(temperature) ? "ERR" : String(temperature, 1);
    String h = isnan(humidity) ? "ERR" : String(humidity, 1);
    lcdLine(0, "T:" + t + "C H:" + h + "%");
    lcdLine(1, "Luz:" + String(light) + "/1023");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(SDA_PIN, SCL_PIN);
  lcdOk = lcd.begin(LCD_COLS, LCD_ROWS) == 0;
  if (lcdOk) {
    lcd.backlight();
    lcdLine(0, "Servicio 2 WiFi");
    lcdLine(1, "Esperando datos");
  } else Serial.println("LCD no detectado: revisar I2C y alimentacion.");

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(SSID, PASSWORD)) {
    Serial.println("ERROR: no se pudo iniciar el access point.");
    return;
  }
  Serial.printf("SSID: %s\nClave: %s\n", SSID, PASSWORD);
  Serial.print("Abrir http://");
  Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html; charset=utf-8", PAGE);
  });
  server.on("/datos", HTTP_GET, []() {
    String t = isnan(temperature) ? "null" : String(temperature, 1);
    String h = isnan(humidity) ? "null" : String(humidity, 1);
    String l = sampled ? String(light) : "null";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", "{\"temperatura\":" + t +
                ",\"humedad\":" + h + ",\"luz\":" + l + "}");
  });
  server.onNotFound([]() { server.send(404, "text/plain", "No encontrado"); });
  server.begin();
  lastSample = millis();
}

void loop() {
  server.handleClient();
  unsigned long now = millis();
  if (now - lastSample >= 2000) {
    lastSample = now;
    sampleSensors();
  }
}
