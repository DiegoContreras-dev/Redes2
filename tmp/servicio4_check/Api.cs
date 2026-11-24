using System;
using System.IO;
using System.Web;
using System.Web.SessionState;
using System.Web.Script.Serialization;
using System.Collections.Generic;
using System.Configuration;
using System.Globalization;

public class Api : IHttpHandler, IRequiresSessionState {
 static readonly object Gate = new object();
 static Dictionary<string,object> readings;
 static DateTime received = DateTime.MinValue, alarmUntil = DateTime.MinValue;
 public bool IsReusable { get { return false; } }
 JavaScriptSerializer json = new JavaScriptSerializer();
 void Reply(HttpContext c, int code, object value) {
  c.Response.StatusCode=code; c.Response.Write(json.Serialize(value));
 }
 Dictionary<string,object> Body(HttpContext c) {
  if(c.Request.ContentLength>8192) throw new ArgumentException();
  using(var r=new StreamReader(c.Request.InputStream)) {
   var d=json.Deserialize<Dictionary<string,object>>(r.ReadToEnd());
   if(d==null) throw new ArgumentException(); return d;
  }
 }
 bool Device(HttpContext c) {
  string expected=ConfigurationManager.AppSettings["DeviceKey"];
  return !String.IsNullOrEmpty(expected) && c.Request.Headers["X-Device-Key"]==expected;
 }
 bool Operator(HttpContext c) { return c.Session["admin"] as string == "yes"; }
 bool Fresh() { return (DateTime.UtcNow-received).TotalSeconds<90; }
 bool Alarm() { return Fresh() && alarmUntil>DateTime.UtcNow; }
 public void ProcessRequest(HttpContext c) {
  c.Response.ContentType="application/json; charset=utf-8";
  c.Response.Cache.SetCacheability(HttpCacheability.NoCache);
  c.Response.Cache.SetNoStore();
  c.Response.AddHeader("X-Content-Type-Options","nosniff");
  string op=c.Request.QueryString["op"]??"data";
  string method=c.Request.HttpMethod;
  try {
   if(op=="data" && method=="GET") {
    lock(Gate) Reply(c,200,new {datos=readings,ultima=received==DateTime.MinValue?null:received.ToString("o"),
     conectado=Fresh(),alarmaSolicitada=Alarm(),segundosRestantes=Alarm()?(int)Math.Ceiling((alarmUntil-DateTime.UtcNow).TotalSeconds):0});
    return;
   }
   if(op=="login" && method=="POST") {
    var b=Body(c);
    string user=b.ContainsKey("usuario")?Convert.ToString(b["usuario"]):"";
    string pass=b.ContainsKey("clave")?Convert.ToString(b["clave"]):"";
    string expected=ConfigurationManager.AppSettings["AdminPassword"];
    if(String.IsNullOrEmpty(expected)||expected=="CAMBIAR-CLAVE-ADMIN") {
     Reply(c,503,new {error="Configura AdminPassword en web.config."}); return;
    }
    if(user!=ConfigurationManager.AppSettings["AdminUser"]||pass!=expected) {
     Reply(c,401,new {error="Usuario o clave incorrectos."}); return;
    }
    c.Session["admin"]="yes";
    c.Session["csrf"]=Guid.NewGuid().ToString("N");
    Reply(c,200,new {csrf=c.Session["csrf"]});return;
   }
   if(op=="logout" && method=="POST") {
    if(!Operator(c)||c.Request.Headers["X-CSRF-Token"]!=c.Session["csrf"] as string) {
     Reply(c,403,new {error="Sesion no autorizada."});return;
    }
    c.Session.Clear();c.Session.Abandon();Reply(c,200,new {ok=true});return;
   }
   if(op=="alarm" && method=="POST") {
    if(!Operator(c)) {Reply(c,401,new {error="Inicia sesion como administrador."});return;}
    if(c.Request.Headers["X-CSRF-Token"]!=c.Session["csrf"] as string) {
     Reply(c,403,new {error="Sesion invalida. Inicia sesion nuevamente."});return;
    }
    var b=Body(c);
    if(!b.ContainsKey("activa")||!(b["activa"] is bool)) throw new ArgumentException();
    bool on=(bool)b["activa"];
    lock(Gate) {
     if(on&&!Fresh()) {Reply(c,409,new {error="No hay mediciones recientes de la ESP32."});return;}
     alarmUntil=on?DateTime.UtcNow.AddSeconds(60):DateTime.MinValue;
    }
    Reply(c,200,new {ok=true});return;
   }
   if(op=="command" && method=="GET") {
    if(!Device(c)) {Reply(c,403,new {error="Dispositivo no autorizado."});return;}
    c.Response.ContentType="text/plain";
    lock(Gate) c.Response.Write(Alarm()?"1":"0");return;
   }
   if(op=="telemetry" && method=="POST") {
    if(!Device(c)) {Reply(c,403,new {error="Dispositivo no autorizado."});return;}
    var b=Body(c);var clean=new Dictionary<string,object>();
    string[] names={"temperatura","humedad","luz","gas"};
    double[] mins={-40,0,0,0},maxs={80,100,1023,1023};
    for(int i=0;i<names.Length;i++) {
     if(!b.ContainsKey(names[i]))throw new ArgumentException();
     object v=b[names[i]];
     if(v==null) {clean[names[i]]=null;continue;}
     if(v is bool || v is string)throw new ArgumentException();
     double n=Convert.ToDouble(v,CultureInfo.InvariantCulture);
     if(Double.IsNaN(n)||Double.IsInfinity(n)||n<mins[i]||n>maxs[i])throw new ArgumentException();
     clean[names[i]]=n;
    }
    lock(Gate) {readings=clean;received=DateTime.UtcNow;}
    Reply(c,200,new {ok=true});return;
   }
   Reply(c,405,new {error="Ruta o metodo no permitido."});
  } catch(ArgumentException) {Reply(c,400,new {error="Datos invalidos."});}
    catch(InvalidCastException) {Reply(c,400,new {error="Formato invalido."});}
    catch(FormatException) {Reply(c,400,new {error="Formato invalido."});}
    catch(OverflowException) {Reply(c,400,new {error="Valor fuera de rango."});}
 }
}