/*
  RAECA Micro-Edge (ESP32) reference firmware
  ------------------------------------------------
  Role:
  - Sense (DHT22, analog soil moisture)
  - Decide simple actions locally (hysteresis for irrigation)
  - Publish telemetry to an MQTT broker and expose health state
  - Receive commands to override irrigation

  Notes for developers:
  - Calibrate soil moisture (DRY/WET) for your sensor and medium.
  - Update Wi‑Fi and MQTT broker settings for your environment.
  - Topics are namespaced to the device MAC to avoid collisions.
  - Health is advertised using a retained LWT (Last Will & Testament).
  - This sketch favors readability; production deployments should add
    TLS/mTLS, backoff/timeout policies, and persistent storage as needed.
*/
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <esp_system.h>

// Wi‑Fi credentials. For Wokwi, use SSID "Wokwi-GUEST" with empty password.
#define WIFI_SSID     "Wokwi-GUEST"
#define WIFI_PASS     ""

// Public test broker used for demo purposes. Do not publish secrets here.
// For production, prefer a private broker with TLS/mTLS and ACLs.
#define MQTT_BROKER   "broker.hivemq.com"
#define MQTT_PORT     1883

// Hardware pin mapping (adjust if your wiring differs)
#define DHTPIN        15
#define DHTTYPE       DHT22
#define SOIL_PIN      34
#define RELAY_PIN      2

// Root topic prefix. Final topic format:
//   raeca/agri/<MAC_HEX>/field1/{telemetry|cmd|health}
#define BASE_PREFIX   "raeca/agri"

DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// Telemetry cadence tracking (millis)
unsigned long lastSend = 0;
bool irrigationOn = false;
// Topic and identity strings are built at runtime from the ESP32 MAC
String mqttClientId;
String baseTopic;
String tlmTopic;
String cmdTopic;
String healthTopic;

// Map raw analog reading to an approximate soil moisture percentage.
// IMPORTANT: Calibrate DRY and WET with your sensor in air and water,
// then tune for your soil/substrate. Wokwi readings differ from real hardware.
float mapSoilToPercent(int raw) {
  // Adjust these after calibrating your sensor in Wokwi/real hardware
  const int DRY = 3000, WET = 1200;
  raw = constrain(raw, WET, DRY);
  float pct = 100.0 * (float)(DRY - raw) / (float)(DRY - WET);
  return constrain(pct, 0.0, 100.0);
}

// Drive the relay controlling the pump/valve.
// NOTE: Some relay boards are active-LOW; invert logic if needed.
void setIrrigation(bool on) {
  irrigationOn = on;
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
}

// Publish a compact JSON telemetry payload to the telemetry topic.
// Schema: {"temp":<C|null>,"hum":<%|null>,"soil":<%|null>,
//          "pump":<bool>,"rssi":<dBm>,"heap":<bytes>}
// The message is marked 'retained' so the broker keeps the latest sample.
void publishJson(float t, float h, float soilPct, int rssi, int freeHeap) {
  char payload[256];
  char tbuf[16];
  char hbuf[16];
  char sbuf[16];
  if (isnan(t)) strcpy(tbuf, "null"); else dtostrf(t, 0, 2, tbuf);
  if (isnan(h)) strcpy(hbuf, "null"); else dtostrf(h, 0, 2, hbuf);
  if (isnan(soilPct)) strcpy(sbuf, "null"); else dtostrf(soilPct, 0, 2, sbuf);
  snprintf(payload, sizeof(payload),
    "{\"temp\":%s,\"hum\":%s,\"soil\":%s,\"pump\":%s,\"rssi\":%d,\"heap\":%d}",
    tbuf, hbuf, sbuf, irrigationOn ? "true" : "false", rssi, freeHeap);
  mqtt.publish(tlmTopic.c_str(), payload, true);
}

// Handle incoming control commands on the cmd topic.
// Supported payloads (case-insensitive):
//   - "irrigation/on"
//   - "irrigation/off"
void handleCmd(char* topic, byte* payload, unsigned int length) {
  String cmd;
  for (unsigned int i = 0; i < length; i++) cmd += (char)payload[i];
  cmd.trim();
  if (cmd.equalsIgnoreCase("irrigation/on")) {
    setIrrigation(true);
  } else if (cmd.equalsIgnoreCase("irrigation/off")) {
    setIrrigation(false);
  }
}

