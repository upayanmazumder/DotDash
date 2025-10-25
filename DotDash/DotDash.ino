#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <map>

//  PINS 
#define TOUCH_PIN 4
#define SDA_PIN 21
#define SCL_PIN 22

//  OLED 
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

//  WiFi AP 
const char* AP_SSID = "DotDash";
const char* AP_PASS = "";
DNSServer dnsServer;
WebServer webServer(80);

//  Morse 
String currentToken = "";
String liveMorse = "";      // full Morse for web
String morseLine = "";      // OLED line 1 dots/dashes
String letterLine = "";     // OLED line 2 decoded letters
bool translationShown = false;

//  State 
bool isPressed = false;
unsigned long pressStart = 0;
unsigned long lastRead = 0;
unsigned long lastRelease = 0;
int baseLevel = 0;
int touchThreshold = 0;

//  Config 
const int DOT_TIME = 200;           // ms
const int DEBOUNCE = 30;            // ms
const float THRESHOLD_FACTOR = 0.7; // 70% of baseline triggers touch
const unsigned long CHAR_GAP = 1000;  // 1s gap between letters
const unsigned long END_GAP  = 5000;  // 5s end of input
const unsigned long DASH_TIME = 3 * DOT_TIME;

//  Morse Table 
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

//  Setup 
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);

  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(53, "*", apIP);

  webServer.on("/", handleRoot);
  webServer.on("/live", handleLive);
  webServer.onNotFound(handleRoot);
  webServer.begin();

  // Calibrate touch baseline
  long total = 0;
  for (int i = 0; i < 50; i++) {
    total += touchRead(TOUCH_PIN);
    delay(20);
  }
  baseLevel = total / 50;
  touchThreshold = baseLevel * THRESHOLD_FACTOR;
  lastRelease = millis();

  Serial.println("DotDash ESP32 Morse Started");
  Serial.print("Touch baseline: "); Serial.println(baseLevel);
  Serial.print("Touch threshold: "); Serial.println(touchThreshold);
}

//  Web Handlers 
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>"
                "<title>DotDash Portal</title></head><body>"
                "<h1>DotDash Portal</h1>"
                "<div><strong>Morse:</strong> <span id='morse'></span></div>"
                "<div><strong>Letters:</strong> <span id='letters'></span></div>"
                "<script>"
                "setInterval(async()=>{"
                " const res = await fetch('/live');"
                " const txt = await res.text();"
                " const parts = txt.split('|');"
                " document.getElementById('morse').innerText = parts[0];"
                " document.getElementById('letters').innerText = parts[1];"
                "},200);"
                "</script>"
                "</body></html>";
  webServer.send(200, "text/html", html);
}

void handleLive() {
  String out = morseLine + "|" + letterLine;
  webServer.send(200, "text/plain", out);
}

//  Scrolling helper 
void addToScroll(String &line, String s, int maxChars) {
  line += s;
  if (line.length() > maxChars) {
    line = line.substring(line.length() - maxChars);
  }
}

//  Loop 
void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  int reading = touchRead(TOUCH_PIN);
  unsigned long now = millis();

  //  Touch detection 
  if (reading < touchThreshold && !isPressed && now - lastRead > DEBOUNCE) {
    isPressed = true;
    pressStart = now;
    lastRead = now;
    Serial.println("Pressed!");
  }

  //  Release detection 
  if (isPressed && reading >= touchThreshold) {
    isPressed = false;
    unsigned long pressDuration = now - pressStart;
    lastRelease = now;

    String symbol = (pressDuration < DOT_TIME) ? "." : "-";
    currentToken += symbol;
    addToScroll(morseLine, symbol, 28 - 7);

    Serial.print("Released! Duration: "); Serial.print(pressDuration);
    Serial.print(" ms | Symbol: "); Serial.println(symbol);
    Serial.print("Current token: "); Serial.println(currentToken);
  }

  //  Character gap detection 
  if (!isPressed && currentToken != "" && now - lastRelease > CHAR_GAP) {
    char decoded = '?';
    if (morseTable.count(currentToken)) decoded = morseTable[currentToken];
    addToScroll(letterLine, String(decoded), 28 - 9);
    Serial.print("Decoded char: "); Serial.println(decoded);
    morseLine = "";
    currentToken = "";
  }

  //  End of input (5s) 
  if (!isPressed && now - lastRelease > END_GAP && !translationShown) {
    translationShown = true;
    Serial.println("Message complete: " + letterLine);

    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.print("Message: ");
    u8g2.print(letterLine);
    u8g2.sendBuffer();
  }

  //  OLED live update 
  if (!translationShown) {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.print("Morse: ");
    u8g2.print(morseLine);
    u8g2.setCursor(0, 26);
    u8g2.print("Letters: ");
    u8g2.print(letterLine);

    //  Progress bar 
    const int barWidth = 120;
    const int barHeight = 6;
    const int barX = 4;
    const int barY = 58;
    u8g2.drawFrame(barX, barY, barWidth, barHeight);

    if (isPressed) {
      float progress = (float)(now - pressStart) / (float)DASH_TIME;
      if (progress > 1.0) progress = 1.0;
      int fill = progress * barWidth;
      u8g2.drawBox(barX, barY, fill, barHeight);

      // glow when crossing DOT threshold
      if ((now - pressStart) > DOT_TIME) {
        u8g2.drawHLine(barX, barY - 2, barWidth);
      }
    } else if (now - lastRelease < 150) {
      int fade = map(now - lastRelease, 0, 150, barWidth, 0);
      u8g2.drawBox(barX, barY, fade, barHeight);
    }

    u8g2.sendBuffer();

    // Serial live update (like web)
    Serial.print("Live Morse: "); Serial.print(morseLine);
    Serial.print(" | Letters: "); Serial.println(letterLine);
  }
}
