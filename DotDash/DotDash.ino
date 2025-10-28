#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>

// Pins
#define TOUCH_PIN 4
#define SDA_PIN 21
#define SCL_PIN 22

// OLED setup
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

// WiFi AP Setup
const char* AP_SSID = "DotDash-DSD";
const char* AP_PASS = ""; // Empty password for open network
DNSServer dnsServer;
WebServer webServer(80);

// Timing for Morse code
const int DOT_TIME = 200;
const int DASH_TIME = 600;
const int DEBOUNCE = 30;
const unsigned long CHAR_GAP = 1000;
const unsigned long END_GAP = 4000;
const float THRESHOLD_FACTOR = 0.7;

// State variables for detecting touch and Morse
bool flipFlop_Q = false;
unsigned long pressStart = 0;
unsigned long lastRelease = 0;
int baseLevel = 0;
int touchThreshold = 0;

// Buffers for Morse and letters
String morseLine = "";
String letterLine = "";
String currentToken = "";
bool translationShown = false;

// Morse code encoding for A-Z and 0-9
String encodeToMorse(char input) {
  switch (input) {
    case 'A': return ".-";
    case 'B': return "-...";
    case 'C': return "-.-.";
    case 'D': return "-..";
    case 'E': return ".";
    case 'F': return "..-.";
    case 'G': return "--.";
    case 'H': return "....";
    case 'I': return "..";
    case 'J': return ".---";
    case 'K': return "-.-";
    case 'L': return ".-..";
    case 'M': return "--";
    case 'N': return "-.";
    case 'O': return "---";
    case 'P': return ".--.";
    case 'Q': return "--.-";
    case 'R': return ".-.";
    case 'S': return "...";
    case 'T': return "-";
    case 'U': return "..-";
    case 'V': return "...-";
    case 'W': return ".--";
    case 'X': return "-..-";
    case 'Y': return "-.--";
    case 'Z': return "--..";
    case '0': return "-----";
    case '1': return ".----";
    case '2': return "..---";
    case '3': return "...--";
    case '4': return "....-";
    case '5': return ".....";
    case '6': return "-....";
    case '7': return "--...";
    case '8': return "---..";
    case '9': return "----.";
    default: return "?"; // Error case for unsupported characters
  }
}

// Web interface handlers
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>"
                "<title>DotDash DSD</title></head><body>"
                "<h1>DotDash (DSD Simulation)</h1>"
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
                "</script></body></html>";
  webServer.send(200, "text/html", html);
}

void handleLive() {
  String out = morseLine + "|" + letterLine;
  webServer.send(200, "text/plain", out);
}

// Helper function for scrolling the display
void addToScroll(String &line, String s, int maxChars) {
  line += s;
  if (line.length() > maxChars) line = line.substring(line.length() - maxChars);
}

// Setup function
void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);

  // Set up WiFi access point
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  dnsServer.start(53, "*", apIP);
  webServer.on("/", handleRoot);
  webServer.on("/live", handleLive);
  webServer.begin();

  // Touch sensor baseline calibration
  long total = 0;
  for (int i = 0; i < 50; i++) {
    total += touchRead(TOUCH_PIN);
    delay(20);
  }
  baseLevel = total / 50;
  touchThreshold = baseLevel * THRESHOLD_FACTOR;

  lastRelease = millis();
  Serial.println("DSD Morse Encoder Started");
}

// Main loop function
void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  int reading = touchRead(TOUCH_PIN);
  unsigned long now = millis();

  // Detect touch event
  if (reading < touchThreshold && !flipFlop_Q && now - lastRelease > DEBOUNCE) {
    flipFlop_Q = true;
    pressStart = now;
  }

  if (flipFlop_Q && reading >= touchThreshold) {
    flipFlop_Q = false;
    unsigned long pressDuration = now - pressStart;
    lastRelease = now;

    String symbol = (pressDuration < DOT_TIME) ? "." : "-";
    addToScroll(morseLine, symbol, 24);
    currentToken += symbol;
  }

  // Morse code to letter decoding
  if (!flipFlop_Q && currentToken != "" && now - lastRelease > CHAR_GAP) {
    char decoded = '?';
    if (currentToken == ".-") decoded = 'A';
    else if (currentToken == "-...") decoded = 'B';
    else if (currentToken == "-.-.") decoded = 'C';
    else if (currentToken == "-..") decoded = 'D';
    else if (currentToken == ".") decoded = 'E';
    else if (currentToken == "..-.") decoded = 'F';
    else if (currentToken == "--.") decoded = 'G';
    else if (currentToken == "....") decoded = 'H';
    else if (currentToken == "..") decoded = 'I';
    else if (currentToken == ".---") decoded = 'J';
    else if (currentToken == "-.-") decoded = 'K';
    else if (currentToken == ".-..") decoded = 'L';
    else if (currentToken == "--") decoded = 'M';
    else if (currentToken == "-.") decoded = 'N';
    else if (currentToken == "---") decoded = 'O';
    else if (currentToken == ".--.") decoded = 'P';
    else if (currentToken == "--.-") decoded = 'Q';
    else if (currentToken == ".-.") decoded = 'R';
    else if (currentToken == "...") decoded = 'S';
    else if (currentToken == "-") decoded = 'T';
    else if (currentToken == "..-") decoded = 'U';
    else if (currentToken == "...-") decoded = 'V';
    else if (currentToken == ".--") decoded = 'W';
    else if (currentToken == "-..-") decoded = 'X';
    else if (currentToken == "-.--") decoded = 'Y';
    else if (currentToken == "--..") decoded = 'Z';
    else if (currentToken == "-----") decoded = '0';
    else if (currentToken == ".----") decoded = '1';
    else if (currentToken == "..---") decoded = '2';
    else if (currentToken == "...--") decoded = '3';
    else if (currentToken == "....-") decoded = '4';
    else if (currentToken == ".....") decoded = '5';
    else if (currentToken == "-....") decoded = '6';
    else if (currentToken == "--...") decoded = '7';
    else if (currentToken == "---..") decoded = '8';
    else if (currentToken == "----.") decoded = '9';

    addToScroll(letterLine, String(decoded), 20);
    currentToken = "";
    morseLine = "";
  }

  // Display the decoded message after delay
  if (!flipFlop_Q && now - lastRelease > END_GAP && !translationShown) {
    translationShown = true;
    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.print("Message: ");
    u8g2.print(letterLine);
    u8g2.sendBuffer();
  }

  // OLED live update for Morse code
  if (!translationShown) {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.print("Morse: ");
    u8g2.print(morseLine);
    u8g2.setCursor(0, 26);
    u8g2.print("Letters: ");
    u8g2.print(letterLine);

    // Signal bar simulation for press duration
    const int barWidth = 120;
    const int barHeight = 6;
    const int barX = 4;
    const int barY = 58;
    u8g2.drawFrame(barX, barY, barWidth, barHeight);

    if (flipFlop_Q) {
      float progress = (float)(now - pressStart) / (float)DASH_TIME;
      if (progress > 1.0) progress = 1.0;
      int fill = progress * barWidth;
      u8g2.drawBox(barX, barY, fill, barHeight);
    }
    u8g2.sendBuffer();
  }
}
