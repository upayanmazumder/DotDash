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
const char* AP_PASS = "";
DNSServer dnsServer;
WebServer webServer(80);

// Timing
const int DOT_TIME = 200;
const int DASH_TIME = 600;
const int DEBOUNCE = 30;
const unsigned long CHAR_GAP = 1000;
const unsigned long END_GAP = 4000;
const float THRESHOLD_FACTOR = 0.7;

// State variables
bool Q_press = false;
unsigned long pressStart = 0;
unsigned long lastRelease = 0;
int baseLevel = 0;
int touchThreshold = 0;

// Buffers
String morseLine = "";
String decodedMessage = "";
String currentToken = "";
bool translationShown = false;

// Morse code lookup table
const char* MORSE_TABLE[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....",
  "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.",
  "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-",
  "-.--", "--.."
};

// Digital Logic Gate Simulation
bool AND(bool a, bool b) { return a && b; }
bool OR(bool a, bool b) { return a || b; }
bool NOT(bool a) { return !a; }
bool XOR(bool a, bool b) { return (a != b); }

// Encode using logic gates (complete A-Z) - ALL using 5 bits
String encodeToMorse(char input) {
  char c = toupper(input);
  int val = c - 'A';  // Convert A-Z to 0-25
  
  if (val < 0 || val > 25) return "?";
  
  // Extract 5 bits using logic gates (b4 b3 b2 b1 b0)
  bool b0 = (val & 0x01);
  bool b1 = (val & 0x02) >> 1;
  bool b2 = (val & 0x04) >> 2;
  bool b3 = (val & 0x08) >> 3;
  bool b4 = (val & 0x10) >> 4;

  // A = 0 (00000)
  if (AND(AND(NOT(b4), NOT(b3)), AND(NOT(b2), NOT(b1))) && NOT(b0)) return ".-";
  // B = 1 (00001)
  if (AND(AND(NOT(b4), NOT(b3)), AND(NOT(b2), NOT(b1))) && b0) return "-...";
  // C = 2 (00010)
  if (AND(AND(NOT(b4), NOT(b3)), AND(NOT(b2), b1)) && NOT(b0)) return "-.-.";
  // D = 3 (00011)
  if (AND(AND(NOT(b4), NOT(b3)), AND(NOT(b2), b1)) && b0) return "-..";
  // E = 4 (00100)
  if (AND(AND(NOT(b4), NOT(b3)), AND(b2, NOT(b1))) && NOT(b0)) return ".";
  // F = 5 (00101)
  if (AND(AND(NOT(b4), NOT(b3)), AND(b2, NOT(b1))) && b0) return "..-.";
  // G = 6 (00110)
  if (AND(AND(NOT(b4), NOT(b3)), AND(b2, b1)) && NOT(b0)) return "--.";
  // H = 7 (00111)
  if (AND(AND(NOT(b4), NOT(b3)), AND(b2, b1)) && b0) return "....";
  // I = 8 (01000)
  if (AND(AND(NOT(b4), b3), AND(NOT(b2), NOT(b1))) && NOT(b0)) return "..";
  // J = 9 (01001)
  if (AND(AND(NOT(b4), b3), AND(NOT(b2), NOT(b1))) && b0) return ".---";
  // K = 10 (01010)
  if (AND(AND(NOT(b4), b3), AND(NOT(b2), b1)) && NOT(b0)) return "-.-";
  // L = 11 (01011)
  if (AND(AND(NOT(b4), b3), AND(NOT(b2), b1)) && b0) return ".-..";
  // M = 12 (01100)
  if (AND(AND(NOT(b4), b3), AND(b2, NOT(b1))) && NOT(b0)) return "--";
  // N = 13 (01101)
  if (AND(AND(NOT(b4), b3), AND(b2, NOT(b1))) && b0) return "-.";
  // O = 14 (01110)
  if (AND(AND(NOT(b4), b3), AND(b2, b1)) && NOT(b0)) return "---";
  // P = 15 (01111)
  if (AND(AND(NOT(b4), b3), AND(b2, b1)) && b0) return ".--.";
  // Q = 16 (10000)
  if (AND(AND(b4, NOT(b3)), AND(NOT(b2), NOT(b1))) && NOT(b0)) return "--.-";
  // R = 17 (10001)
  if (AND(AND(b4, NOT(b3)), AND(NOT(b2), NOT(b1))) && b0) return ".-.";
  // S = 18 (10010)
  if (AND(AND(b4, NOT(b3)), AND(NOT(b2), b1)) && NOT(b0)) return "...";
  // T = 19 (10011)
  if (AND(AND(b4, NOT(b3)), AND(NOT(b2), b1)) && b0) return "-";
  // U = 20 (10100)
  if (AND(AND(b4, NOT(b3)), AND(b2, NOT(b1))) && NOT(b0)) return "..-";
  // V = 21 (10101)
  if (AND(AND(b4, NOT(b3)), AND(b2, NOT(b1))) && b0) return "...-";
  // W = 22 (10110)
  if (AND(AND(b4, NOT(b3)), AND(b2, b1)) && NOT(b0)) return ".--";
  // X = 23 (10111)
  if (AND(AND(b4, NOT(b3)), AND(b2, b1)) && b0) return "-..-";
  // Y = 24 (11000)
  if (AND(AND(b4, b3), AND(NOT(b2), NOT(b1))) && NOT(b0)) return "-.--";
  // Z = 25 (11001)
  if (AND(AND(b4, b3), AND(NOT(b2), NOT(b1))) && b0) return "--..";

  return "?";
}

