#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <map>

// -------------------- PINS --------------------
#define TOUCH_PIN 4    // GPIO4 = touch pin on ESP32
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
String liveMorse = "";          // full live Morse string (for web)
String morseLine = "";          // Line 1: only dots/dashes
String letterLine = "";         // Line 2: decoded letters
bool translationShown = false;

// -------------------- State --------------------
bool isPressed = false;
unsigned long pressStart = 0;
unsigned long lastRead = 0;
unsigned long lastRelease = 0;
int baseLevel = 0;
int touchThreshold = 0;

// -------------------- Config --------------------
const int DOT_TIME = 200;           // ms
const int DEBOUNCE = 30;            // ms
const float THRESHOLD_FACTOR = 0.7; // 70% of baseline triggers touch

// -------------------- Gap Detection --------------------
const unsigned long CHAR_GAP = 3000; // 3s = gap between letters
const unsigned long END_GAP  = 10000; // 10s = end of input

// -------------------- Morse Table --------------------
std::map<String, char> morseTable = {
  {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},
  {".", 'E'}, {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'},
  {"..", 'I'}, {".---", 'J'}, {"-.-", 'K'}, {".-..", 'L'},
  {"--", 'M'}, {"-.", 'N'}, {"---", 'O'}, {".--.", 'P'},
  {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
  {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'},
  {"-.--", 'Y'}, {"--..", 'Z'},
  {"-----",'0'}, {".----",'1'}, {"..---",'2'}, {"...--",'3'},
  {"....-",'4'}, {".....",'5'}, {"-....",'6'}, {"--...",'7'},
  {"---..",'8'}, {"----.",'9'}
};

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  Serial.println("\n--- DotDash (ESP32 Touch Native) ---");

  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);

  // WiFi Access Point
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(53, "*", apIP);

  webServer.on("/", handleRoot);
  webServer.on("/live", handleLive);
  webServer.onNotFound(handleRoot);
  webServer.begin();

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
                "<p>Live Morse: <span id='morse'></span></p>"
                "<script>"
                "setInterval(async()=>{"
                " const res = await fetch('/live');"
                " document.getElementById('morse').innerText = await res.text();"
                "},200);"
                "</script>"
                "</body></html>";
  webServer.send(200, "text/html", html);
}

void handleLive() {
  webServer.send(200, "text/plain", liveMorse);
}

// -------------------- Scrolling helper --------------------
void addToScroll(String &line, String s, int maxChars, int labelLength) {
  line += s;
  int effectiveMax = maxChars - labelLength;
  if (line.length() > effectiveMax) {
    line = line.substring(line.length() - effectiveMax);
  }
}

// -------------------- Loop --------------------
void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  int reading = touchRead(TOUCH_PIN);
  unsigned long now = millis();

  // --- Touch detection ---
  if (reading < touchThreshold && !isPressed && now - lastRead > DEBOUNCE) {
    isPressed = true;
    pressStart = now;
    lastRead = now;
  }

  // --- Release detection ---
  if (isPressed && reading >= touchThreshold) {
    isPressed = false;
    unsigned long pressDuration = now - pressStart;
    lastRelease = now;

    // Dot or dash
    String symbol = (pressDuration < DOT_TIME) ? "." : "-";
    currentToken += symbol;
    liveMorse += symbol;
    addToScroll(morseLine, symbol, 28, 7); // "Morse: " label length = 7
  }

  // --- Character gap detection ---
  if (!isPressed && currentToken != "" && now - lastRelease > CHAR_GAP) {
    char decoded = '?';
    if (morseTable.count(currentToken)) decoded = morseTable[currentToken];
    liveMorse += "("; liveMorse += decoded; liveMorse += ")";
    addToScroll(letterLine, String(decoded), 28, 9); // "Letters: " label length = 9
    currentToken = "";
  }

  // --- End of input (10s) ---
  if (!isPressed && now - lastRelease > END_GAP && !translationShown) {
    translationShown = true;
    u8g2.clearBuffer();
    u8g2.setCursor(0,12);
    u8g2.print("Message: ");
    u8g2.print(letterLine);  // <-- only decoded letters
    u8g2.sendBuffer();
  }

  // --- OLED live update ---
  if (!translationShown) {
    u8g2.clearBuffer();
    // Line 1: dots/dashes
    u8g2.setCursor(0,12);
    u8g2.print("Morse: ");
    u8g2.print(morseLine);
    // Line 2: decoded letters
    u8g2.setCursor(0,26);
    u8g2.print("Letters: ");
    u8g2.print(letterLine);

    // --- Progress bar ---
    const int barWidth = 120;
    const int barHeight = 6;
    const int barX = 4;
    const int barY = 58;
    u8g2.drawFrame(barX, barY, barWidth, barHeight);

    if (isPressed) {
      float progress = (float)(now - pressStart) / DOT_TIME;
      if (progress > 1.0) progress = 1.0;
      int fill = progress * barWidth;
      u8g2.drawBox(barX, barY, fill, barHeight);
    }

    u8g2.sendBuffer();
  }
}
