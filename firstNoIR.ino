#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>

// GPIO Pin Definitions
#define RELAY_1 22
#define RELAY_2 23
#define RELAY_3 21
#define RELAY_4 19
#define RELAY_5 18
#define RELAY_6 25
#define RELAY_7 32
#define RELAY_8 4

#define FAN_ON 27
#define FAN_LEV1 33
#define FAN_LEV2 14
#define FAN_LEV3 13

#define RESET_PIN 15  // Reset to AP mode button (pull to GND)

// AP Mode Configuration
const char* ap_ssid = "ESP32_SmartSwitch";
const char* ap_password = "12345678";
IPAddress ap_ip(192, 168, 4, 1);
IPAddress ap_gateway(192, 168, 4, 1);
IPAddress ap_subnet(255, 255, 255, 0);

// Static IP Configuration for Home WiFi
IPAddress static_ip(192, 168, 0, 200); // Change this to your desired static IP
IPAddress gateway(192, 168, 0, 1);     // Your router's gateway
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 0, 1);

// Global Variables
WebServer server(80);
Preferences preferences;

bool relayStates[8] = {true, true, true, true, true, true, true, true}; // Default ON
bool fanOnState = true;  // Default ON
int fanLevel = 0;        // 0=OFF, 1=LEV1, 2=LEV2, 3=LEV3
bool useAPMode = true;
String storedSSID = "";
String storedPassword = "";
unsigned long lastStatusPrint = 0;

// GPIO Pins Array
const int relayPins[8] = {RELAY_1, RELAY_2, RELAY_3, RELAY_4, RELAY_5, RELAY_6, RELAY_7, RELAY_8};

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("   ESP32 Smart Switchboard v1.0");
  Serial.println("========================================\n");
  
  // Initialize Preferences
  preferences.begin("wifi-config", false);
  storedSSID = preferences.getString("ssid", "");
  storedPassword = preferences.getString("password", "");
  useAPMode = preferences.getBool("useAP", true);
  
  Serial.println("Stored SSID: " + (storedSSID.length() > 0 ? storedSSID : "None"));
  Serial.println("Use AP Mode: " + String(useAPMode ? "Yes" : "No"));
  
  // Load saved states
  for(int i = 0; i < 8; i++) {
    relayStates[i] = preferences.getBool(("relay" + String(i)).c_str(), true);
  }
  fanOnState = preferences.getBool("fanOn", true);
  fanLevel = preferences.getInt("fanLevel", 0);
  
  // Initialize GPIO Pins
  initGPIO();
  applyAllStates();
  
  Serial.println("\n--- Network Configuration ---");
  
  // Start WiFi
  if (!useAPMode && storedSSID.length() > 0) {
    connectToWiFi();
  } else {
    startAPMode();
  }
  
  // Setup Web Server Routes
  setupRoutes();
  server.begin();
  
  Serial.println("\n========================================");
  Serial.println("   System Ready!");
  Serial.println("========================================\n");
}

void loop() {
  server.handleClient();
  
  // Print status every 10 seconds if in WiFi mode
  if(WiFi.getMode() == WIFI_STA && millis() - lastStatusPrint > 10000) {
    lastStatusPrint = millis();
    Serial.println("\n--- Status Update ---");
    Serial.print("WiFi Connected: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "YES" : "NO");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Access at: http://");
    Serial.println(WiFi.localIP());
    Serial.print("Or via: http://smartswitch.local");
    Serial.println("\n-------------------\n");
  }
  
  // Check if reset button is pressed (pulled to GND)
  if(digitalRead(RESET_PIN) == LOW) {
    delay(50); // Debounce
    if(digitalRead(RESET_PIN) == LOW) {
      Serial.println("Reset button pressed! Switching to AP mode...");
      
      // Save AP mode preference
      preferences.putBool("useAP", true);
      
      delay(500);
      ESP.restart();
    }
  }
}