// Web interface
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>"
                "<style>body{font-family:Arial;padding:20px;background:#1a1a1a;color:#0f0}"
                "h1{color:#0f0;text-shadow:0 0 10px #0f0}"
                ".box{background:#000;border:2px solid #0f0;padding:15px;margin:10px 0;border-radius:5px}"
                ".label{color:#0a0;font-size:14px}.value{font-size:20px;font-weight:bold;font-family:monospace}</style>"
                "<title>DotDash DSD</title></head><body>"
                "<h1>⚡ DotDash DSD ⚡</h1>"
                "<div class='box'><div class='label'>MORSE CODE:</div><div class='value' id='morse'>---</div></div>"
                "<div class='box'><div class='label'>DECODED:</div><div class='value' id='letters'>...</div></div>"
                "<script>"
                "setInterval(async()=>{"
                "const res=await fetch('/live');"
                "const txt=await res.text();"
                "const parts=txt.split('|');"
                "document.getElementById('morse').innerText=parts[0]||'---';"
                "document.getElementById('letters').innerText=parts[1]||'...';"
                "},200);"
                "</script></body></html>";
  webServer.send(200, "text/html", html);
}

void handleLive() {
  String out = morseLine + "|" + decodedMessage;
  webServer.send(200, "text/plain", out);
}

void addToScroll(String &line, String s, int maxChars) {
  line += s;
  if (line.length() > maxChars) line = line.substring(line.length() - maxChars);
}

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
  webServer.begin();

  long total = 0;
  for (int i = 0; i < 50; i++) {
    total += touchRead(TOUCH_PIN);
    delay(20);
  }
  baseLevel = total / 50;
  touchThreshold = baseLevel * THRESHOLD_FACTOR;
  lastRelease = millis();

  Serial.println("DSD Morse Encoder Started (A-Z Complete)");
  Serial.print("Access at: ");
  Serial.println(apIP);
}

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  int reading = touchRead(TOUCH_PIN);
  unsigned long now = millis();

  if (reading < touchThreshold && !Q_press && now - lastRelease > DEBOUNCE) {
    Q_press = true;
    pressStart = now;
  }

  if (Q_press && reading >= touchThreshold) {
    Q_press = false;
    unsigned long pressDuration = now - pressStart;
    lastRelease = now;
    String symbol = (pressDuration < DOT_TIME) ? "." : "-";
    addToScroll(morseLine, symbol, 24);
    currentToken += symbol;
  }

  // Decode Morse to letter (A–Z)
  if (!Q_press && currentToken != "" && now - lastRelease > CHAR_GAP) {
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

    addToScroll(decodedMessage, String(decoded), 20);
    currentToken = "";
    morseLine = "";
  }

  if (!Q_press && now - lastRelease > END_GAP && !translationShown) {
    translationShown = true;
    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.print("Message: ");
    u8g2.print(decodedMessage);
    u8g2.sendBuffer();
  }

  if (!translationShown) {
    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.print("Morse: ");
    u8g2.print(morseLine);
    u8g2.setCursor(0, 26);
    u8g2.print("Letters: ");
    u8g2.print(decodedMessage);

    const int barWidth = 120;
    const int barHeight = 6;
    const int barX = 4;
    const int barY = 58;
    u8g2.drawFrame(barX, barY, barWidth, barHeight);

    if (Q_press) {
      float progress = (float)(now - pressStart) / (float)DASH_TIME;
      if (progress > 1.0) progress = 1.0;
      int fill = progress * barWidth;
      u8g2.drawBox(barX, barY, fill, barHeight);
    }
    u8g2.sendBuffer();
  }
}