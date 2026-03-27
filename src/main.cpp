#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

const char* ssid = "Astra ";
const char* password = "12345678";

ESP8266WebServer server(80);

const uint8_t LED_PIN = LED_BUILTIN;   // на ESP8266 вбудований LED зазвичай active LOW
bool ledState = false;

void applyLedState(bool on) {
  ledState = on;
  digitalWrite(LED_PIN, on ? LOW : HIGH);  // LOW = увімкнено, HIGH = вимкнено
}

void sendStatusJson(int code = 200) {
  StaticJsonDocument<64> doc;
  doc["led_on"] = ledState;

  String response;
  serializeJson(doc, response);
  server.send(code, "application/json", response);
}

void handleRoot() {
  File file = LittleFS.open("index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "index.html not found in LittleFS");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}

void handleStatus() {
  Serial.println("GET /api/status");
  sendStatusJson(200);
}

void handleControl() {
  Serial.println("POST /api/control");

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"no body\"}");
    return;
  }

  String body = server.arg("plain");
  Serial.print("Body: ");
  Serial.println(body);

  StaticJsonDocument<128> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "application/json", "{\"error\":\"bad json\"}");
    return;
  }

  if (!doc.containsKey("led_on")) {
    server.send(400, "application/json", "{\"error\":\"no led_on\"}");
    return;
  }

  bool desiredState = doc["led_on"].as<bool>();
  applyLedState(desiredState);

  sendStatusJson(200);
}

void handleNotFound() {
  Serial.print("404: ");
  Serial.println(server.uri());
  server.send(404, "application/json", "{\"error\":\"not found\"}");
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(LED_PIN, OUTPUT);
  applyLedState(false);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    while (true) {
      delay(1000);
    }
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/index.html", HTTP_GET, handleRoot);

  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/control", HTTP_POST, handleControl);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}