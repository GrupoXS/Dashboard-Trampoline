#include <WiFi.h>
#include <HTTPClient.h>

const char* SSID = "Bonetti-Pini";
const char* PASSWORD = "Emily7077";
const char* API_KEY = "db01563a08a4663e170dec0f435d7b059578fd9fcfd910d0";
const char* URL = "http://192.168.15.33:3000/api/telemetry";
struct Telemetria {
  int speed;    // km/h
  int battery;  // %
  int range;    // km
  int power;    // kW
  char gear;    // P, R, N, D ou B
};

Telemetria telemetria = {66, 76, 304, 42,'D'};
char payloadBuffer[128];
const char* PAYLOAD = payloadBuffer;

void atualizarPayload() {
  snprintf(payloadBuffer, sizeof(payloadBuffer),
           R"({"speed":%d,"battery":%d,"range":%d,"power":%d,"gear":"%c"})",
           telemetria.speed, telemetria.battery, telemetria.range,
           telemetria.power, telemetria.gear);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(SSID, PASSWORD);
  Serial.println("Conectando ao Wi-Fi...");
}

void loop() {
  static uint32_t lastAttempt = 0;
  static uint32_t lastReconnect = 0;
  uint32_t now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastReconnect >= 15000) {
      lastReconnect = now;
      Serial.println("Tentando reconectar ao Wi-Fi...");
      WiFi.reconnect();
    }
    delay(10);
    return;
  }
  if (now - lastAttempt < 1000) {
    delay(1);
    return;
  }
  lastAttempt = now;
  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, URL)) {
    Serial.println("Falha ao preparar a URL.");
    return;
  }
  http.setConnectTimeout(3000);
  http.setTimeout(3000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + API_KEY);

  telemetria.speed++;
  telemetria.gear = 'B';
  atualizarPayload();

  int status = http.POST(String(PAYLOAD));
  Serial.printf("POST /api/telemetry: %d\n", status);
  if (status < 0) {
    Serial.printf("Conexao com %s: %s | IP da placa: %s | RSSI: %d dBm\n",
                  URL, HTTPClient::errorToString(status).c_str(),
                  WiFi.localIP().toString().c_str(), (int)WiFi.RSSI());
  }
  http.end();
}