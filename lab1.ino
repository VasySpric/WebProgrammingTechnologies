#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

const char* ssid = "Astra ";
const char* password = "12345678";

ESP8266WebServer server(80);

void handleRoot()
{
  File file = LittleFS.open("/index.html", "r");

  if(!file)
  {
    server.send(500, "text/plain", "File error");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}

void setup()
{
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  Serial.println("Connecting...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  LittleFS.begin();

  server.on("/", handleRoot);

  server.begin();

  Serial.println("HTTP server started");
}

void loop()
{
  server.handleClient();
}