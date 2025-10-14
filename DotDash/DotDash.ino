#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>

// -------------------- PINS --------------------
#define TOUCH_PIN T0  // GPIO 4 = T0 touch pin on ESP32
#define BUZZER_PIN 15
#define SDA_PIN 21
#define SCL_PIN 22

// -------------------- OLED --------------------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

// -------------------- WiFi AP --------------------
const char* AP_SSID = "DotDash";
const char* AP_PASS = "";
DNSServer dnsServer;
WebServer webServer(80);

// -------------------- Morse --------------------
String currentToken = "";
String liveMorse = "";

// -------------------- State --------------------
bool isPressed = false;
unsigned long pressStart = 0;
unsigned long lastRead = 0;
int baseLevel = 0;
int touchThreshold = 0;

// -------------------- Config --------------------
const int DOT_TIME = 200;  // milliseconds
const int DEBOUNCE = 30;   // ms
const float THRESHOLD_FACTOR = 0.7;  // touch triggers when below 70% of baseline

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  Serial.println("\n--- DotDash (ESP32 Touch Native) ---");

  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);

  pinMode(BUZZER_PIN, OUTPUT);

  // WiFi Access Point
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(53, "*", apIP);

  webServer.on("/", handleRoot);
  webServer.on("/live", handleLive);
  webServer.onNotFound(handleRoot);
  webServer.begin();

  tone(BUZZER_PIN, 1000, 100); delay(150);
  tone(BUZZER_PIN, 1500, 150); delay(200);

  // Calibrate baseline
  Serial.println("Calibrating touch baseline...");
  long total = 0;
  for (int i = 0; i < 50; i++) {
    total += touchRead(TOUCH_PIN);
    delay(20);
  }
  baseLevel = total / 50;
  touchThreshold = baseLevel * THRESHOLD_FACTOR;
  Serial.print("Base level: "); Serial.println(baseLevel);
  Serial.print("Touch threshold: "); Serial.println(touchThreshold);

  u8g2.clearBuffer();
  u8g2.setCursor(0, 12);
  u8g2.print("Touch baseline: ");
  u8g2.print(baseLevel);
  u8g2.sendBuffer();

  Serial.println("DotDash ready!");
}

// -------------------- Web Handlers --------------------
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>"
                "<title>DotDash Portal</title></head><body>"
                "<h1>DotDash Portal</h1>"
                "<p>Live Morse input:</p>"
                "<div id='morse'></div>"
                "<script>"
                "var evtSource = new EventSource('/live');"
                "evtSource.onmessage = function(e){document.getElementById('morse').innerText = e.data;};"
                "</script></body></html>";
  webServer.send(200, "text/html", html);
}

void handleLive() {
  webServer.sendHeader("Content-Type", "text/event-stream");
  webServer.sendHeader("Cache-Control", "no-cache");
  webServer.sendHeader("Connection", "keep-alive");
  webServer.send(200, "text/event-stream", "data: " + liveMorse + "\n\n");
}

// -------------------- Touch detection --------------------
void checkTouch() {
  int rawValue = touchRead(TOUCH_PIN);
  bool touched = (rawValue < touchThreshold);

  static bool wasTouched = false;
  static unsigned long touchStart = 0;

  if (touched && !wasTouched) {
    // Touch started
    wasTouched = true;
    touchStart = millis();
    tone(BUZZER_PIN, 1200);
    Serial.print("Touch start (value=");
    Serial.print(rawValue);
    Serial.println(")");
  }

  if (!touched && wasTouched) {
    // Touch released
    wasTouched = false;
    noTone(BUZZER_PIN);
    unsigned long duration = millis() - touchStart;
    Serial.print("Touch released (value=");
    Serial.print(rawValue);
    Serial.print(") duration=");
    Serial.print(duration);
    Serial.println("ms");

    String symbol = (duration < DOT_TIME) ? "." : "-";
    currentToken += symbol;
    liveMorse = currentToken;

    Serial.print("Added symbol: ");
    Serial.println(symbol);

    // OLED visual
    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.print("Detected: ");
    u8g2.print(symbol);
    u8g2.sendBuffer();
  }

  // While holding
  if (touched) {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.print("HOLDING...");
    u8g2.sendBuffer();
  }

  delay(20);
}

// -------------------- OLED Update --------------------
void updateOLED() {
  u8g2.clearBuffer();
  u8g2.setCursor(0, 0);
  u8g2.print("Morse:");
  u8g2.setCursor(0, 14);
  u8g2.print(currentToken);
  u8g2.sendBuffer();
}

// -------------------- Loop --------------------
void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  checkTouch();
  updateOLED();
}
