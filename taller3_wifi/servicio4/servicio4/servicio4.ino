#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <DHT.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

const char *SSID = "TallerRedesII";
const char *PASSWORD = "TallerII2026";
// CAMBIAR por IP del PC IIS, no la IP del AP ni de la ESP32.
const char *SERVER_URL = "http://10.0.0.105:8084";
const char *DEVICE_KEY = "9f2aeecdbffff4775bff9d65428954575b7ceb2ded014d5b";
constexpr int DHT_PIN=4, LDR_PIN=34, GAS_PIN=35, RELAY_PIN=25;
constexpr int SDA_PIN=21, SCL_PIN=22;
// Cambiar a false si el modulo realmente se activa con LOW.
// Probar sin carga primero: la polaridad aun no esta confirmada.
constexpr bool RELAY_ACTIVE_HIGH=true;
DHT dht(DHT_PIN,DHT22);
hd44780_I2Cexp lcd;
bool lcdOk=false,relayRequested=false,haveSample=false;
float temperature=NAN,humidity=NAN;
int light=0,gas=0;
unsigned long lastSample=0,lastPost=0,lastPoll=0,lastContact=0,lastRetry=0;
bool posted=false,wasConnected=false,alarmContactValid=false;

void setRelay(bool on) {
 digitalWrite(RELAY_PIN,on?(RELAY_ACTIVE_HIGH?HIGH:LOW):(RELAY_ACTIVE_HIGH?LOW:HIGH));
 if(on!=relayRequested) Serial.println(on?"Orden rele: ON":"Orden rele: OFF");
 relayRequested=on;
}
void line(int row,String s) {
 lcd.setCursor(0,row);s=s.substring(0,16);while(s.length()<16)s+=' ';lcd.print(s);
}
void sampleSensors() {
 temperature=dht.readTemperature();humidity=dht.readHumidity();
 light=analogRead(LDR_PIN)/4;
 // MQ en GPIO35 con divisor equivalente 10k/20k del taller anterior.
 gas=min((analogRead(GAS_PIN)/4)*3/2,1023);
 haveSample=true;
 Serial.printf("T=%.1f H=%.1f Luz=%d Gas=%d\n",temperature,humidity,light,gas);
 if(lcdOk) {
  line(0,"T:"+(isnan(temperature)?String("ERR"):String(temperature,1))+" H:"+(isnan(humidity)?String("ERR"):String(humidity,1)));
  line(1,"L:"+String(light)+" G:"+String(gas));
 }
}
String api(const char *op) {return String(SERVER_URL)+"/api.ashx?op="+op;}
void postReadings() {
 HTTPClient http;http.setConnectTimeout(1000);http.setTimeout(1000);
 if(!http.begin(api("telemetry")))return;
 http.addHeader("Content-Type","application/json");http.addHeader("X-Device-Key",DEVICE_KEY);
 String t=isnan(temperature)?"null":String(temperature,1);
 String h=isnan(humidity)?"null":String(humidity,1);
 String body="{\"temperatura\":"+t+",\"humedad\":"+h+",\"luz\":"+String(light)+",\"gas\":"+String(gas)+"}";
 int code=http.POST(body);Serial.printf("POST lecturas: HTTP %d\n",code);
 if(code>0&&code!=200)Serial.println(http.getString());http.end();
}
void pollCommand() {
 HTTPClient http;http.setConnectTimeout(1000);http.setTimeout(1000);
 if(!http.begin(api("command")))return;
 http.addHeader("X-Device-Key",DEVICE_KEY);
 int code=http.GET();
 if(code==200) {
  String s=http.getString();s.trim();
  if(s=="0"||s=="1") {
   lastContact=millis();alarmContactValid=true;setRelay(s=="1");
  }else setRelay(false);
 }else {Serial.printf("GET orden: HTTP %d\n",code);setRelay(false);}
 http.end();
}
void setup() {
 digitalWrite(RELAY_PIN,RELAY_ACTIVE_HIGH?LOW:HIGH);pinMode(RELAY_PIN,OUTPUT);setRelay(false);
 Serial.begin(115200);dht.begin();Wire.begin(SDA_PIN,SCL_PIN);
 lcdOk=lcd.begin(16,2)==0;
 if(lcdOk){lcd.backlight();line(0,"Servicio 4 IIS");line(1,"Esperando datos");}
 else Serial.println("LCD no detectado");
 WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(true);WiFi.begin(SSID,PASSWORD);
 Serial.print("Servidor IIS: ");Serial.println(SERVER_URL);
 Serial.println("Rele GPIO25; polaridad inicial activa HIGH. Verificar SIN carga.");
 lastSample=millis();
}
void loop() {
 unsigned long now=millis();bool connected=WiFi.status()==WL_CONNECTED;
 if(!connected){setRelay(false);alarmContactValid=false;}
 if(alarmContactValid&&now-lastContact>=5000){setRelay(false);alarmContactValid=false;}
 if(connected&&!wasConnected){Serial.print("ESP32 conectada: ");Serial.println(WiFi.localIP());posted=false;}
 wasConnected=connected;
 if(!connected&&now-lastRetry>=10000){lastRetry=now;WiFi.reconnect();}
 if(now-lastSample>=2000){lastSample=now;sampleSensors();}
 if(connected&&now-lastPoll>=1000){lastPoll=now;pollCommand();}
 // Recalcular despues de llamadas HTTP para que el tiempo de espera no quede obsoleto.
 now=millis();
 if(connected&&haveSample&&(!posted||now-lastPost>=30000)){
  lastPost=now;posted=true;postReadings();
 }
 if(alarmContactValid&&millis()-lastContact>=5000){setRelay(false);alarmContactValid=false;}
}
