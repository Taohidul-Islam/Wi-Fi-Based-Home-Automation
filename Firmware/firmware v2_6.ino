/*
 * ============================================================
 *   Wi-Fi Based Automation Firmware v2.6
 *   by Taohid | Phyxon Tech
 *   Board: DOIT ESP32 DevKit V1
 *   Change Line 113 and 504 as per your credentials
 * ============================================================
 *
 * Required Libraries (install via Arduino Library Manager):
 *   - IRremote (by Armin Joachimsmeyer) >= 4.x
 *   - U8g2 (by oliver)
 *   - ArduinoJson (by Benoit Blanchon) >= 6.x
 *   - NewPing (by Tim Eckel)
 *
 * Changes in v2.6:
 *   - IR I button: decrease people count (min 0)
 *   - IR J button: increase people count
 *   - IR K button: factory reset (moved from J)
 * ============================================================
 * GPIO TABLE:
 *   RELAY_1=22, RELAY_2=23, RELAY_3=21, RELAY_4=19
 *   RELAY_5=18, RELAY_6=25, RELAY_7=32, RELAY_8=4
 *   RESET_PIN=15  (INPUT_PULLUP, pull to GND = factory reset)
 *   LED_CTRL=2    (PNP driver, LOW=LEDs ON)
 *   LED_BTN=14    (OUTPUT HIGH after boot, pull to GND = toggle LED)
 *   IR_RECV=26
 *   OLED_SDA=27, OLED_SCL=33
 *   SONAR1_TRIG=5,  SONAR1_ECHO=34  (outside doorframe)
 *   SONAR2_TRIG=12, SONAR2_ECHO=35  (inside doorframe)
 * ============================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <IRremote.hpp>
#include <NewPing.h>
#include <time.h>
#include <math.h>

// ============================================================
// GPIO
// ============================================================
#define RELAY_1      22
#define RELAY_2      23
#define RELAY_3      21
#define RELAY_4      19
#define RELAY_5      18
#define RELAY_6      25
#define RELAY_7      32
#define RELAY_8       4
#define RESET_PIN    15
#define LED_CTRL      2
#define LED_BTN      14
#define IR_RECV_PIN  26
#define OLED_SDA     27
#define OLED_SCL     33
#define SONAR1_TRIG   5
#define SONAR1_ECHO  34
#define SONAR2_TRIG  12
#define SONAR2_ECHO  35
#define SONAR_MAX_DIST 200

// ============================================================
// IR CODES
// ============================================================
#define IR_1     0x01
#define IR_2     0x02
#define IR_3     0x03
#define IR_4     0x04
#define IR_5     0x05
#define IR_6     0x06
#define IR_7     0x07
#define IR_8     0x08
#define IR_9     0x09
#define IR_0     0x00
#define IR_PWR   0x12
#define IR_A     0x14
#define IR_B     0x10
#define IR_C     0x16
#define IR_D     0x0C
#define IR_E     0x49
#define IR_H     0x0F
#define IR_I     0x45
#define IR_J     0x56
#define IR_K     0x5C
#define IR_OK    0x0E
#define IR_UP    0x1B
#define IR_DOWN  0x1F
#define IR_LEFT  0x1E
#define IR_RIGHT 0x1A
#define IR_SLASH 0x0B

// ============================================================
// NETWORK
// ============================================================
const char* ap_ssid     = "ESP32_SmartSwitch";
const char* ap_password = "12345678";
IPAddress ap_ip(192, 168, 4, 1);
IPAddress ap_gateway(192, 168, 4, 1);
IPAddress ap_subnet(255, 255, 255, 0);
IPAddress static_ip(192, 168, 0, 200);
IPAddress gateway_ip(192, 168, 0, 1);
IPAddress subnet_ip(255, 255, 255, 0);
IPAddress dns_ip(192, 168, 0, 1);

const char* OTA_HOST   = "YOUR SERVER IP HERE";
const int   OTA_PORT   = 8080;
const char* FW_VERSION = "2.6";

// ============================================================
// OLED — SH1106 1.3" SW I2C, U8G2_R2 = 180° for upside-down mount
// ============================================================
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R2, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

// ============================================================
// SONARS
// ============================================================
NewPing sonar1(SONAR1_TRIG, SONAR1_ECHO, SONAR_MAX_DIST);
NewPing sonar2(SONAR2_TRIG, SONAR2_ECHO, SONAR_MAX_DIST);

// ============================================================
// SCREEN IDs
// ============================================================
#define SCREEN_WEATHER   0
#define SCREEN_EYES      1
#define SCREEN_CLOCK     2
#define SCREEN_ANALOG    3
#define SCREEN_OVERVIEW  4
#define SCREEN_WEBUI     5
#define SCREEN_PEOPLE    6
#define SCREEN_COUNT     7

// ============================================================
// EYE SYSTEM
// ============================================================
#define LEX    34
#define REX    94
#define EYE_CY 30
#define EW     28
#define EH     36
#define ER     10

enum Emotion {
  EMO_FRONT=0, EMO_BLINK, EMO_SLEEP, EMO_HAPPY,
  EMO_MAD, EMO_CRY, EMO_CONFUSED, EMO_TIRED,
  EMO_WIDE, EMO_NIGHT, EMO_COUNT
};

Emotion currentEmo = EMO_FRONT;
unsigned long emoStart = 0;
int emoSeqIdx = 0;
float pupilX = 0, pupilY = 0, pupilTX = 0, pupilTY = 0;
unsigned long lastPupilPick = 0;

Emotion emoSequence[] = {
  EMO_FRONT, EMO_BLINK, EMO_FRONT, EMO_HAPPY,
  EMO_FRONT, EMO_BLINK, EMO_MAD,   EMO_FRONT,
  EMO_CRY,   EMO_FRONT, EMO_TIRED, EMO_BLINK,
  EMO_WIDE,  EMO_FRONT, EMO_CONFUSED, EMO_FRONT,
  EMO_SLEEP, EMO_FRONT, EMO_NIGHT, EMO_FRONT
};
const int emoSeqLen = 20;
unsigned long emoDurations[] = {
  2500,400,2000,1500,2000,400,1800,1500,
  2000,1500,1800,400,1200,2000,2000,2000,
  2500,1500,2000,2000
};

// ============================================================
// WEATHER
// ============================================================
struct Weather {
  String description;
  float  temp=0; int humidity=0;
  bool   isCloudy=false; bool fetched=false;
};
Weather wx;
unsigned long lastWeatherFetch=0;
float cloudOffset=0; unsigned long lastCloudUpdate=0;
#define WEATHER_INTERVAL 600000UL

// ============================================================
// GLOBAL STATE
// ============================================================
WebServer   server(80);
Preferences preferences;

bool relayStates[8]      = {false};
bool ledState             = true;
const int relayPins[8]   = {RELAY_1,RELAY_2,RELAY_3,RELAY_4,RELAY_5,RELAY_6,RELAY_7,RELAY_8};

bool   useAPMode    = true;
String storedSSID   = "";
String storedPass   = "";

bool   darkMode         = true;
String weatherCity      = "Dhaka";
int    timezone_offset  = 6;

bool   autoOffEnabled[8]      = {false};
int    peopleCount             = 1;
bool   sonarActive             = false;
bool   autoOffedByPresence[8] = {false};

int    autoOffDelaySec  = 0;   // seconds to wait before turning off on zero count
int    autoOnDelaySec   = 0;   // seconds to wait before turning on when count goes 0→1+

// Timer state for delayed actions
bool          pendingOff        = false;
unsigned long pendingOffTime    = 0;
bool          pendingOn         = false;
unsigned long pendingOnTime     = 0;

bool   screenEnabled[SCREEN_COUNT];
int    screenInterval       = 5;
bool   autoSwitch           = true;
int    currentScreen        = SCREEN_EYES;
int    fixedScreen          = SCREEN_EYES;
int    screenBeforeWebUI    = SCREEN_EYES;

bool   oledInWebUI  = false;
int    oledTab      = 0;
int    oledCursor   = 0;

bool   showingStartupIP = true;
unsigned long startupIPTime = 0;
#define STARTUP_IP_DURATION 10000UL

unsigned long lastOLEDDraw   = 0;
#define OLED_REFRESH_MS 30

unsigned long lastScreenSwitch = 0;
unsigned long ledBtnLastPress  = 0;
#define DEBOUNCE_MS 200

int    sonarDetectDist  = 60;    // cm — configurable from web settings
int    sonarConfirmWin  = 1500;  // ms — tightened for 15cm doorframe

// Alternating fire state
int    sonarTurn        = 0;     // 0=fire S1, 1=fire S2
unsigned long lastSonarFire = 0;
#define SONAR_FIRE_INTERVAL 50   // ms between alternate pings

unsigned long sonar1TrigTime=0, sonar2TrigTime=0;
bool sonar1Triggered=false, sonar2Triggered=false;

// Cooldown after crossing
#define SONAR_COOLDOWN 2500
unsigned long lastCrossingTime=0;

// Live state for web dots
bool sonar1Live=false, sonar2Live=false;
unsigned long sonar1LiveTime=0, sonar2LiveTime=0;
#define SONAR_LIVE_DISPLAY_MS 400

// FreeRTOS sonar task handle
TaskHandle_t sonarTaskHandle = NULL;
// Mutex to protect shared sonar state accessed from both cores
portMUX_TYPE sonarMux = portMUX_INITIALIZER_UNLOCKED;

String otaStatusMsg = "";

// ============================================================
// FORWARD DECLARATIONS
// ============================================================
void initGPIO(); void applyAllStates();
void startAPMode(); void connectToWiFi();
void setupRoutes(); void factoryReset();
void fetchWeather(); void syncNTP(); void doOTAUpdate();
void sonarTask(void* pvParameters);
void handleLEDButton(); void handleIR();
void drawOLED();
void drawWeather(); void drawEyes(); void drawDigitalClock();
void drawAnalogClock(); void drawOverview(); void drawWebUI();
void drawPeople(); void drawStartupIP();
void nextEnabledScreen(); void centreStr(const char* s, int y);
String buildHomePage(); String buildSettingsPage(); String buildStatusJSON();
void saveSettings(); void loadSettings();
void fillEye(int cx,int cy,int w,int h,int r);
void drawPupil(int cx,int cy,int px,int py,int pr);
void drawEyeWithLid(int cx,int cy,float close,bool upperOnly);
void drawEmo_Front(); void drawEmo_Blink(float t); void drawEmo_Sleep();
void drawEmo_Happy(); void drawEmo_Mad(); void drawEmo_Cry(float t);
void drawEmo_Confused(); void drawEmo_Tired(); void drawEmo_Wide(); void drawEmo_Night();
void drawSunIcon(int cx,int cy); void drawCloudIcon(int cx,int cy);
void handleToggle(); void handleToggleLed(); void handleSaveWiFi();
void handleUseAP(); void handleSaveSettings(); void handleSetPeople();

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200); delay(500);
  Serial.println("\n=== Phyxon Home Automation v2.1 ===");

  pinMode(LED_BTN, OUTPUT);
  digitalWrite(LED_BTN, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();
  u8g2.setContrast(220);

  preferences.begin("phyxon", false);
  loadSettings();
  for(int i=0;i<SCREEN_COUNT;i++)
    screenEnabled[i] = preferences.getBool(("se"+String(i)).c_str(), true);

  initGPIO();
  applyAllStates();

  sonarActive = false;
  for(int i=0;i<8;i++) if(autoOffEnabled[i]) { sonarActive=true; break; }

  // Always start sonar task on Core 0 (for people counting even without auto-off)
  xTaskCreatePinnedToCore(sonarTask, "SonarTask", 4096, NULL, 1, &sonarTaskHandle, 0);

  IrReceiver.begin(IR_RECV_PIN, DISABLE_LED_FEEDBACK);

  if(!useAPMode && storedSSID.length()>0) connectToWiFi();
  else startAPMode();

  if(MDNS.begin("phyxon")) Serial.println("mDNS: phyxon.local");
  setupRoutes();
  server.begin();

  if(WiFi.getMode()==WIFI_STA && WiFi.status()==WL_CONNECTED) {
    syncNTP(); fetchWeather();
  }

  showingStartupIP = true;
  startupIPTime    = millis();
  emoStart         = millis();
  randomSeed(millis());
  Serial.println("=== Ready ===");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  server.handleClient();
  handleLEDButton();
  handleIR();

  // Delayed auto-off: fire if timer expired and count still zero
  if(pendingOff && peopleCount == 0 &&
     millis() - pendingOffTime >= (unsigned long)(autoOffDelaySec * 1000)) {
    pendingOff = false;
    for(int i=0;i<8;i++) if(autoOffEnabled[i] && relayStates[i]) {
      relayStates[i]=false; digitalWrite(relayPins[i],HIGH);
      preferences.putBool(("r"+String(i)).c_str(),false);
      autoOffedByPresence[i]=true;
    }
    Serial.println("Delayed OFF executed");
  }
  // Cancel pending off if someone came back
  if(pendingOff && peopleCount > 0) {
    pendingOff = false;
    Serial.println("Pending OFF cancelled, room occupied");
  }

  // Delayed auto-on: fire if timer expired and count still > 0
  if(pendingOn && peopleCount > 0 &&
     millis() - pendingOnTime >= (unsigned long)(autoOnDelaySec * 1000)) {
    pendingOn = false;
    for(int i=0;i<8;i++) if(autoOffedByPresence[i]) {
      relayStates[i]=true; digitalWrite(relayPins[i],LOW);
      preferences.putBool(("r"+String(i)).c_str(),true);
      autoOffedByPresence[i]=false;
    }
    Serial.println("Delayed ON executed");
  }
  // Cancel pending on if everyone left again
  if(pendingOn && peopleCount == 0) {
    pendingOn = false;
    Serial.println("Pending ON cancelled, room empty");
  }

  if(millis()-lastOLEDDraw > OLED_REFRESH_MS) {
    lastOLEDDraw=millis(); drawOLED();
  }

  if(autoSwitch && !oledInWebUI && !showingStartupIP) {
    if(millis()-lastScreenSwitch > (unsigned long)(screenInterval*1000)) {
      lastScreenSwitch=millis(); nextEnabledScreen();
    }
  }

  if(WiFi.status()==WL_CONNECTED && millis()-lastWeatherFetch>WEATHER_INTERVAL)
    fetchWeather();

  if(digitalRead(RESET_PIN)==LOW) { delay(50); if(digitalRead(RESET_PIN)==LOW) factoryReset(); }

  if(showingStartupIP && millis()-startupIPTime>STARTUP_IP_DURATION)
    showingStartupIP=false;
}

// ============================================================
// GPIO
// ============================================================
void initGPIO() {
  for(int i=0;i<8;i++) pinMode(relayPins[i],OUTPUT);
  pinMode(RESET_PIN,INPUT_PULLUP);
  pinMode(LED_CTRL,OUTPUT);
}
void applyAllStates() {
  for(int i=0;i<8;i++) digitalWrite(relayPins[i], relayStates[i]?LOW:HIGH);
  digitalWrite(LED_CTRL, ledState?LOW:HIGH);
}

// ============================================================
// WIFI
// ============================================================
void startAPMode() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ap_ip,ap_gateway,ap_subnet);
  WiFi.softAP(ap_ssid,ap_password);
  Serial.println("AP @ 192.168.4.1");
}
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.config(static_ip,gateway_ip,subnet_ip,dns_ip);
  WiFi.begin(storedSSID.c_str(),storedPass.c_str());
  Serial.print("Connecting");
  int att=0;
  while(WiFi.status()!=WL_CONNECTED && att<30) { delay(500); Serial.print("."); att++; }
  if(WiFi.status()==WL_CONNECTED) Serial.println("\nConnected: "+WiFi.localIP().toString());
  else { Serial.println("\nFailed, fallback AP"); startAPMode(); }
}

// ============================================================
// SETTINGS
// ============================================================
void loadSettings() {
  storedSSID     = preferences.getString("ssid","");
  storedPass     = preferences.getString("pass","");
  useAPMode      = preferences.getBool("useAP",true);
  ledState       = preferences.getBool("led",true);
  darkMode       = preferences.getBool("dark",true);
  weatherCity    = preferences.getString("city","Dhaka");
  timezone_offset= preferences.getInt("tz",6);
  peopleCount    = preferences.getInt("people",1);
  autoSwitch     = preferences.getBool("autoSw",true);
  screenInterval = preferences.getInt("scrInt",5);
  fixedScreen    = preferences.getInt("fixedScr",SCREEN_EYES);
  currentScreen  = autoSwitch ? SCREEN_EYES : fixedScreen;
  sonarDetectDist= preferences.getInt("snrDist", 60);
  sonarConfirmWin= preferences.getInt("snrWin",  2000);
  autoOffDelaySec= preferences.getInt("aoDelay", 0);
  autoOnDelaySec = preferences.getInt("aonDelay",0);
  for(int i=0;i<8;i++) {
    relayStates[i]   = preferences.getBool(("r"+String(i)).c_str(),false);
    autoOffEnabled[i]= preferences.getBool(("ao"+String(i)).c_str(),false);
  }
}
void saveSettings() {
  preferences.putBool("useAP",useAPMode); preferences.putBool("led",ledState);
  preferences.putBool("dark",darkMode);   preferences.putString("city",weatherCity);
  preferences.putInt("tz",timezone_offset); preferences.putInt("people",peopleCount);
  preferences.putBool("autoSw",autoSwitch); preferences.putInt("scrInt",screenInterval);
  preferences.putInt("fixedScr",fixedScreen);
  preferences.putInt("snrDist", sonarDetectDist);
  preferences.putInt("snrWin",  sonarConfirmWin);
  preferences.putInt("aoDelay", autoOffDelaySec);
  preferences.putInt("aonDelay",autoOnDelaySec);
  for(int i=0;i<8;i++) {
    preferences.putBool(("r"+String(i)).c_str(),relayStates[i]);
    preferences.putBool(("ao"+String(i)).c_str(),autoOffEnabled[i]);
  }
  for(int i=0;i<SCREEN_COUNT;i++)
    preferences.putBool(("se"+String(i)).c_str(),screenEnabled[i]);
}
void factoryReset() {
  Serial.println("FACTORY RESET");
  preferences.clear(); delay(500); ESP.restart();
}

// ============================================================
// NTP
// ============================================================
void syncNTP() {
  configTime(timezone_offset*3600,0,"pool.ntp.org","time.nist.gov");
  Serial.print("NTP sync");
  struct tm t; int tr=0;
  while(!getLocalTime(&t)&&tr<20){delay(500);Serial.print(".");tr++;}
  Serial.println(getLocalTime(&t)?" OK":" FAIL");
}

// ============================================================
// WEATHER
// ============================================================
void fetchWeather() {
  if(WiFi.status()!=WL_CONNECTED) return;
  lastWeatherFetch=millis();
  HTTPClient http;
  String url="http://api.openweathermap.org/data/2.5/weather?q="+
             weatherCity+"&appid=YOURAPIKEYHERE&units=metric";
  http.begin(url); http.setTimeout(8000);
  if(http.GET()==200) {
    String payload=http.getString();
    JsonDocument doc;
    if(!deserializeJson(doc,payload)) {
      wx.temp       =doc["main"]["temp"].as<float>();
      wx.humidity   =doc["main"]["humidity"].as<int>();
      wx.description=doc["weather"][0]["description"].as<String>();
      wx.isCloudy   =(String(doc["weather"][0]["main"].as<const char*>())!="Clear");
      wx.fetched    =true;
      Serial.printf("Weather:%.1fC %d%% %s\n",wx.temp,wx.humidity,wx.description.c_str());
    }
  }
  http.end();
}

// ============================================================
// OTA (manual trigger from web button)
// ============================================================
void doOTAUpdate() {
  if(WiFi.status()!=WL_CONNECTED) { otaStatusMsg="No WiFi"; return; }
  otaStatusMsg="Checking server...";
  HTTPClient http;
  http.begin("http://"+String(OTA_HOST)+":"+String(OTA_PORT)+"/version");
  int code=http.GET();
  if(code!=200) { otaStatusMsg="Server unreachable (HTTP "+String(code)+")"; http.end(); return; }
  String sv=http.getString(); sv.trim(); http.end();
  if(sv==String(FW_VERSION)) { otaStatusMsg="Already up to date (v"+String(FW_VERSION)+")"; return; }
  otaStatusMsg="Updating to v"+sv+"...";
  saveSettings();
  String binUrl="http://"+String(OTA_HOST)+":"+String(OTA_PORT)+"/firmware.bin";
  WiFiClient client;
  t_httpUpdate_return ret=httpUpdate.update(client, binUrl);
  switch(ret) {
    case HTTP_UPDATE_FAILED:    otaStatusMsg="Failed: "+httpUpdate.getLastErrorString(); break;
    case HTTP_UPDATE_NO_UPDATES:otaStatusMsg="No updates found"; break;
    case HTTP_UPDATE_OK:        otaStatusMsg="Updated! Rebooting..."; break;
  }
}

// ============================================================
// LED BUTTON
// ============================================================
void handleLEDButton() {
  pinMode(LED_BTN,INPUT);
  bool pressed=(digitalRead(LED_BTN)==LOW);
  pinMode(LED_BTN,OUTPUT); digitalWrite(LED_BTN,HIGH);
  if(pressed && millis()-ledBtnLastPress>DEBOUNCE_MS) {
    ledBtnLastPress=millis();
    ledState=!ledState;
    digitalWrite(LED_CTRL,ledState?LOW:HIGH);
    preferences.putBool("led",ledState);
  }
}

// ============================================================
// SONAR TASK — runs on Core 0, alternating fire to prevent crosstalk
// ============================================================
void sonarTask(void* pvParameters) {
  while(true) {
    unsigned long now = millis();

    // Alternate: fire S1 then S2, never simultaneously
    unsigned int dist = 0;
    if(sonarTurn == 0) {
      dist = sonar1.ping_cm();
      bool det = (dist > 0 && dist <= sonarDetectDist);

      portENTER_CRITICAL(&sonarMux);
      if(det){ sonar1Live=true; sonar1LiveTime=now; }
      else if(now-sonar1LiveTime>SONAR_LIVE_DISPLAY_MS) sonar1Live=false;

      if(now-lastCrossingTime >= SONAR_COOLDOWN) {
        if(det && !sonar1Triggered){
          sonar1Triggered=true; sonar1TrigTime=now;
          Serial.println("S1 trig d="+String(dist)+"cm");
        }
      }
      portEXIT_CRITICAL(&sonarMux);
      sonarTurn = 1;

    } else {
      dist = sonar2.ping_cm();
      bool det = (dist > 0 && dist <= sonarDetectDist);

      portENTER_CRITICAL(&sonarMux);
      if(det){ sonar2Live=true; sonar2LiveTime=now; }
      else if(now-sonar2LiveTime>SONAR_LIVE_DISPLAY_MS) sonar2Live=false;

      if(now-lastCrossingTime >= SONAR_COOLDOWN) {
        if(det && !sonar2Triggered){
          sonar2Triggered=true; sonar2TrigTime=now;
          Serial.println("S2 trig d="+String(dist)+"cm");
        }
      }
      portEXIT_CRITICAL(&sonarMux);
      sonarTurn = 0;
    }

    // Check for complete crossing
    portENTER_CRITICAL(&sonarMux);
    if(sonar1Triggered && sonar2Triggered) {
      unsigned long diff = (sonar1TrigTime < sonar2TrigTime)
                           ? sonar2TrigTime - sonar1TrigTime
                           : sonar1TrigTime - sonar2TrigTime;

      if(diff < (unsigned long)sonarConfirmWin) {
        if(sonar1TrigTime < sonar2TrigTime) {
          // S1 first = ENTRY
          peopleCount++;
          preferences.putInt("people", peopleCount);
          // Cancel any pending turn-off
          pendingOff = false;
          // If coming from zero, start turn-on timer (or act immediately if delay=0)
          if(peopleCount == 1 && !pendingOn) {
            if(autoOnDelaySec == 0) {
              for(int i=0;i<8;i++) if(autoOffedByPresence[i]) {
                relayStates[i]=true; digitalWrite(relayPins[i],LOW);
                preferences.putBool(("r"+String(i)).c_str(),true);
                autoOffedByPresence[i]=false;
              }
            } else {
              pendingOn     = true;
              pendingOnTime = millis();
            }
          }
          Serial.println("ENTRY people="+String(peopleCount));
        } else {
          // S2 first = EXIT
          if(peopleCount > 0) peopleCount--;
          preferences.putInt("people", peopleCount);
          // Cancel any pending turn-on
          pendingOn = false;
          // If now zero, start turn-off timer (or act immediately if delay=0)
          if(peopleCount == 0) {
            if(autoOffDelaySec == 0) {
              for(int i=0;i<8;i++) if(autoOffEnabled[i]&&relayStates[i]) {
                relayStates[i]=false; digitalWrite(relayPins[i],HIGH);
                preferences.putBool(("r"+String(i)).c_str(),false);
                autoOffedByPresence[i]=true;
              }
            } else {
              pendingOff     = true;
              pendingOffTime = millis();
            }
          }
          Serial.println("EXIT people="+String(peopleCount));
        }
        lastCrossingTime = now;
      }
      sonar1Triggered = sonar2Triggered = false;
    }

    // Timeout stale single triggers
    if(sonar1Triggered && now-sonar1TrigTime > (unsigned long)sonarConfirmWin) {
      Serial.println("S1 timeout ignored");
      sonar1Triggered = false;
    }
    if(sonar2Triggered && now-sonar2TrigTime > (unsigned long)sonarConfirmWin) {
      Serial.println("S2 timeout ignored");
      sonar2Triggered = false;
    }
    portEXIT_CRITICAL(&sonarMux);

    vTaskDelay(pdMS_TO_TICKS(SONAR_FIRE_INTERVAL));
  }
}

// ============================================================
// IR REMOTE
// ============================================================
void handleIR() {
  if(!IrReceiver.decode()) return;
  uint8_t cmd=IrReceiver.decodedIRData.command;
  IrReceiver.resume();

  if(oledInWebUI) {
    switch(cmd) {
      case IR_UP:    if(oledCursor>0) oledCursor--; break;
      case IR_DOWN:  oledCursor++; break;
      case IR_LEFT:  if(oledTab>0){oledTab--;oledCursor=0;} break;
      case IR_RIGHT: if(oledTab<1){oledTab++;oledCursor=0;} break;
      case IR_SLASH:
      case IR_A:
        oledInWebUI=false;
        currentScreen=(!autoSwitch)?fixedScreen:screenBeforeWebUI;
        break;
      case IR_C: oledTab=(oledTab+1)%2; oledCursor=0; break;
      case IR_OK:
        if(oledTab==0&&oledCursor<8) {
          relayStates[oledCursor]=!relayStates[oledCursor];
          digitalWrite(relayPins[oledCursor],relayStates[oledCursor]?LOW:HIGH);
          preferences.putBool(("r"+String(oledCursor)).c_str(),relayStates[oledCursor]);
        }
        break;
      case IR_1:case IR_2:case IR_3:case IR_4:
      case IR_5:case IR_6:case IR_7:case IR_8:case IR_9:
        oledCursor=cmd-1; break;
    }
    return;
  }

  // Normal mode
  switch(cmd) {
    case IR_1:case IR_2:case IR_3:case IR_4:
    case IR_5:case IR_6:case IR_7:case IR_8: {
      int idx=cmd-1;
      relayStates[idx]=!relayStates[idx];
      digitalWrite(relayPins[idx],relayStates[idx]?LOW:HIGH);
      preferences.putBool(("r"+String(idx)).c_str(),relayStates[idx]);
      break;
    }
    case IR_PWR:
      for(int i=0;i<8;i++){relayStates[i]=true;digitalWrite(relayPins[i],LOW);preferences.putBool(("r"+String(i)).c_str(),true);}
      break;
    case IR_B:
      for(int i=0;i<8;i++){relayStates[i]=false;digitalWrite(relayPins[i],HIGH);preferences.putBool(("r"+String(i)).c_str(),false);}
      break;
    case IR_A:
      screenBeforeWebUI=currentScreen;
      oledInWebUI=true; oledTab=0; oledCursor=0;
      currentScreen=SCREEN_WEBUI;
      break;
    case IR_C:
      screenBeforeWebUI=currentScreen;
      oledInWebUI=true; oledTab=1; oledCursor=0;
      currentScreen=SCREEN_WEBUI;
      break;
    case IR_D:
      nextEnabledScreen();
      fixedScreen=currentScreen; autoSwitch=false;
      preferences.putBool("autoSw",false); preferences.putInt("fixedScr",fixedScreen);
      break;
    case IR_E:
      nextEnabledScreen(); autoSwitch=true;
      preferences.putBool("autoSw",true);
      break;
    case IR_H:
      ledState=!ledState; digitalWrite(LED_CTRL,ledState?LOW:HIGH);
      preferences.putBool("led",ledState);
      break;
    case IR_I:
      if(peopleCount>0){ peopleCount--; preferences.putInt("people",peopleCount); }
      break;
    case IR_J:
      peopleCount++; preferences.putInt("people",peopleCount);
      break;
    case IR_K: factoryReset(); break;
  }
}

// ============================================================
// OLED HELPERS
// ============================================================
void nextEnabledScreen() {
  int tries=0;
  do {
    currentScreen=(currentScreen+1)%SCREEN_COUNT;
    tries++;
    if(currentScreen==SCREEN_WEBUI) continue;
  } while(!screenEnabled[currentScreen]&&tries<SCREEN_COUNT);
}

void centreStr(const char* str, int y) {
  int w=u8g2.getStrWidth(str);
  u8g2.drawStr((128-w)/2, y, str);
}

void drawOLED() {
  u8g2.clearBuffer();
  if(showingStartupIP){drawStartupIP();u8g2.sendBuffer();return;}
  if(oledInWebUI||currentScreen==SCREEN_WEBUI) drawWebUI();
  else switch(currentScreen) {
    case SCREEN_WEATHER:  drawWeather();      break;
    case SCREEN_EYES:     drawEyes();         break;
    case SCREEN_CLOCK:    drawDigitalClock(); break;
    case SCREEN_ANALOG:   drawAnalogClock();  break;
    case SCREEN_OVERVIEW: drawOverview();     break;
    case SCREEN_PEOPLE:   drawPeople();       break;
    default:              drawDigitalClock(); break;
  }
  u8g2.sendBuffer();
}

// ============================================================
// SCREEN: STARTUP IP
// ============================================================
void drawStartupIP() {
  String ip=(WiFi.getMode()==WIFI_AP)?WiFi.softAPIP().toString():WiFi.localIP().toString();
  String label=(WiFi.getMode()==WIFI_AP)?"AP Mode":"WiFi OK";
  u8g2.setFont(u8g2_font_6x10_tr);
  centreStr("Phyxon Home Auto",12); centreStr("v2.6",24);
  u8g2.drawHLine(0,27,128);
  centreStr(label.c_str(),40);
  u8g2.setFont(u8g2_font_7x14B_tr);
  centreStr(ip.c_str(),58);
}

// ============================================================
// EYE PRIMITIVES
// ============================================================
void fillEye(int cx,int cy,int w,int h,int r){u8g2.drawRBox(cx-w/2,cy-h/2,w,h,r);}

void drawPupil(int cx,int cy,int px,int py,int pr){
  u8g2.setDrawColor(0); u8g2.drawDisc(cx+px,cy+py,pr);
  u8g2.setDrawColor(1); u8g2.drawDisc(cx+px-pr/3,cy+py-pr/3,2);
}

void drawEyeWithLid(int cx,int cy,float close,bool upperOnly){
  fillEye(cx,cy,EW,EH,ER);
  u8g2.setDrawColor(0);
  int lidH=(int)(close*EH);
  if(upperOnly) u8g2.drawBox(cx-EW/2,cy-EH/2,EW,lidH);
  else {
    int half=lidH/2;
    u8g2.drawBox(cx-EW/2,cy-EH/2,EW,half);
    u8g2.drawBox(cx-EW/2,cy+EH/2-half,EW,half);
  }
  u8g2.setDrawColor(1);
}

void drawEmo_Front(){
  unsigned long now=millis();
  if(now-lastPupilPick>1000){pupilTX=random(-6,7);pupilTY=random(-5,6);lastPupilPick=now;}
  pupilX+=(pupilTX-pupilX)*0.15f; pupilY+=(pupilTY-pupilY)*0.15f;
  fillEye(LEX,EYE_CY,EW,EH,ER); fillEye(REX,EYE_CY,EW,EH,ER);
  drawPupil(LEX,EYE_CY,(int)pupilX,(int)pupilY,7);
  u8g2.setDrawColor(0); u8g2.drawDisc(REX+(int)pupilX,EYE_CY+(int)pupilY,7);
  u8g2.setDrawColor(1); u8g2.drawDisc(REX+(int)pupilX-2,EYE_CY+(int)pupilY-2,2);
}

void drawEmo_Blink(float phase){
  float close=(phase<0.5f)?phase*2.0f:(1.0f-phase)*2.0f;
  drawEyeWithLid(LEX,EYE_CY,close,false);
  drawEyeWithLid(REX,EYE_CY,close,false);
}

void drawEmo_Sleep(){
  drawEyeWithLid(LEX,EYE_CY,0.6f,true); drawEyeWithLid(REX,EYE_CY,0.6f,true);
  u8g2.setFont(u8g2_font_6x10_tr);  u8g2.drawStr(106,20,"z");
  u8g2.setFont(u8g2_font_8x13_tr);  u8g2.drawStr(112,13,"z");
  u8g2.setFont(u8g2_font_10x20_tr); u8g2.drawStr(118,5, "z");
}

void drawEmo_Happy(){
  fillEye(LEX,EYE_CY,EW,EH,ER); fillEye(REX,EYE_CY,EW,EH,ER);
  u8g2.setDrawColor(0);
  u8g2.drawBox(LEX-EW/2-1,EYE_CY-EH/2-1,EW+2,EH/2+2);
  u8g2.drawBox(REX-EW/2-1,EYE_CY-EH/2-1,EW+2,EH/2+2);
  u8g2.setDrawColor(1);
  for(int x=-16;x<=16;x++){int y=(x*x)/20;u8g2.drawPixel(64+x,54+y);u8g2.drawPixel(64+x,55+y);}
}

void drawEmo_Mad(){
  fillEye(LEX,EYE_CY,EW,EH,ER); fillEye(REX,EYE_CY,EW,EH,ER);
  u8g2.setDrawColor(0);
  for(int i=0;i<8;i++){
    u8g2.drawLine(LEX-EW/2,EYE_CY-EH/2+i,LEX+EW/2,EYE_CY-EH/2);
    u8g2.drawLine(REX+EW/2,EYE_CY-EH/2+i,REX-EW/2,EYE_CY-EH/2);
  }
  u8g2.drawDisc(LEX,EYE_CY+3,6); u8g2.drawDisc(REX,EYE_CY+3,6);
  u8g2.setDrawColor(1);
}

void drawEmo_Cry(float t){
  fillEye(LEX,EYE_CY,EW,EH,ER); fillEye(REX,EYE_CY,EW,EH,ER);
  drawPupil(LEX,EYE_CY,0,2,6);
  u8g2.setDrawColor(0); u8g2.drawDisc(REX,EYE_CY+2,6);
  u8g2.setDrawColor(1); u8g2.drawDisc(REX-2,EYE_CY-1,2);
  int td=(int)(t*20)%24;
  u8g2.drawBox(LEX-2,EYE_CY+EH/2+td,4,6); u8g2.drawBox(REX-2,EYE_CY+EH/2+td,4,6);
  u8g2.drawLine(LEX-EW/2,EYE_CY-EH/2-5,LEX+EW/2,EYE_CY-EH/2-2);
  u8g2.drawLine(REX-EW/2,EYE_CY-EH/2-2,REX+EW/2,EYE_CY-EH/2-5);
}

void drawEmo_Confused(){
  fillEye(LEX,EYE_CY,EW,EH,ER); fillEye(REX,EYE_CY,EW,EH-10,ER-2);
  drawPupil(LEX,EYE_CY,0,0,7);
  u8g2.setDrawColor(0); u8g2.drawDisc(REX,EYE_CY,5);
  u8g2.setDrawColor(1); u8g2.drawDisc(REX-2,EYE_CY-2,2);
  u8g2.setFont(u8g2_font_10x20_tr); u8g2.drawStr(110,30,"?");
}

void drawEmo_Tired(){
  drawEyeWithLid(LEX,EYE_CY,0.4f,true); drawEyeWithLid(REX,EYE_CY,0.4f,true);
  u8g2.setDrawColor(0);
  u8g2.drawDisc(LEX,EYE_CY+4,5); u8g2.drawDisc(REX,EYE_CY+4,5);
  u8g2.setDrawColor(1);
}

void drawEmo_Wide(){
  fillEye(LEX,EYE_CY,EW+6,EH+4,ER); fillEye(REX,EYE_CY,EW+6,EH+4,ER);
  drawPupil(LEX,EYE_CY,0,0,6);
  u8g2.setDrawColor(0); u8g2.drawDisc(REX,EYE_CY,6);
  u8g2.setDrawColor(1); u8g2.drawDisc(REX-2,EYE_CY-2,2);
}

void drawEmo_Night(){
  u8g2.drawDisc(LEX,EYE_CY,14);
  u8g2.setDrawColor(0); u8g2.drawDisc(LEX+5,EYE_CY-3,11); u8g2.setDrawColor(1);
  u8g2.drawDisc(REX,EYE_CY,14);
  u8g2.setDrawColor(0); u8g2.drawDisc(REX+5,EYE_CY-3,11); u8g2.setDrawColor(1);
  u8g2.drawPixel(10,10);u8g2.drawPixel(12,8);u8g2.drawPixel(10,6);
  u8g2.drawPixel(118,10);u8g2.drawPixel(120,7);u8g2.drawPixel(116,7);
}

// ============================================================
// SCREEN: ROBOT EYES
// ============================================================
void drawEyes(){
  unsigned long now=millis();
  if(now-emoStart>emoDurations[emoSeqIdx]){
    emoSeqIdx=(emoSeqIdx+1)%emoSeqLen;
    currentEmo=emoSequence[emoSeqIdx]; emoStart=now;
  }
  float t=(float)(now-emoStart)/(float)emoDurations[emoSeqIdx];
  switch(currentEmo){
    case EMO_FRONT:    drawEmo_Front();     break;
    case EMO_BLINK:    drawEmo_Blink(t);   break;
    case EMO_SLEEP:    drawEmo_Sleep();    break;
    case EMO_HAPPY:    drawEmo_Happy();    break;
    case EMO_MAD:      drawEmo_Mad();      break;
    case EMO_CRY:      drawEmo_Cry(t);    break;
    case EMO_CONFUSED: drawEmo_Confused(); break;
    case EMO_TIRED:    drawEmo_Tired();    break;
    case EMO_WIDE:     drawEmo_Wide();     break;
    case EMO_NIGHT:    drawEmo_Night();    break;
    default: break;
  }
}

// ============================================================
// SCREEN: WEATHER
// ============================================================
void drawSunIcon(int cx,int cy){
  u8g2.drawDisc(cx,cy,9);
  for(int i=0;i<8;i++){float a=i*3.14159f/4.0f;u8g2.drawLine(cx+cos(a)*12,cy+sin(a)*12,cx+cos(a)*16,cy+sin(a)*16);}
}
void drawCloudIcon(int cx,int cy){
  u8g2.drawDisc(cx-6,cy,7); u8g2.drawDisc(cx+2,cy-4,9); u8g2.drawDisc(cx+10,cy,6);
  u8g2.drawBox(cx-13,cy,30,9);
  if((int)cloudOffset%20>10){u8g2.drawPixel(cx-6,cy+14);u8g2.drawPixel(cx+2,cy+16);u8g2.drawPixel(cx+10,cy+14);}
}
void drawWeather(){
  unsigned long now=millis();
  if(now-lastCloudUpdate>60){cloudOffset+=0.5;if(cloudOffset>128)cloudOffset=-30;lastCloudUpdate=now;}
  if(!wx.fetched){
    u8g2.setFont(u8g2_font_6x10_tr);
    centreStr("No weather data",24); centreStr("Check WiFi",38); return;
  }
  u8g2.setFont(u8g2_font_6x10_tr);
  centreStr((weatherCity+", BD").c_str(),10);
  u8g2.drawHLine(0,13,128);
  u8g2.setFont(u8g2_font_ncenB18_tr);
  String ts=String((int)round(wx.temp))+"\xb0""C";
  u8g2.drawStr(2,38,ts.c_str());
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(2,48,wx.description.c_str());
  u8g2.drawStr(2,57,("Hum:"+String(wx.humidity)+"%").c_str());
  int ix=96+(int)(cloudOffset/8)%6-3;
  if(wx.isCloudy) drawCloudIcon(ix,32); else drawSunIcon(ix,32);
}

// ============================================================
// SCREEN: DIGITAL CLOCK
// ============================================================
void drawDigitalClock(){
  struct tm t; if(!getLocalTime(&t)){u8g2.setFont(u8g2_font_6x10_tr);centreStr("Syncing...",32);return;}
  char tb[12],db[16],dy[12];
  strftime(tb,sizeof(tb),"%I:%M:%S",&t);
  strftime(db,sizeof(db),"%d %b %Y",&t);
  strftime(dy,sizeof(dy),"%A",&t);
  u8g2.setFont(u8g2_font_ncenB18_tr); centreStr(tb,26);
  u8g2.drawHLine(14,30,100);
  u8g2.setFont(u8g2_font_6x10_tr); centreStr(db,44);
  u8g2.setFont(u8g2_font_5x7_tr);  centreStr(dy,55);
}

// ============================================================
// SCREEN: ANALOG CLOCK
// ============================================================
void drawAnalogClock(){
  struct tm t; if(!getLocalTime(&t)){u8g2.setFont(u8g2_font_6x10_tr);centreStr("Syncing...",32);return;}
  int cx=64,cy=30,R=28;
  u8g2.drawCircle(cx,cy,R); u8g2.drawCircle(cx,cy,R-1);
  for(int i=0;i<12;i++){
    float a=i*3.14159f/6.0f-3.14159f/2.0f; int len=(i%3==0)?5:3;
    u8g2.drawLine(cx+cos(a)*(R-1),cy+sin(a)*(R-1),cx+cos(a)*(R-1-len),cy+sin(a)*(R-1-len));
  }
  float sA=t.tm_sec*3.14159f/30.0f-3.14159f/2.0f;
  float mA=(t.tm_min+t.tm_sec/60.0f)*3.14159f/30.0f-3.14159f/2.0f;
  float hA=((t.tm_hour%12)+t.tm_min/60.0f)*3.14159f/6.0f-3.14159f/2.0f;
  u8g2.drawLine(cx,cy,cx+cos(hA)*(R-12),cy+sin(hA)*(R-12));
  u8g2.drawLine(cx+1,cy,cx+1+cos(hA)*(R-12),cy+sin(hA)*(R-12));
  u8g2.drawLine(cx,cy,cx+cos(mA)*(R-6),cy+sin(mA)*(R-6));
  u8g2.drawLine(cx,cy,cx+cos(sA)*(R-3),cy+sin(sA)*(R-3));
  u8g2.drawDisc(cx,cy,2);
  char tb[6]; strftime(tb,sizeof(tb),"%I:%M",&t);
  u8g2.setFont(u8g2_font_6x10_tr); centreStr(tb,62);
}

// ============================================================
// SCREEN: RELAY OVERVIEW
// ============================================================
void drawOverview(){
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(0,8,"Relay Overview"); u8g2.drawHLine(0,10,128);
  for(int i=0;i<8;i++){
    int col=(i<4)?0:64, y=20+(i%4)*12; char buf[16];
    snprintf(buf,sizeof(buf),"SW%d %s",i+1,relayStates[i]?"[ON]":"[--]");
    u8g2.drawStr(col+2,y,buf);
  }
  u8g2.drawStr(0,62,ledState?"LED:ON":"LED:OFF");
}

// ============================================================
// SCREEN: WEBUI REPLICA
// ============================================================
void drawWebUI(){
  u8g2.setFont(u8g2_font_5x7_tr);
  const char* tabs[]={"Home","Settings"};
  for(int i=0;i<2;i++){
    if(i==oledTab){
      u8g2.drawBox(i*64,0,64,10); u8g2.setDrawColor(0);
      u8g2.drawStr(i*64+4,8,tabs[i]); u8g2.setDrawColor(1);
    } else {
      u8g2.drawFrame(i*64,0,64,10); u8g2.drawStr(i*64+4,8,tabs[i]);
    }
  }
  u8g2.drawHLine(0,11,128);
  if(oledTab==0){
    for(int i=0;i<8;i++){
      int y=20+i*7; if(y>62) break;
      char buf[24]; snprintf(buf,sizeof(buf),"%d. SW%d%s",i+1,i+1,relayStates[i]?" *ON":"OFF");
      if(oledCursor==i){
        u8g2.drawBox(0,y-6,128,8); u8g2.setDrawColor(0);
        u8g2.drawStr(2,y,buf); u8g2.setDrawColor(1);
      } else u8g2.drawStr(2,y,buf);
    }
  } else {
    const char* items[]={"1.LED","2.Dark","3.City","4.TZ","5.Screen","6.Interval","7.AutoOff","8.People"};
    if(oledCursor>7) oledCursor=7;
    for(int i=0;i<8;i++){
      int y=20+i*7; if(y>62) break;
      char buf[32];
      if(i==0)snprintf(buf,32,"%s:%s",items[i],ledState?"ON":"OFF");
      else if(i==1)snprintf(buf,32,"%s:%s",items[i],darkMode?"ON":"OFF");
      else if(i==2)snprintf(buf,32,"%s:%s",items[i],weatherCity.substring(0,7).c_str());
      else if(i==3)snprintf(buf,32,"%s:UTC+%d",items[i],timezone_offset);
      else if(i==4)snprintf(buf,32,"%s:%s",items[i],autoSwitch?"Auto":"Fixed");
      else if(i==5)snprintf(buf,32,"%s:%ds",items[i],screenInterval);
      else if(i==6)snprintf(buf,32,"%s",items[i]);
      else snprintf(buf,32,"%s:%d",items[i],peopleCount);
      if(oledCursor==i){
        u8g2.drawBox(0,y-6,128,8); u8g2.setDrawColor(0);
        u8g2.drawStr(2,y,buf); u8g2.setDrawColor(1);
      } else u8g2.drawStr(2,y,buf);
    }
  }
}

// ============================================================
// SCREEN: PEOPLE COUNT
// ============================================================
void drawPeople(){
  u8g2.setFont(u8g2_font_6x10_tr); centreStr("Room Occupancy",12);
  u8g2.drawHLine(0,14,128);
  u8g2.setFont(u8g2_font_logisoso32_tr);
  char buf[8]; snprintf(buf,sizeof(buf),"%d",peopleCount);
  centreStr(buf,56);
  u8g2.setFont(u8g2_font_5x7_tr); centreStr("people in room",63);
}

// ============================================================
// WEB SERVER ROUTES
// ============================================================
void setupRoutes(){
  server.on("/",             HTTP_GET,  [](){ server.send(200,"text/html",buildHomePage()); });
  server.on("/settings",     HTTP_GET,  [](){ server.send(200,"text/html",buildSettingsPage()); });
  server.on("/status",       HTTP_GET,  [](){ server.send(200,"application/json",buildStatusJSON()); });
  server.on("/toggle",       HTTP_POST, handleToggle);
  server.on("/toggleled",    HTTP_POST, handleToggleLed);
  server.on("/savewifi",     HTTP_POST, handleSaveWiFi);
  server.on("/useap",        HTTP_POST, handleUseAP);
  server.on("/savesettings", HTTP_POST, handleSaveSettings);
  server.on("/setpeople",    HTTP_POST, handleSetPeople);
  server.on("/sonarstatus",    HTTP_GET,  [](){ 
    String j="{\"s1\":"+String(sonar1Live?"true":"false")+",\"s2\":"+String(sonar2Live?"true":"false")+"}";
    server.send(200,"application/json",j); 
  });
  server.on("/doupdate",     HTTP_POST, [](){ server.send(200,"text/plain","Checking..."); doOTAUpdate(); });
  server.on("/otastatus",    HTTP_GET,  [](){ server.send(200,"text/plain",otaStatusMsg); });
  server.on("/factoryreset", HTTP_POST, [](){ server.send(200,"text/plain","Resetting..."); delay(500); factoryReset(); });
  server.onNotFound([](){server.send(404,"text/plain","Not Found");});
}

void handleToggle(){
  if(server.hasArg("relay")){
    int r=server.arg("relay").toInt();
    if(r>=0&&r<8){
      relayStates[r]=!relayStates[r];
      digitalWrite(relayPins[r],relayStates[r]?LOW:HIGH);
      preferences.putBool(("r"+String(r)).c_str(),relayStates[r]);
      server.send(200,"application/json",buildStatusJSON()); return;
    }
  }
  server.send(400,"text/plain","Bad Request");
}
void handleToggleLed(){
  ledState=!ledState; digitalWrite(LED_CTRL,ledState?LOW:HIGH);
  preferences.putBool("led",ledState);
  server.send(200,"application/json",buildStatusJSON());
}
void handleSaveWiFi(){
  if(server.hasArg("ssid")){
    storedSSID=server.arg("ssid"); storedPass=server.arg("password");
    preferences.putString("ssid",storedSSID); preferences.putString("pass",storedPass);
    preferences.putBool("useAP",false); useAPMode=false;
    String html="<html><body style='font-family:sans-serif;text-align:center;padding:40px'>";
    html+="<h2>Saved! Connecting...</h2><p>Find device at: http://"+static_ip.toString()+"</p></body></html>";
    server.send(200,"text/html",html); delay(2000); ESP.restart();
  }
}
void handleUseAP(){
  preferences.putBool("useAP",true); useAPMode=true;
  server.send(200,"text/html","<html><body><h2>Switching to AP...</h2></body></html>");
  delay(2000); ESP.restart();
}
void handleSaveSettings(){
  if(server.hasArg("dark"))   darkMode=server.arg("dark")=="1";
  if(server.hasArg("city"))   {weatherCity=server.arg("city");if(weatherCity.length()==0)weatherCity="Dhaka";}
  if(server.hasArg("tz"))     timezone_offset=server.arg("tz").toInt();
  if(server.hasArg("led"))    {ledState=server.arg("led")=="1";digitalWrite(LED_CTRL,ledState?LOW:HIGH);}
  if(server.hasArg("autosw")) autoSwitch=server.arg("autosw")=="1";
  if(server.hasArg("scrint")) screenInterval=constrain(server.arg("scrint").toInt(),2,120);
  if(server.hasArg("fixedscr"))fixedScreen=server.arg("fixedscr").toInt();
  if(server.hasArg("people")) {int c=server.arg("people").toInt();if(c>=0)peopleCount=c;}
  for(int i=0;i<SCREEN_COUNT;i++) screenEnabled[i]=server.hasArg("scr"+String(i));
  bool anyE=false; for(int i=0;i<SCREEN_COUNT;i++) if(screenEnabled[i]){anyE=true;break;}
  if(!anyE) screenEnabled[SCREEN_CLOCK]=true;
  if(server.hasArg("snrdist")) sonarDetectDist=constrain(server.arg("snrdist").toInt(),10,300);
  if(server.hasArg("snrwin"))  sonarConfirmWin=constrain(server.arg("snrwin").toInt(),500,5000);
  if(server.hasArg("aodelay")) autoOffDelaySec=constrain(server.arg("aodelay").toInt(),0,300);
  if(server.hasArg("aondelay"))autoOnDelaySec =constrain(server.arg("aondelay").toInt(),0,300);
  bool anyAO=false;
  for(int i=0;i<8;i++){autoOffEnabled[i]=server.hasArg("ao"+String(i));if(autoOffEnabled[i])anyAO=true;}
  sonarActive=anyAO;
  if(!autoSwitch) currentScreen=fixedScreen;
  saveSettings();
  if(WiFi.status()==WL_CONNECTED){syncNTP();fetchWeather();}
  server.sendHeader("Location","/settings"); server.send(303);
}
void handleSetPeople(){
  if(server.hasArg("count")){int c=server.arg("count").toInt();if(c>=0){peopleCount=c;preferences.putInt("people",peopleCount);}}
  server.send(200,"application/json",buildStatusJSON());
}

// ============================================================
// STATUS JSON
// ============================================================
String buildStatusJSON(){
  String j="{\"relays\":[";
  for(int i=0;i<8;i++){j+=relayStates[i]?"true":"false";if(i<7)j+=",";}
  j+= "],\"led\":"    +String(ledState?"true":"false");
  j+=",\"people\":"   +String(peopleCount);
  j+=",\"dark\":"     +String(darkMode?"true":"false");
  j+=",\"mode\":\""  +String(WiFi.getMode()==WIFI_AP?"AP":"STA")+"\"";
  j+=",\"ip\":\""    +(WiFi.getMode()==WIFI_AP?WiFi.softAPIP().toString():WiFi.localIP().toString())+"\"";
  j+=",\"version\":\""+String(FW_VERSION)+"\"";
  j+="}"; return j;
}

// ============================================================
// HOME PAGE
// ============================================================
String buildHomePage(){
  String bg=darkMode?"#1a1a2e":"#f0f4f8";
  String card=darkMode?"#16213e":"#ffffff";
  String text=darkMode?"#e0e0e0":"#333333";
  String sub=darkMode?"#a0a0b0":"#666666";
  String acc="#4ade80", accOff="#444466";
  String bdr=darkMode?"#2a2a4a":"#eeeeee";

  String h="<!DOCTYPE html><html><head>";
  h+="<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h+="<meta charset='UTF-8'><title>Phyxon Home</title><style>";
  h+="*{box-sizing:border-box;margin:0;padding:0;}";
  h+="body{font-family:'Segoe UI',sans-serif;background:"+bg+";color:"+text+";min-height:100vh;padding:16px;}";
  h+=".hdr{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px;}";
  h+=".hdr h1{font-size:1.3rem;font-weight:700;}";
  h+=".tabs{display:flex;gap:8px;margin-bottom:18px;}";
  h+=".tab{padding:8px 22px;border-radius:20px;border:none;cursor:pointer;font-weight:600;font-size:.9rem;transition:.2s;}";
  h+=".tab.on{background:"+acc+";color:#111;} .tab.off{background:"+card+";color:"+sub+";}";
  h+=".card{background:"+card+";border-radius:14px;padding:16px;margin-bottom:12px;box-shadow:0 2px 8px rgba(0,0,0,.15);}";
  h+=String(".row{display:flex;justify-content:space-between;align-items:center;padding:12px 0;border-bottom:1px solid ")+bdr.c_str()+";}";
  h+=".row:last-child{border-bottom:none;} .lbl{font-size:.95rem;font-weight:500;}";
  h+=".tgl{position:relative;width:52px;height:28px;border-radius:28px;cursor:pointer;border:none;outline:none;transition:.3s;}";
  h+=".tgl.on{background:"+acc+";} .tgl.off{background:"+accOff+";}";
  h+=".knob{position:absolute;top:3px;width:22px;height:22px;background:#fff;border-radius:50%;transition:.3s;}";
  h+=".tgl.on .knob{left:27px;} .tgl.off .knob{left:3px;}";
  h+=".pr{display:flex;align-items:center;gap:12px;justify-content:center;padding:10px;}";
  h+=".pn{font-size:2.5rem;font-weight:700;min-width:60px;text-align:center;}";
  h+=".pb{width:38px;height:38px;border-radius:50%;border:none;font-size:1.4rem;cursor:pointer;background:"+card+";color:"+text+";box-shadow:0 1px 4px rgba(0,0,0,.25);}";
  h+=".badge{font-size:.75rem;color:"+sub+";}";
  h+="</style></head><body>";

  h+="<div class='hdr'><h1>&#127968; Phyxon Home</h1>";
  h+="<span class='badge'>"+(WiFi.getMode()==WIFI_AP?WiFi.softAPIP().toString():WiFi.localIP().toString())+" &bull; v"+FW_VERSION+"</span></div>";
  h+="<div class='tabs'><button class='tab on'>Home</button><button class='tab off' onclick='location.href=\"/settings\"'>Settings</button></div>";

  h+="<div class='card'>";
  for(int i=0;i<8;i++){
    h+="<div class='row'><span class='lbl'>Switch "+String(i+1)+"</span>";
    h+="<button class='tgl "+String(relayStates[i]?"on":"off")+"' onclick='tog("+String(i)+")'><div class='knob'></div></button></div>";
  }
  h+="</div>";

  h+="<div class='card'><div class='row'><span class='lbl'>&#128161; LED Indicator</span>";
  h+="<button class='tgl "+String(ledState?"on":"off")+"' onclick='togLed()'><div class='knob'></div></button></div></div>";

  h+="<div class='card'><div class='row'><span class='lbl'>&#128101; People in Room</span></div>";
  h+="<div class='pr'><button class='pb' onclick='ppl(-1)'>&#8722;</button>";
  h+="<span class='pn' id='pc'>"+String(peopleCount)+"</span>";
  h+="<button class='pb' onclick='ppl(1)'>&#43;</button></div></div>";

  h+="<script>";
  h+="function tog(n){fetch('/toggle',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'relay='+n}).then(r=>r.json()).then(()=>location.reload());}";
  h+="function togLed(){fetch('/toggleled',{method:'POST'}).then(r=>r.json()).then(()=>location.reload());}";
  h+="function ppl(d){var c=parseInt(document.getElementById('pc').innerText)+d;if(c<0)c=0;";
  h+="fetch('/setpeople',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'count='+c}).then(r=>r.json()).then(j=>{document.getElementById('pc').innerText=j.people;});}";
  h+="</script></body></html>";
  return h;
}

// ============================================================
// SETTINGS PAGE
// ============================================================
String buildSettingsPage(){
  String bg=darkMode?"#1a1a2e":"#f0f4f8";
  String card=darkMode?"#16213e":"#ffffff";
  String text=darkMode?"#e0e0e0":"#333333";
  String sub=darkMode?"#a0a0b0":"#666666";
  String acc="#4ade80";
  String inp=darkMode?"#0f3460":"#f5f5f5";
  String bdr=darkMode?"#2a2a4a":"#dddddd";

  String h="<!DOCTYPE html><html><head>";
  h+="<meta name='viewport' content='width=device-width,initial-scale=1'>";
  h+="<meta charset='UTF-8'><title>Phyxon Settings</title><style>";
  h+="*{box-sizing:border-box;margin:0;padding:0;}";
  h+="body{font-family:'Segoe UI',sans-serif;background:"+bg+";color:"+text+";padding:16px;}";
  h+=".hdr{margin-bottom:20px;} .hdr h1{font-size:1.3rem;font-weight:700;}";
  h+=".tabs{display:flex;gap:8px;margin-bottom:18px;}";
  h+=".tab{padding:8px 22px;border-radius:20px;border:none;cursor:pointer;font-weight:600;font-size:.9rem;}";
  h+=".tab.on{background:"+acc+";color:#111;} .tab.off{background:"+card+";color:"+sub+";}";
  h+=".card{background:"+card+";border-radius:14px;padding:16px;margin-bottom:14px;box-shadow:0 2px 8px rgba(0,0,0,.15);}";
  h+=".card h2{font-size:.85rem;font-weight:700;margin-bottom:12px;color:"+sub+";text-transform:uppercase;letter-spacing:.5px;}";
  h+=String(".row{display:flex;justify-content:space-between;align-items:center;padding:10px 0;border-bottom:1px solid ")+bdr.c_str()+";}";
  h+=".row:last-child{border-bottom:none;} .lbl{font-size:.9rem;}";
  h+=".tgl{position:relative;width:48px;height:26px;border-radius:26px;cursor:pointer;border:none;outline:none;}";
  h+=".tgl.on{background:"+acc+";} .tgl.off{background:#444466;}";
  h+=".knob{position:absolute;top:3px;width:20px;height:20px;background:#fff;border-radius:50%;transition:.2s;}";
  h+=".tgl.on .knob{left:25px;} .tgl.off .knob{left:3px;}";
  h+="input[type=text],input[type=number],select{background:"+inp+";color:"+text+";border:1px solid "+bdr+";border-radius:8px;padding:7px 10px;font-size:.9rem;width:100%;margin-top:6px;}";
  h+=".cbg{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:8px;}";
  h+=".cbi{display:flex;align-items:center;gap:6px;font-size:.88rem;}";
  h+="input[type=checkbox]{width:16px;height:16px;accent-color:"+acc+";}";
  h+=".btn{width:100%;padding:13px;border:none;border-radius:10px;font-size:.95rem;font-weight:600;cursor:pointer;margin-top:8px;}";
  h+=".bs{background:"+acc+";color:#111;} .bd{background:#e74c3c;color:#fff;} .bw{background:#3498db;color:#fff;} .bu{background:#e67e22;color:#fff;}";
  h+="details summary{cursor:pointer;padding:8px 0;font-weight:600;list-style:none;} details summary::-webkit-details-marker{display:none;}";
  h+="#otamsg{margin-top:8px;font-size:.85rem;color:"+sub+";min-height:18px;}";
  h+="</style></head><body>";

  h+="<div class='hdr'><h1>&#9881; Settings</h1></div>";
  h+="<div class='tabs'><button class='tab off' onclick='location.href=\"/\"'>Home</button><button class='tab on'>Settings</button></div>";

  h+="<form action='/savesettings' method='post'>";

  // Appearance
  h+="<div class='card'><h2>Appearance</h2>";
  h+="<div class='row'><span class='lbl'>Dark Mode</span>";
  h+="<button type='button' class='tgl "+String(darkMode?"on":"off")+"' id='dkB' onclick='th(\"dk\",\"dkB\")'><div class='knob'></div></button>";
  h+="<input type='hidden' name='dark' id='dk' value='"+String(darkMode?"1":"0")+"'></div></div>";

  // LED
  h+="<div class='card'><h2>LED Indicator</h2>";
  h+="<div class='row'><span class='lbl'>LED On/Off</span>";
  h+="<button type='button' class='tgl "+String(ledState?"on":"off")+"' id='ldB' onclick='th(\"ld\",\"ldB\")'><div class='knob'></div></button>";
  h+="<input type='hidden' name='led' id='ld' value='"+String(ledState?"1":"0")+"'></div></div>";

  // Weather & TZ
  h+="<div class='card'><details><summary>&#127777; Weather &amp; Timezone</summary><div style='margin-top:10px'>";
  h+="<label class='lbl'>City Name</label>";
  h+="<input type='text' name='city' value='"+weatherCity+"' placeholder='Dhaka, London, Tokyo...'>";
  h+="<label class='lbl' style='display:block;margin-top:10px'>Timezone (UTC offset)</label>";
  h+="<select name='tz'>";
  for(int tz=-12;tz<=14;tz++){
    String sel=(tz==timezone_offset)?" selected":"";
    h+="<option value='"+String(tz)+"'"+sel+">UTC"+(tz>=0?"+":"")+String(tz)+"</option>";
  }
  h+="</select></div></details></div>";

  // Screen functions
  h+="<div class='card'><h2>OLED Screen Functions</h2>";
  const char* sn[]={"Weather","Robot Eyes","Digital Clock","Analog Clock","Relay Overview","Web UI","People Count"};
  h+="<div class='cbg'>";
  for(int i=0;i<SCREEN_COUNT;i++)
    h+="<label class='cbi'><input type='checkbox' name='scr"+String(i)+"'"+(screenEnabled[i]?" checked":"")+"> "+String(sn[i])+"</label>";
  h+="</div>";
  h+="<div class='row' style='margin-top:12px'><span class='lbl'>Auto-switch</span>";
  h+="<button type='button' class='tgl "+String(autoSwitch?"on":"off")+"' id='asB' onclick='th(\"as\",\"asB\")'><div class='knob'></div></button>";
  h+="<input type='hidden' name='autosw' id='as' value='"+String(autoSwitch?"1":"0")+"'></div>";
  h+="<label class='lbl' style='display:block;margin-top:10px'>Interval (seconds)</label>";
  h+="<input type='number' name='scrint' value='"+String(screenInterval)+"' min='2' max='120'></div>";

  // Auto-off + Sonar diagnostics
  h+="<div class='card'><h2>Auto-Off on No Presence</h2>";
  h+="<p style='font-size:.8rem;color:"+sub+";margin-bottom:8px'>Turn off when room is empty:</p>";
  h+="<div class='cbg'>";
  for(int i=0;i<8;i++)
    h+="<label class='cbi'><input type='checkbox' name='ao"+String(i)+"'"+(autoOffEnabled[i]?" checked":"")+"> Switch "+String(i+1)+"</label>";
  h+="</div>";
  // Delay timers
  h+="<div style='margin-top:12px;padding-top:12px;border-top:1px solid "+bdr+"'>";
  h+="<label class='lbl' style='display:block;margin-bottom:4px'>Turn Off Delay (seconds, 0=instant)</label>";
  h+="<input type='number' name='aodelay' value='"+String(autoOffDelaySec)+"' min='0' max='300' style='margin-bottom:10px'>";
  h+="<label class='lbl' style='display:block;margin-bottom:4px'>Turn On Delay (seconds, 0=instant)</label>";
  h+="<input type='number' name='aondelay' value='"+String(autoOnDelaySec)+"' min='0' max='300'>";
  h+="</div>";
  // Sonar tuning inputs
  h+="<div style='margin-top:12px;padding-top:12px;border-top:1px solid "+bdr+"'>";
  h+="<label class='lbl' style='display:block;margin-bottom:4px'>Detection Distance (cm)</label>";
  h+="<input type='number' name='snrdist' value='"+String(sonarDetectDist)+"' min='10' max='300' style='margin-bottom:10px'>";
  h+="<label class='lbl' style='display:block;margin-bottom:4px'>Confirm Window (ms)</label>";
  h+="<input type='number' name='snrwin' value='"+String(sonarConfirmWin)+"' min='500' max='5000'>";
  h+="</div>";
  // Sonar live indicator dots
  h+="<div style='display:flex;gap:20px;align-items:center;margin-top:14px;padding-top:12px;border-top:1px solid "+bdr+"'>";
  h+="<div style='display:flex;align-items:center;gap:8px'>";
  h+="<div id='dot1' style='width:14px;height:14px;border-radius:50%;background:#444466;transition:.2s'></div>";
  h+="<span style='font-size:.85rem'>Sonar Out (S1)</span></div>";
  h+="<div style='display:flex;align-items:center;gap:8px'>";
  h+="<div id='dot2' style='width:14px;height:14px;border-radius:50%;background:#444466;transition:.2s'></div>";
  h+="<span style='font-size:.85rem'>Sonar In (S2)</span></div>";
  h+="</div></div>";

  h+="<button type='submit' class='btn bs'>&#10003; Save Settings</button></form>";

  // WiFi
  h+="<div class='card' style='margin-top:14px'><h2>WiFi</h2>";
  h+="<form action='/savewifi' method='post'>";
  h+="<input type='text' name='ssid' placeholder='WiFi SSID' value='"+storedSSID+"' style='margin-bottom:8px'>";
  h+="<input type='password' name='password' placeholder='Password'>";
  h+="<button type='submit' class='btn bw' style='margin-top:8px'>&#128246; Connect</button></form>";
  h+="<form action='/useap' method='post'><button type='submit' class='btn' style='background:#8e44ad;color:#fff;margin-top:8px'>AP Mode</button></form></div>";

  // OTA
  h+="<div class='card'><h2>Firmware Update</h2>";
  h+="<p style='font-size:.82rem;color:"+sub+";margin-bottom:8px'>Current: v"+String(FW_VERSION)+" &bull; "+String(OTA_HOST)+":"+String(OTA_PORT)+"</p>";
  h+="<button type='button' class='btn bu' onclick='doUpd()'>&#8593; Check &amp; Update</button>";
  h+="<div id='otamsg'></div></div>";

  // Factory reset
  h+="<button class='btn bd' style='margin-top:4px' onclick='if(confirm(\"Factory reset? All settings lost!\"))fetch(\"/factoryreset\",{method:\"POST\"})'>&#9888; Factory Reset</button>";

  h+="<script>";
  h+="function th(id,btn){var v=document.getElementById(id),b=document.getElementById(btn);";
  h+="if(v.value=='1'){v.value='0';b.className='tgl off';}else{v.value='1';b.className='tgl on';}";
  h+="b.innerHTML='<div class=\"knob\"></div>';}";
  h+="function doUpd(){var m=document.getElementById('otamsg');m.innerText='Requesting...';";
  h+="fetch('/doupdate',{method:'POST'}).then(()=>{";
  h+="var p=setInterval(()=>{fetch('/otastatus').then(r=>r.text()).then(t=>{m.innerText=t;";
  h+="if(t.includes('date')||t.includes('fail')||t.includes('reach')||t.includes('found'))clearInterval(p);});},1500);});}";
  // Sonar live dot polling
  h+="setInterval(()=>{fetch('/sonarstatus').then(r=>r.json()).then(d=>{";
  h+="document.getElementById('dot1').style.background=d.s1?'#4ade80':'#444466';";
  h+="document.getElementById('dot2').style.background=d.s2?'#4ade80':'#444466';";
  h+="}).catch(()=>{});},300);";
  h+="</script></body></html>";
  return h;
}

// ============================================================
// END v2.6
// ============================================================
