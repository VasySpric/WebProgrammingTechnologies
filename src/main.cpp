#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServerSecure.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// ================= WIFI =================
const char* WIFI_SSID = "Astra ";
const char* WIFI_PASS = "12345678";

// ================= BASIC AUTH =================
const char* AUTH_USER = "admin";
const char* AUTH_PASS = "1234";

// ================= LED =================
const uint8_t LED_PIN = 2;   // для більшості ESP8266 dev board це ок
bool ledState = false;

// ================= HTTPS SERVER =================
BearSSL::ESP8266WebServerSecure server(443);

// ================= CERT / KEY =================
static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDBTCCAe2gAwIBAgIURg575/i3Ot0B/gzHP3ERf18ydNgwDQYJKoZIhvcNAQEL
BQAwEjEQMA4GA1UEAwwHRVNQODI2NjAeFw0yNjAzMjUyMTU3MzdaFw0yNzAzMjUy
MTU3MzdaMBIxEDAOBgNVBAMMB0VTUDgyNjYwggEiMA0GCSqGSIb3DQEBAQUAA4IB
DwAwggEKAoIBAQC4ooAPhZS4Ahm19TOpwSEFD3FCW3E1qP4NAHbemEEzac9rGul3
C7J0TQyFrvK4OF6cTTQzuzkJQ2zVZFGCgwL+xjo0xjeaK+JGOwGx5g9e64Hbi6Ew
FCIK3lOIzxHFgI5ht4BEET9kYytJ+gU4ZWGOJa9WlM8VuWm0QUcEfYX7XogvCEZ9
03TVCTAc1qiUEZSkx+AUVcY3eL515QImnVrIswPYuHvdFd/gwHUv2hh5WN6HqNgT
DCbxCklcIdHlLjTmItvhZovPnpkbhCMN35t0rYwetmLcj4GuJQB3vlE1lcoIC+83
F/kU4xfeiz0pEKxPWx4HPcecHZW9EVNX6tlnAgMBAAGjUzBRMB0GA1UdDgQWBBR+
WyTw3nw8WKSESsOe0m3aBP47ZzAfBgNVHSMEGDAWgBR+WyTw3nw8WKSESsOe0m3a
BP47ZzAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQAVgMWCN1t3
KLHAsCLC5mgQWy7VjR0RwzguXlyLR2xdHbTRo8CevRyqJtFT9+xxR+OtQj4hwRM6
ZdR2V57ZAvbRWdRJSnVh+KgPem07nlVu2suguy9zMGf51MNiugQJBgFBUgXh+nP0
exKkOUXCXZgvg7JjxjnM7bGXOWTxw7DaF2pp/blZk8/NGmYHuVU6z4pbxR4pXq3M
XPW4YzREJwluvS8DXVESN2fUEvWWLEmYbLoj2tFN6vHiK1twqXqlr+JNq1VY9KQK
07YTJo985IFq0kg9DuVAMR1BTTueeEcZPwOtf40ea6yGjKIet49P3ZvFvtJhnL3S
8IDre6th1i7P
-----END CERTIFICATE-----
)EOF";