void initGPIO() {
  // Initialize all relay pins
  for(int i = 0; i < 8; i++) {
    pinMode(relayPins[i], OUTPUT);
  }
  
  // Initialize fan control pins
  pinMode(FAN_ON, OUTPUT);
  pinMode(FAN_LEV1, OUTPUT);
  pinMode(FAN_LEV2, OUTPUT);
  pinMode(FAN_LEV3, OUTPUT);
  
  // Initialize reset button with internal pull-up
  pinMode(RESET_PIN, INPUT_PULLUP);
}

void applyAllStates() {
  // Apply relay states (ON = LOW, OFF = HIGH)
  for(int i = 0; i < 8; i++) {
    digitalWrite(relayPins[i], relayStates[i] ? LOW : HIGH);
  }
  
  // Apply fan states
  digitalWrite(FAN_ON, fanOnState ? LOW : HIGH);
  
  if(fanOnState) {
    // Fan is ON, disable all levels
    digitalWrite(FAN_LEV1, LOW);
    digitalWrite(FAN_LEV2, LOW);
    digitalWrite(FAN_LEV3, LOW);
  } else {
    // Fan is OFF, apply level if selected
    applyFanLevel();
  }
}

void applyFanLevel() {
  // First, turn off all levels with delay
  digitalWrite(FAN_LEV1, LOW);
  delay(50);
  digitalWrite(FAN_LEV2, LOW);
  delay(50);
  digitalWrite(FAN_LEV3, LOW);
  delay(50);
  
  // Then turn on the selected level (if any)
  if(fanLevel == 1) {
    digitalWrite(FAN_LEV1, HIGH);
  } else if(fanLevel == 2) {
    digitalWrite(FAN_LEV2, HIGH);
  } else if(fanLevel == 3) {
    digitalWrite(FAN_LEV3, HIGH);
  }
}

