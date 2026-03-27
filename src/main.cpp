#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

ESP8266WebServer server(80);
DNSServer dnsServer;

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

String saved_ssid = "";
String saved_pass = "";

// ================= HTML STATUS PAGE =================
String buildConnectedPage() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP Connected</title>
    <style>
        body {
            margin: 0;
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #11998e, #38ef7d);
            color: white;
        }
        .box {
            background: rgba(255,255,255,0.12);
            padding: 30px;
            border-radius: 18px;
            width: 340px;
            text-align: center;
            box-shadow: 0 10px 25px rgba(0,0,0,0.25);
        }
        h1 {
            margin-top: 0;
            font-size: 28px;
        }
        p {
            margin: 10px 0;
            font-size: 16px;
        }
        .ip {
            margin-top: 15px;
            font-weight: bold;
            font-size: 18px;
            background: rgba(0,0,0,0.15);
            padding: 10px;
            border-radius: 10px;
        }
        .ssid {
            margin-top: 10px;
            background: rgba(0,0,0,0.12);
            padding: 8px;
            border-radius: 10px;
        }
        .btn {
            display: inline-block;
            margin-top: 18px;
            padding: 12px 18px;
            border-radius: 10px;
            text-decoration: none;
            color: white;
            background: rgba(0,0,0,0.18);
        }
    </style>
</head>
<body>
    <div class="box">
        <h1>Connected</h1>
        <p>ESP8266 successfully connected to Wi-Fi</p>
        <p class="ssid">SSID: )rawliteral";

    html += saved_ssid;

    html += R"rawliteral(</p>
        <p class="ip">IP: )rawliteral";

    html += WiFi.localIP().toString();

    html += R"rawliteral(</p>
        <a class="btn" href="/reset-wifi">Reset Wi-Fi</a>
    </div>
</body>
</html>
)rawliteral";

    return html;
}

// ================= SAVE WIFI =================
void handleSave() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    Serial.println("POST /save");
    Serial.print("SSID: ");
    Serial.println(ssid);

    if (ssid.length() == 0) {
        server.send(400, "text/plain", "SSID is empty");
        return;
    }

    File file = LittleFS.open("/wifi.json", "w");
    if (!file) {
        server.send(500, "text/plain", "Failed to open wifi.json for write");
        return;
    }

    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["pass"] = pass;

    serializeJson(doc, file);
    file.close();

    server.send(200, "text/html", "<h2>Saved! Rebooting...</h2>");
    delay(2000);
    ESP.restart();
}

// ================= LOAD WIFI =================
void loadWiFi() {
    if (!LittleFS.exists("/wifi.json")) {
        Serial.println("wifi.json not found");
        return;
    }

    File file = LittleFS.open("/wifi.json", "r");
    if (!file) {
        Serial.println("Failed to open wifi.json");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        Serial.println("Failed to parse wifi.json");
        return;
    }

    saved_ssid = doc["ssid"].as<String>();
    saved_pass = doc["pass"].as<String>();

    Serial.print("Loaded SSID: ");
    Serial.println(saved_ssid);
}

// ================= RESET WIFI =================
void handleResetWiFi() {
    Serial.println("GET /reset-wifi");

    if (LittleFS.exists("/wifi.json")) {
        LittleFS.remove("/wifi.json");
        Serial.println("wifi.json removed");
    }

    server.send(200, "text/html", "<h2>Wi-Fi settings removed. Rebooting...</h2>");
    delay(1500);
    ESP.restart();
}

// ================= SHOW CONFIG PAGE =================
void handleConfigPage() {
    Serial.println("GET /");

    File file = LittleFS.open("/config.html", "r");
    if (!file) {
        server.send(404, "text/plain", "config.html not found");
        return;
    }

    server.streamFile(file, "text/html");
    file.close();
}

// ================= SHOW CONNECTED PAGE =================
void handleConnectedPage() {
    Serial.println("GET /");
    server.send(200, "text/html", buildConnectedPage());
}

// ================= AP MODE =================
void startAPMode() {
    Serial.println("WiFi connect failed -> starting AP mode");

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("ESP8266-Setup");

    dnsServer.start(DNS_PORT, "*", apIP);

    server.on("/", HTTP_GET, handleConfigPage);
    server.on("/save", HTTP_POST, handleSave);

    // Для авто-детекції captive portal на різних ОС
    server.on("/generate_204", HTTP_GET, []() {
        server.sendHeader("Location", String("http://") + apIP.toString(), true);
        server.send(302, "text/plain", "");
    });

    server.on("/fwlink", HTTP_GET, []() {
        server.sendHeader("Location", String("http://") + apIP.toString(), true);
        server.send(302, "text/plain", "");
    });

    server.on("/hotspot-detect.html", HTTP_GET, []() {
        server.sendHeader("Location", String("http://") + apIP.toString(), true);
        server.send(302, "text/plain", "");
    });

    server.onNotFound([]() {
        server.sendHeader("Location", String("http://") + apIP.toString(), true);
        server.send(302, "text/plain", "");
    });

    server.begin();

    Serial.println("AP MODE: 192.168.4.1");
    Serial.println("SSID: ESP8266-Setup");
}

// ================= STA MODE =================
void startSTAModeServer() {
    server.on("/", HTTP_GET, handleConnectedPage);
    server.on("/reset-wifi", HTTP_GET, handleResetWiFi);
    server.begin();

    Serial.println("HTTP server started (STA)");
}

// ================= SETUP =================
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("BOOT");
    Serial.println("SETUP START");

    if (!LittleFS.begin()) {
        Serial.println("LittleFS ERROR");
        return;
    }
    Serial.println("LittleFS OK");

    loadWiFi();

    if (saved_ssid.length() > 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(saved_ssid.c_str(), saved_pass.c_str());

        Serial.print("Connecting to WiFi");
        int timeout = 20;
        while (WiFi.status() != WL_CONNECTED && timeout--) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WIFI CONNECTED");
        Serial.print("Device IP: ");
        Serial.println(WiFi.localIP());

        startSTAModeServer();
    } else {
        startAPMode();
    }
}

// ================= LOOP =================
void loop() {
    if (WiFi.getMode() == WIFI_AP) {
        dnsServer.processNextRequest();
    }

    server.handleClient();
}