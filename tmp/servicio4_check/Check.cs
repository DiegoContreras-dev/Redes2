using System;
using System.IO;
using System.Web;
using System.Web.Hosting;
using System.Web.SessionState;
class Worker : SimpleWorkerRequest {
 string verb;
 public Worker(string query,string method,TextWriter output):base("/",Environment.CurrentDirectory,"api.ashx",query,output){verb=method;}
 public override string GetHttpVerbName(){return verb;}
}
class Check {
 static void Run(string op,string method,int expected){
  var output=new StringWriter();var context=new HttpContext(new Worker("op="+op,method,output));
  var session=new HttpSessionStateContainer("test",new SessionStateItemCollection(),new HttpStaticObjectsCollection(),20,true,HttpCookieMode.UseCookies,SessionStateMode.InProc,false);
  SessionStateUtility.AddHttpSessionStateToContext(context,session);
  new Api().ProcessRequest(context);
  if(context.Response.StatusCode!=expected)throw new Exception(op+": expected "+expected+" got "+context.Response.StatusCode);
  Console.WriteLine(method+" "+op+": "+expected+" OK");
 }
 static void Main(){Run("data","GET",200);Run("alarm","POST",401);Run("telemetry","POST",403);Run("command","GET",403);Run("logout","POST",403);Run("alarm","GET",405);}
}


