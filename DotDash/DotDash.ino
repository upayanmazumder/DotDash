#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>

// -------------------- PINS --------------------
#define TOUCH_PIN 4
#define BUZZER_PIN 15
#define SDA_PIN 21
#define SCL_PIN 22

// -------------------- OLED --------------------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

// -------------------- WiFi AP / Captive portal --------------------
const char* AP_SSID = "DotDash";
const char* AP_PASS = "";
DNSServer dnsServer;
WebServer webServer(80);

// -------------------- Morse --------------------
String currentToken = "";
String liveMorse = "";

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  // OLED init
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.clearBuffer();
  u8g2.setCursor(0,12);
  u8g2.print("Starting DotDash...");
  u8g2.sendBuffer();

  pinMode(TOUCH_PIN, INPUT_PULLDOWN);
  pinMode(BUZZER_PIN, OUTPUT);

  // Start WiFi AP
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP: "); Serial.println(apIP);

  // Start DNS server (catch-all)
  dnsServer.start(53, "*", apIP);

  // Web server routes
  webServer.on("/", handleRoot);
  webServer.on("/live", handleLive);
  webServer.onNotFound(handleRoot);
  webServer.begin();

  // Startup beep sequence
  tone(BUZZER_PIN, 1000, 200); delay(250);
  tone(BUZZER_PIN, 1200, 200); delay(250);
  tone(BUZZER_PIN, 1500, 300); delay(350);

  Serial.println("DotDash ready!");
}

// -------------------- Web handlers --------------------
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>"
                "<title>DotDash Portal</title></head><body>"
                "<h1>DotDash Portal</h1>"
                "<p>Live Morse input:</p>"
                "<div id='morse'> </div>"
                "<script>"
                "var evtSource = new EventSource('/live');"
                "evtSource.onmessage = function(e){"
                "document.getElementById('morse').innerText = e.data;"
                "};"
                "</script></body></html>";
  webServer.send(200, "text/html", html);
}

void handleLive() {
  webServer.sendHeader("Content-Type", "text/event-stream");
  webServer.sendHeader("Cache-Control", "no-cache");
  webServer.sendHeader("Connection", "keep-alive");
  webServer.send(200, "text/event-stream", liveMorse + "\n\n");
}

// -------------------- Word-wrap helper --------------------
void drawWrappedText(String text, int x, int y, int maxWidth, int lineHeight){
  int start = 0;
  int len = text.length();
  while(start < len){
    int end = start;
    String line = "";
    while(end < len){
      line += text[end];
      if(u8g2.getStrWidth(line.c_str()) > maxWidth){
        line.remove(line.length()-1);
        break;
      }
      end++;
    }
    u8g2.setCursor(x, y);
    u8g2.print(line);
    y += lineHeight;
    start = end;
  }
}

// -------------------- Touch & buzzer --------------------
void checkTouch() {
  static bool pressed = false;
  static unsigned long pressStart = 0;
  static bool symbolAdded = false;  // ensure we only add once
  int state = digitalRead(TOUCH_PIN);

  if (state && !pressed) {
    // just pressed
    pressed = true;
    pressStart = millis();
    symbolAdded = false;
    tone(BUZZER_PIN, 1000);  // start passive buzzer
  }

  if (!state && pressed) {
    // just released
    unsigned long duration = millis() - pressStart;
    noTone(BUZZER_PIN);       // stop buzzer

    // determine symbol
    String symbol = (duration < 200) ? "." : "-";
    currentToken += symbol;
    liveMorse = currentToken; // update browser/OLED

    pressed = false;
  }
}



// -------------------- OLED --------------------
void updateOLED(){
  u8g2.clearBuffer();
  u8g2.setCursor(0,0);
  u8g2.print("Morse: "); 
  drawWrappedText(currentToken, 0, 12, u8g2.getDisplayWidth(), 12);
  u8g2.sendBuffer();
}

// -------------------- Main loop --------------------
void loop(){
  dnsServer.processNextRequest();
  webServer.handleClient();
  checkTouch();
  updateOLED();
}
