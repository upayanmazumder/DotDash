#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <map>

// -------------------- PINS --------------------
#define TOUCH_PIN T0 // GPIO4
#define BUZZER_PIN 15
#define SDA_PIN 21
#define SCL_PIN 22

// -------------------- DISPLAY --------------------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

// -------------------- WIFI --------------------
const char* AP_SSID = "DotDashCyber";
const char* AP_PASS = "";
DNSServer dnsServer;
WebServer webServer(80);

// -------------------- MORSE MAP --------------------
std::map<String, char> MORSE = {
  {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'},
  {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'},
  {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'},
  {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
  {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'},
  {"--..", 'Z'}, {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
  {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'},
  {"---..", '8'}, {"----.", '9'}
};

// -------------------- CONFIG --------------------
String morseSeq = "";
String translated = "";
unsigned long touchStart = 0, lastRelease = 0, lastTouch = 0, lastCalib = 0, lastInputTime = 0;
bool isPressed = false, isHeld = false;
int baseLevel = 0, touchThreshold = 0, progress = 0, shimmerOffset = 0;
int morseScroll = 0, textScroll = 0;

const int DOT_TIME = 250;
const int HOLD_TIME = 700;
const float THRESHOLD_FACTOR = 0.7;
const int CALIB_INTERVAL = 10000;
const unsigned long CHAR_GAP = 3000; // 3 sec for inter-character

// -------------------- CALIBRATION --------------------
void calibrateTouch() {
  Serial.println("🌀 Calibrating touch...");
  delay(500);
  for (int i = 0; i < 10; i++) touchRead(TOUCH_PIN);
  long total = 0;
  for (int i = 0; i < 50; i++) { total += touchRead(TOUCH_PIN); delay(10); }
  baseLevel = total / 50;
  touchThreshold = baseLevel * THRESHOLD_FACTOR;
  Serial.printf("✅ Base=%d Threshold=%d\n", baseLevel, touchThreshold);
}

// -------------------- TRANSLATION --------------------
char translateMorse(String s) {
  if (MORSE.count(s)) return MORSE[s];
  return '?';
}

// -------------------- DISPLAY --------------------
void drawGradientBar(int progress) {
  for (int i = 0; i < 128; i++) {
    int bright = (i + shimmerOffset) % 16 < 8 ? 1 : 0;
    if (i < progress && bright) {
      for (int y = 49; y <= 55; y++) u8g2.drawPixel(i, y);
    }
  }
}

void drawDisplay() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);

  int morseWidth = u8g2.getStrWidth(morseSeq.c_str());
  int textWidth = u8g2.getStrWidth(translated.c_str());

  // 1️⃣ Morse line
  int morseX = (morseWidth > 128) ? 128 - morseWidth + morseScroll : 0;
  u8g2.setCursor(morseX, 12);
  u8g2.print(morseSeq);
  if (morseWidth > 128) { morseScroll--; if (morseScroll < -8) morseScroll = 0; }

  // 2️⃣ Translation line
  int textX = (textWidth > 128) ? 128 - textWidth + textScroll : 0;
  u8g2.setCursor(textX, 26);
  u8g2.print(translated);
  if (textWidth > 128) { textScroll--; if (textScroll < -8) textScroll = 0; }

  // 3️⃣ Empty line (spacing)

  // 4️⃣ Animated progress bar
  drawGradientBar(progress);
  shimmerOffset = (shimmerOffset + 1) % 16;

  u8g2.sendBuffer();
}

// -------------------- TOUCH LOGIC --------------------
void handleTouch() {
  int raw = touchRead(TOUCH_PIN);
  bool touched = raw < touchThreshold;
  unsigned long now = millis();

  // --- Debounce ---
  static bool lastTouched = false;
  static unsigned long lastDebounce = 0;
  const unsigned long DEBOUNCE_MS = 50;

  if (touched != lastTouched) lastDebounce = now;
  if ((now - lastDebounce) > DEBOUNCE_MS) {

    // --- Touch start ---
    if (touched && !isPressed) {
      isPressed = true; isHeld = false; touchStart = now;
      tone(BUZZER_PIN, 1200);
      Serial.println("👉 Touch start");
    }

    // --- While pressed ---
    if (isPressed && touched) {
      unsigned long held = now - touchStart;
      progress = map(min((int)held, HOLD_TIME), 0, HOLD_TIME, 0, 128);
      if (held > HOLD_TIME && !isHeld) { isHeld = true; Serial.println("⏱ Hold detected"); }
    }

    // --- Touch release ---
    if (!touched && isPressed) {
      noTone(BUZZER_PIN);
      unsigned long duration = now - touchStart;
      isPressed = false; progress = 0;

      String symbol = (duration < HOLD_TIME) ? "." : "-";
      morseSeq += symbol;
      lastInputTime = now;

      Serial.printf("🟢 Symbol: %s (%lums)\n", symbol.c_str(), duration);

      drawDisplay(128); delay(80); drawDisplay();
    }

    lastTouched = touched;
  }

  // --- Inter-character check ---
  if (morseSeq.length() > 0 && (now - lastInputTime > CHAR_GAP)) {
    char c = translateMorse(morseSeq);
    translated += c;
    Serial.printf("🔤 Character completed: %c\n", c);
    morseSeq = "";
    lastInputTime = now;
  }

  // --- Auto recalibrate idle ---
  if (!touched && now - lastTouch > CALIB_INTERVAL && now - lastCalib > CALIB_INTERVAL) {
    calibrateTouch();
    lastCalib = now;
  }

  lastTouch = now;
}

// -------------------- WEB PORTAL --------------------
void handleRoot() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>DotDash CyberLink</title>
  <style>
    body {background: radial-gradient(circle at top left, #0f0f0f, #000); color: #0ff; font-family: 'Consolas', monospace; text-align:center; padding:20px;}
    h1 {color:#0ff;text-shadow:0 0 8px #0ff;}
    #morse,#text {font-size:1.5em;margin:20px auto;width:80%;padding:10px;border-radius:8px;background:rgba(0,255,255,0.1);box-shadow:inset 0 0 10px #0ff;}
    footer {font-size:0.8em;opacity:0.5;margin-top:40px;}
  </style>
</head>
<body>
  <h1>DotDash CyberLink</h1>
  <div id="morse">Waiting for signal...</div>
  <div id="text"></div>
  <footer>ESP32 Morse System ⚡ Upayan Edition</footer>
  <script>
    const evt=new EventSource('/live');
    evt.onmessage=e=>{
      const data=JSON.parse(e.data);
      document.getElementById('morse').innerText=data.morse;
      document.getElementById('text').innerText=data.text;
    };
  </script>
</body>
</html>
  )";
  webServer.send(200, "text/html", html);
}

void handleLive() {
  String json = "{\"morse\":\"" + morseSeq + "\",\"text\":\"" + translated + "\"}";
  webServer.send(200, "text/event-stream", "data: " + json + "\n\n");
}

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(115200);
  Serial.println("🚀 DotDash CyberLink Starting...");
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  WiFi.softAP(AP_SSID, AP_PASS);
  dnsServer.start(53, "*", WiFi.softAPIP());
  webServer.on("/", handleRoot);
  webServer.on("/live", handleLive);
  webServer.begin();
  tone(BUZZER_PIN, 800, 100); delay(100); tone(BUZZER_PIN, 1200, 100);
  calibrateTouch();
  drawDisplay();
}

// -------------------- LOOP --------------------
void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();
  handleTouch();
  drawDisplay();
  delay(30);
}