// Ensure Wi‑Fi connectivity. Blocks until connected.
// For production, consider adding a timeout/backoff and offline mode.
void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
}

// Ensure MQTT connectivity. Sets a retained LWT of "offline" and publishes
// a retained "online" message upon successful connect. Subscribes to cmd topic.
void ensureMqtt() {
  if (mqtt.connected()) return;
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(handleCmd);
  Serial.print("Connecting to MQTT broker ");
  Serial.print(MQTT_BROKER);
  Serial.print(":");
  Serial.println(MQTT_PORT);
  while (!mqtt.connected()) {
    // Connect with LWT configured: if this client drops unexpectedly,
    // broker will publish "offline" to healthTopic (retained).
    if (mqtt.connect(mqttClientId.c_str(), nullptr, nullptr, healthTopic.c_str(), 1, true, "offline")) {
      Serial.println("MQTT connected");
      break;
    } else {
      Serial.print("MQTT connect failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(". Retrying in 1s...");
      delay(1000);
    }
  }
  // Mark device online and subscribe for commands.
  // Note: PubSubClient publishes at QoS 0; retention preserves last value.
  mqtt.publish(healthTopic.c_str(), "online", true);
  Serial.println("Published health: online (retained)");
  mqtt.subscribe(cmdTopic.c_str(), 1);
  Serial.print("Subscribed to CMD topic: ");
  Serial.println(cmdTopic);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  setIrrigation(false);
  // Keep radios awake for lower latency (higher energy cost). Adjust as needed.
  WiFi.setSleep(false);
  ensureWifi();
  // Keepalive and buffer size tuned for small JSON payloads and health pings.
  mqtt.setKeepAlive(15);
  mqtt.setBufferSize(512);
  uint32_t rnd = esp_random();
  // Unique clientId helps brokers manage sessions and logs.
  mqttClientId = String("raeca-") + String((uint32_t)ESP.getEfuseMac(), HEX) + "-" + String(rnd, HEX);
  Serial.print("MQTT clientId: ");
  Serial.println(mqttClientId);

  // Build unique topic namespace using MAC
  String macHex = String((uint32_t)ESP.getEfuseMac(), HEX);
  baseTopic = String(BASE_PREFIX) + "/" + macHex + "/field1";
  tlmTopic = baseTopic + "/telemetry";
  cmdTopic = baseTopic + "/cmd";
  healthTopic = baseTopic + "/health";
  Serial.print("Base topic: "); Serial.println(baseTopic);
  Serial.print("Telemetry topic: "); Serial.println(tlmTopic);
  Serial.print("Health topic: "); Serial.println(healthTopic);
}

void loop() {
  // Maintain connectivity and pump MQTT client loop for callbacks.
  ensureWifi();
  ensureMqtt();
  mqtt.loop();

  unsigned long now = millis();
  // Send telemetry at a fixed cadence (default 5s). Tune for your use case.
  if (now - lastSend >= 5000) { // 5s interval
    lastSend = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int soilRaw = analogRead(SOIL_PIN);
    float soilPct = mapSoilToPercent(soilRaw); // Convert raw ADC to % estimate

    // Simple edge decision with hysteresis
    if (!isnan(soilPct)) {
      // Adjust thresholds after field calibration; keep some gap to avoid flapping.
      if (soilPct < 35.0) setIrrigation(true);
      else if (soilPct > 45.0) setIrrigation(false);
    }

    int rssi = WiFi.RSSI();
    int heap = ESP.getFreeHeap();
    if (isnan(t) || isnan(h)) {
      Serial.println("DHT read failed; publishing telemetry with nulls for temp/hum");
    }
    publishJson(t, h, soilPct, rssi, heap); // Retained telemetry for latest snapshot
    Serial.print("Published telemetry: ");
    Serial.print("t="); Serial.print(t);
    Serial.print(", h="); Serial.print(h);
    Serial.print(", soil="); Serial.print(soilPct);
    Serial.print(", rssi="); Serial.print(rssi);
    Serial.print(", heap="); Serial.println(heap);
  }
}