static const char serverKey[] PROGMEM = R"EOF(
-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC4ooAPhZS4Ahm1
9TOpwSEFD3FCW3E1qP4NAHbemEEzac9rGul3C7J0TQyFrvK4OF6cTTQzuzkJQ2zV
ZFGCgwL+xjo0xjeaK+JGOwGx5g9e64Hbi6EwFCIK3lOIzxHFgI5ht4BEET9kYytJ
+gU4ZWGOJa9WlM8VuWm0QUcEfYX7XogvCEZ903TVCTAc1qiUEZSkx+AUVcY3eL51
5QImnVrIswPYuHvdFd/gwHUv2hh5WN6HqNgTDCbxCklcIdHlLjTmItvhZovPnpkb
hCMN35t0rYwetmLcj4GuJQB3vlE1lcoIC+83F/kU4xfeiz0pEKxPWx4HPcecHZW9
EVNX6tlnAgMBAAECggEARAYuu0lCWScA9P6BicXrVfNyDXomNBhufTjSEsGyp/AQ
aMwR/veyyFGIzwn9R+0QwnJZTMPqSCTsyJP8yeJd7KL41++9lZIcfDaKMdJqkFSh
U9NWUCHixvFRgoKT8Bmkm5Dhc3KNUf2NeeRhHv2PGzF8L6CEyWK19KDmOtvKTKlP
ffoNM9dJSXkGEGby360e8ZsBaR9K/u/Wsu3RtbR6LqyNTOMj5tmtCJgXLK9+uvbm
AYDA5QFeJAN5XhSQQAg+td9YEF2q8g6w9Pfj+sl+WCufQpzfoQ1prsgjULEe3aHH
Rgwi9BHOcOabTep1ABwXQ3iGJUQr1BZQABF5ura+GQKBgQDlzqRPXsRMV8Hqy5fL
GLlUMfMVaRLmMoYlEp9jODFI3lVLO1xwpKojYhPs5kcUTdvjjn28N16jeMTRXLeR
RByjczwaqAjaNbcyiX6Oj4WBwoRxnF/Qwtk8/PBYi94OPz6uc6hg5XCsnZfznqtW
hk9vECFCezZOlpPBMTsTFgJ9eQKBgQDNrc8FY1KkGZH66RvXsQu/ulQ3YRWycUjd
NABCLm6dodaFDlAPYGrjrE+FIrRBc7374tu0DCC/11C1Yo2tRVY4qyFt2DwrxoMT
TYbUO7X+xPmQLAXPSvx0IsZr/5zw6xWUpnIaNx5HUlagioSZba8BJYydAtH/oy8P
8SzAtBG13wKBgC698ym5qs+kYerx8jP7GWcIqdrG/nMX/7T2rritq0iZAFxG/Kxk
sb50qnza8cYtd/Y2+1gXIwa4/79dznCbm/+a5rS9TN+lYVfEI5u3kX4tJGc/WTXL
DAidof1Apa/cKVGucKcUw/A4Nbn1of/9XZbHfxxYl1kww2jeLJfbODJxAoGABgj6
kRrn+t6xNN6QLZUJVjfebr2PiGvginUTKN356vArvj28Rac7m7CbqLlq9pyxz2lJ
rw/ICjkEKj5ZD+N/8yu6UGHO6i3p/LACq8mQvogDFqLkDMAThkja76JRdZRATttP
b8t8PPWCizVlTKc2Ql9ar+S8Srb8E4itGn5uIpsCgYEAhssrlLeAlVa+vJZp4UkN
i/ayLrQ+Yq5YYuxhZxJoEXfX6P39qgSRziegUXvQw4bER8MecKvGwkhRCuri9ePA
x/rbydYRDGLKjUIfmetxKJFELoRUj4vDZ4trQ9Sym7NgW+/Rouj1ZleL+tPgLCZR
UhCy/9RluPNwWnJD5Bcnxhw=
-----END PRIVATE KEY-----
)EOF";

BearSSL::X509List cert(serverCert);
BearSSL::PrivateKey key(serverKey);

// ================= HELPERS =================
void applyLed(bool on) {
  ledState = on;
  digitalWrite(LED_PIN, on ? LOW : HIGH);   // ESP8266 LED active LOW
}

bool authRequired() {
  if (!server.authenticate(AUTH_USER, AUTH_PASS)) {
    server.requestAuthentication(BASIC_AUTH, "ESP8266 Secure");
    return false;
  }
  return true;
}

void sendJsonStatus() {
  JsonDocument doc;
  doc["led_on"] = ledState;

  String out;
  serializeJson(doc, out);

  server.sendHeader("Connection", "close");
  server.send(200, "application/json", out);
}

// ================= HANDLERS =================
void handleRoot() {
  if (!authRequired()) return;

  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    server.sendHeader("Connection", "close");
    server.send(500, "text/plain", "index.html not found");
    return;
  }

  server.sendHeader("Connection", "close");
  server.streamFile(file, "text/html");
  file.close();
}

void handleStatus() {
  // без auth на API, щоб ESP8266 не задихався від HTTPS+auth на кожен fetch
  sendJsonStatus();
}

void handleControl() {
  if (!server.hasArg("plain")) {
    server.sendHeader("Connection", "close");
    server.send(400, "application/json", "{\"error\":\"no body\"}");
    return;
  }

  String body = server.arg("plain");

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    server.sendHeader("Connection", "close");
    server.send(400, "application/json", "{\"error\":\"bad json\"}");
    return;
  }

  if (!doc["led_on"].is<bool>()) {
    server.sendHeader("Connection", "close");
    server.send(400, "application/json", "{\"error\":\"no led_on\"}");
    return;
  }

  bool desiredState = doc["led_on"].as<bool>();
  applyLed(desiredState);

  sendJsonStatus();
}

void handleNotFound() {
  server.sendHeader("Connection", "close");
  server.send(404, "application/json", "{\"error\":\"not found\"}");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(LED_PIN, OUTPUT);
  applyLed(false);

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    while (true) {
      delay(1000);
    }
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.getServer().setRSACert(&cert, &key);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/index.html", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/control", HTTP_POST, handleControl);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTPS server started");
}

// ================= LOOP =================
void loop() {
  server.handleClient();
}