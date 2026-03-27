#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// ---------------- WIFI ----------------
const char* ssid = "Astra ";
const char* password = "12345678";

// ---------------- SERVER ----------------
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// ---------------- SENSOR ----------------
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
unsigned long lastSend = 0;

// ---------------- ROOT ----------------
void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "index.html not found");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}

// ---------------- WS EVENT ----------------
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.printf("WS client connected: %u\n", num);
      break;

    case WStype_DISCONNECTED:
      Serial.printf("WS client disconnected: %u\n", num);
      break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // I2C для ESP8266:
  // SDA = D2
  // SCL = D1
  Wire.begin(D2, D1);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    while (true) {
      delay(1000);
    }
  }

  if (!lox.begin()) {
    Serial.println("VL53L0X not found");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("VL53L0X OK");

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
  server.begin();

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  Serial.println("HTTP server started");
  Serial.println("WebSocket server started on port 81");
}

void loop() {
  server.handleClient();
  webSocket.loop();

  if (millis() - lastSend >= 500) {
    lastSend = millis();

    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false);

    String message;

    if (measure.RangeStatus != 4) {
      // валідне значення
      message = String(measure.RangeMilliMeter);
      Serial.print("Distance: ");
      Serial.print(message);
      Serial.println(" mm");
    } else {
      message = "OUT";
      Serial.println("Out of range");
    }

    webSocket.broadcastTXT(message);
  }
}