void startAPMode() {
  Serial.println("Starting AP Mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet);
  WiFi.softAP(ap_ssid, ap_password);
  
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Connect to WiFi: " + String(ap_ssid));
  Serial.println("Navigate to: http://192.168.4.1");
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  Serial.println("SSID: " + storedSSID);
  WiFi.mode(WIFI_STA);
  WiFi.config(static_ip, gateway, subnet, dns);
  WiFi.begin(storedSSID.c_str(), storedPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("Static IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
    
    // Setup mDNS
    if(MDNS.begin("smartswitch")) {
      Serial.println("mDNS started: http://smartswitch.local");
    }
  } else {
    Serial.println("\n❌ Failed to connect to WiFi!");
    Serial.println("Reason: " + String(WiFi.status()));
    Serial.println("Falling back to AP Mode...");
    
    // Clear saved credentials and switch to AP mode
    preferences.putBool("useAP", true);
    startAPMode();
  }
}

void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/config", HTTP_GET, handleConfig);
  server.on("/savewifi", HTTP_POST, handleSaveWiFi);
  server.on("/useap", HTTP_POST, handleUseAP);
  server.on("/toggle", HTTP_POST, handleToggle);
  server.on("/fanon", HTTP_POST, handleFanOn);
  server.on("/fanlevel", HTTP_POST, handleFanLevel);
  server.on("/status", HTTP_GET, handleStatus);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Smart Switchboard</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:0;padding:20px;background:#f0f0f0;}";
  html += ".container{max-width:600px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;text-align:center;margin-bottom:30px;}";
  html += ".switch-row{display:flex;justify-content:space-between;align-items:center;padding:15px;border-bottom:1px solid #eee;}";
  html += ".switch-row:last-child{border-bottom:none;}";
  html += ".label{font-size:16px;font-weight:500;}";
  html += ".toggle{position:relative;width:60px;height:30px;background:#ccc;border-radius:30px;cursor:pointer;transition:0.3s;}";
  html += ".toggle.on{background:#4CAF50;}";
  html += ".toggle-slider{position:absolute;top:3px;left:3px;width:24px;height:24px;background:white;border-radius:50%;transition:0.3s;}";
  html += ".toggle.on .toggle-slider{left:33px;}";
  html += ".fan-section{background:#f9f9f9;padding:15px;margin-top:20px;border-radius:8px;}";
  html += ".level-buttons{display:flex;gap:10px;margin-top:10px;}";
  html += ".level-btn{flex:1;padding:10px;border:none;border-radius:5px;cursor:pointer;font-weight:bold;transition:0.3s;}";
  html += ".level-btn.active{background:#2196F3;color:white;}";
  html += ".level-btn.inactive{background:#e0e0e0;color:#666;}";
  html += ".config-btn{display:block;width:100%;padding:15px;margin-top:20px;background:#FF9800;color:white;border:none;border-radius:5px;font-size:16px;cursor:pointer;}";
  html += ".config-btn:hover{background:#F57C00;}";
  html += ".info{text-align:center;margin-top:20px;color:#666;font-size:14px;}";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>Smart Switchboard</h1>";
  
  // Regular switches
  for(int i = 0; i < 8; i++) {
    html += "<div class='switch-row'>";
    html += "<span class='label'>Switch " + String(i+1) + "</span>";
    html += "<div class='toggle " + String(relayStates[i] ? "on" : "") + "' onclick='toggleRelay(" + String(i) + ")'>";
    html += "<div class='toggle-slider'></div></div></div>";
  }
  
  // Fan section
  html += "<div class='fan-section'>";
  html += "<div class='switch-row'>";
  html += "<span class='label'>Fan Power</span>";
  html += "<div class='toggle " + String(fanOnState ? "on" : "") + "' onclick='toggleFan()'>";
  html += "<div class='toggle-slider'></div></div></div>";
  
  if(!fanOnState) {
    html += "<div class='level-buttons'>";
    html += "<button class='level-btn " + String(fanLevel == 1 ? "active" : "inactive") + "' onclick='setFanLevel(1)'>Level 1</button>";
    html += "<button class='level-btn " + String(fanLevel == 2 ? "active" : "inactive") + "' onclick='setFanLevel(2)'>Level 2</button>";
    html += "<button class='level-btn " + String(fanLevel == 3 ? "active" : "inactive") + "' onclick='setFanLevel(3)'>Level 3</button>";
    html += "</div>";
  }
  html += "</div>";
  
  html += "<button class='config-btn' onclick='location.href=\"/config\"'>WiFi Configuration</button>";
  
  html += "<div class='info'>";
  if(WiFi.getMode() == WIFI_AP) {
    html += "Mode: Access Point<br>IP: " + WiFi.softAPIP().toString();
  } else {
    html += "Mode: WiFi Station<br>IP: " + WiFi.localIP().toString();
  }
  html += "</div>";
  
  html += "</div>";
  
  // JavaScript
  html += "<script>";
  html += "function toggleRelay(num){fetch('/toggle?relay='+num,{method:'POST'}).then(()=>location.reload());}";
  html += "function toggleFan(){fetch('/fanon',{method:'POST'}).then(()=>location.reload());}";
  html += "function setFanLevel(lev){fetch('/fanlevel?level='+lev,{method:'POST'}).then(()=>location.reload());}";
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleConfig() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>WiFi Configuration</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:0;padding:20px;background:#f0f0f0;}";
  html += ".container{max-width:500px;margin:0 auto;background:white;padding:30px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;text-align:center;}";
  html += ".form-group{margin:20px 0;}";
  html += "label{display:block;margin-bottom:5px;font-weight:500;}";
  html += "input{width:100%;padding:10px;border:1px solid #ddd;border-radius:5px;box-sizing:border-box;}";
  html += "button{width:100%;padding:15px;margin-top:10px;border:none;border-radius:5px;font-size:16px;cursor:pointer;font-weight:bold;}";
  html += ".btn-primary{background:#4CAF50;color:white;}";
  html += ".btn-secondary{background:#2196F3;color:white;}";
  html += ".btn-back{background:#666;color:white;margin-top:20px;}";
  html += "button:hover{opacity:0.9;}";
  html += ".info{background:#e3f2fd;padding:15px;border-radius:5px;margin-bottom:20px;}";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>WiFi Setup</h1>";
  
  html += "<div class='info'>Current Mode: " + String(WiFi.getMode() == WIFI_AP ? "Access Point" : "WiFi Station") + "</div>";
  
  html += "<form action='/savewifi' method='post'>";
  html += "<div class='form-group'><label>WiFi SSID:</label><input type='text' name='ssid' value='" + storedSSID + "' required></div>";
  html += "<div class='form-group'><label>WiFi Password:</label><input type='password' name='password' value='" + storedPassword + "'></div>";
  html += "<button type='submit' class='btn-primary'>Save & Connect to WiFi</button>";
  html += "</form>";
  
  html += "<form action='/useap' method='post'>";
  html += "<button type='submit' class='btn-secondary'>Use AP Mode Only</button>";
  html += "</form>";
  
  html += "<button class='btn-back' onclick='location.href=\"/\"'>Back to Controls</button>";
  
  html += "</div>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleSaveWiFi() {
  if(server.hasArg("ssid")) {
    storedSSID = server.arg("ssid");
    storedPassword = server.arg("password");
    
    preferences.putString("ssid", storedSSID);
    preferences.putString("password", storedPassword);
    preferences.putBool("useAP", false);
    
    String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='5;url=/'></head><body style='font-family:Arial;text-align:center;padding:50px;'>";
    html += "<h2>WiFi Settings Saved!</h2>";
    html += "<p>Connecting to: " + storedSSID + "</p>";
    html += "<p>The device will now restart and connect to your WiFi.</p>";
    html += "<p>Find the device at: http://" + static_ip.toString() + "</p>";
    html += "<p>Redirecting in 5 seconds...</p>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
    
    delay(2000);
    ESP.restart();
  }
}

void handleUseAP() {
  preferences.putBool("useAP", true);
  
  String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='3;url=/'></head><body style='font-family:Arial;text-align:center;padding:50px;'>";
  html += "<h2>AP Mode Enabled!</h2>";
  html += "<p>The device will restart in AP mode.</p>";
  html += "<p>Connect to: " + String(ap_ssid) + "</p>";
  html += "<p>Access at: http://192.168.4.1</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  delay(2000);
  ESP.restart();
}

void handleToggle() {
  if(server.hasArg("relay")) {
    int relayNum = server.arg("relay").toInt();
    if(relayNum >= 0 && relayNum < 8) {
      relayStates[relayNum] = !relayStates[relayNum];
      digitalWrite(relayPins[relayNum], relayStates[relayNum] ? LOW : HIGH);
      preferences.putBool(("relay" + String(relayNum)).c_str(), relayStates[relayNum]);
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad Request");
}

void handleFanOn() {
  fanOnState = !fanOnState;
  digitalWrite(FAN_ON, fanOnState ? LOW : HIGH);
  
  if(fanOnState) {
    // Fan turned ON, disable all levels
    fanLevel = 0;
    digitalWrite(FAN_LEV1, LOW);
    digitalWrite(FAN_LEV2, LOW);
    digitalWrite(FAN_LEV3, LOW);
  }
  
  preferences.putBool("fanOn", fanOnState);
  preferences.putInt("fanLevel", fanLevel);
  
  server.send(200, "text/plain", "OK");
}

void handleFanLevel() {
  if(server.hasArg("level")) {
    int level = server.arg("level").toInt();
    if(level >= 1 && level <= 3 && !fanOnState) {
      fanLevel = level;
      applyFanLevel();
      preferences.putInt("fanLevel", fanLevel);
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(400, "text/plain", "Bad Request");
}

void handleStatus() {
  String json = "{";
  json += "\"relays\":[";
  for(int i = 0; i < 8; i++) {
    json += String(relayStates[i] ? "true" : "false");
    if(i < 7) json += ",";
  }
  json += "],";
  json += "\"fanOn\":" + String(fanOnState ? "true" : "false") + ",";
  json += "\"fanLevel\":" + String(fanLevel) + ",";
  json += "\"mode\":\"" + String(WiFi.getMode() == WIFI_AP ? "AP" : "STA") + "\",";
  json += "\"ip\":\"" + (WiFi.getMode() == WIFI_AP ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}