#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

/* ================= WIFI CONFIG ================= */
#define SSID     "WIFI-SSID"
#define PASSWORD "WIFI-PASSWORD"

IPAddress local_IP(192,168,1,36);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8,8,8,8);
IPAddress secondaryDNS(8,8,4,4);

ESP8266WebServer server(80);

/* ================= HARDWARE ================= */
const int LED_SENSOR_PIN = A0;   // LDR
const int RELAY1_PIN = D1;       // USB D+
const int RELAY2_PIN = D2;       // USB D-

/* ================= LDR LOGIC ================= */
const unsigned long CHECK_INTERVAL_MS = 20;
const unsigned long WINDOW_MS = 300;
const int STABILITY_THRESHOLD = 30;
const unsigned long STABLE_TIME_MS = 500;
const unsigned long UNSTABLE_TIME_MS = 300;

/* ================= STATE ================= */
bool relaysOn = false;
unsigned long lastCheck = 0;
unsigned long lastStableTime = 0;
unsigned long lastUnstableTime = 0;
int minVal = 1023;
int maxVal = 0;

/* ================= MODE ================= */
enum ControlMode {
  MODE_LDR,
  MODE_FORCE_ON,
  MODE_FORCE_OFF
};
ControlMode controlMode = MODE_LDR;

/* ================= RELAY HELPERS ================= */
void relaysON() {
  digitalWrite(RELAY1_PIN, LOW);   // active LOW
  digitalWrite(RELAY2_PIN, LOW);
  relaysOn = true;
}

void relaysOFF() {
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  relaysOn = false;
}

/* ================= WIFI ================= */
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);
  WiFi.begin(SSID, PASSWORD);

  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n⚠️ WiFi not connected yet");
  }
}

/* ================= HTML UI ================= */
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>USB LDR Controller</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{font-family:sans-serif;background:#111;color:#eee;
display:flex;justify-content:center;align-items:center;height:100vh}
.card{background:#1c1c1c;padding:20px;border-radius:12px;width:320px}
button{width:100%;padding:12px;margin:6px 0;border:none;
border-radius:8px;font-size:16px}
.ldr{background:#3498db;color:#fff}
.on{background:#27ae60;color:#fff}
.off{background:#c0392b;color:#fff}
.power{background:#f39c12;color:#000;font-weight:bold}
</style>
</head>
<body>
<div class="card">
<h2>USB LDR Controller</h2>

<button class="ldr" onclick="go('/mode/ldr')">LDR (Auto)</button>
<button class="on" onclick="go('/mode/on')">Force ON</button>
<button class="off" onclick="go('/mode/off')">Force OFF</button>
<button class="power" onclick="go('/poweroff')">⏻ Power OFF</button>

</div>

<script>
function go(p){ fetch(p); }
</script>
</body>
</html>
)rawliteral";

/* ================= API HANDLERS ================= */
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleModeLDR() {
  controlMode = MODE_LDR;
  server.send(200,"text/plain","Mode: LDR");
}

void handleModeOn() {
  controlMode = MODE_FORCE_ON;
  relaysON();
  server.send(200,"text/plain","Mode: FORCE ON");
}

void handleModeOff() {
  controlMode = MODE_FORCE_OFF;
  relaysOFF();
  server.send(200,"text/plain","Mode: FORCE OFF");
}

/* 🔥 ATOMIC POWER OFF */
void handlePowerOff() {
  controlMode = MODE_FORCE_OFF;
  relaysOFF();
  delay(2000);               // relay + USB settle
  controlMode = MODE_LDR;
  server.send(200,"text/plain","Power OFF → LDR");
}

/* ================= SETUP ================= */
void setup() {
  Serial.begin(115200);

  pinMode(RELAY1_PIN,OUTPUT);
  pinMode(RELAY2_PIN,OUTPUT);
  relaysOFF();

  connectWiFi();

  server.on("/", handleRoot);
  server.on("/mode/ldr", handleModeLDR);
  server.on("/mode/on", handleModeOn);
  server.on("/mode/off", handleModeOff);
  server.on("/poweroff", handlePowerOff);
  server.begin();

  Serial.println("🚀 USB LDR Controller Ready");
}

/* ================= LOOP ================= */
void loop() {
  server.handleClient();

  /* WiFi reconnect */
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long t=0;
    if (millis()-t>5000) {
      t=millis();
      WiFi.disconnect();
      WiFi.begin(SSID,PASSWORD);
    }
  }

  /* FORCE MODES */
  if (controlMode==MODE_FORCE_ON) {
    if(!relaysOn) relaysON();
    return;
  }
  if (controlMode==MODE_FORCE_OFF) {
    if(relaysOn) relaysOFF();
    return;
  }

  /* LDR AUTO MODE */
  unsigned long now=millis();
  if(now-lastCheck>=CHECK_INTERVAL_MS){
    lastCheck=now;
    int v=analogRead(LED_SENSOR_PIN);
    if(v<minVal)minVal=v;
    if(v>maxVal)maxVal=v;

    if(now%WINDOW_MS<CHECK_INTERVAL_MS){
      int diff=maxVal-minVal;
      if(diff<STABILITY_THRESHOLD){
        lastStableTime=now;
        if(!relaysOn && now-lastUnstableTime>STABLE_TIME_MS)
          relaysON();
      } else {
        lastUnstableTime=now;
        if(relaysOn && now-lastStableTime>UNSTABLE_TIME_MS)
          relaysOFF();
      }
      minVal=1023;
      maxVal=0;
    }
  }
}
