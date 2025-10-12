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

// -------------------- Morse code --------------------
#include <map>
std::map<char, String> CHAR_TO_MORSE = {
  {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."}, {'E', "."}, {'F', "..-."},
  {'G', "--."}, {'H', "...."}, {'I', ".."}, {'J', ".---"}, {'K', "-.-"}, {'L', ".-.."},
  {'M', "--"}, {'N', "-."}, {'O', "---"}, {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."},
  {'S', "..."}, {'T', "-"}, {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"},
  {'Y', "-.--"}, {'Z', "--.."},
  {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"}, {'5', "....."},
  {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."}, {'0', "-----"},
  {' ', "/"}
};
std::map<String, char> MORSE_TO_CHAR;
String currentToken = "";
String decodedMessage = "";
String liveMorse = "";

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  // OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.clearBuffer();
  u8g2.setCursor(0,12);
  u8g2.print("Starting DotDash...");
  u8g2.sendBuffer();

  pinMode(TOUCH_PIN, INPUT_PULLDOWN);
  pinMode(BUZZER_PIN, OUTPUT);

  // Morse reverse map
  for(auto &p : CHAR_TO_MORSE) MORSE_TO_CHAR[p.second] = p.first;

  // Start WiFi AP
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP IP: "); Serial.println(apIP);

  // Start DNS server
  dnsServer.start(53, "*", apIP);

  // Web server root
  webServer.on("/", handleRoot);
  
  // SSE endpoint for live Morse updates
  webServer.on("/live", handleLive);
  
  // Send message manually
  webServer.on("/send", [](){
    String msg = webServer.arg("m");
    decodedMessage += msg;
    webServer.send(200, "text/plain", "OK");
  });

  // Redirect all unknown requests to root
  webServer.onNotFound(handleRoot);

  webServer.begin();
  Serial.println("Captive portal ready!");
}

// -------------------- Web handlers --------------------
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width'>"
                "<title>DotDash Portal</title></head><body>"
                "<h1>DotDash Portal</h1>"
                "<p>Live Morse input:</p>"
                "<div id='morse'> </div>"
                "<p>Decoded text:</p>"
                "<div id='decoded'> </div>"
                "<input id='msg' placeholder='Type message'><button onclick='sendMsg()'>Send</button>"
                "<script>"
                "var evtSource = new EventSource('/live');"
                "evtSource.onmessage = function(e){"
                "let data = JSON.parse(e.data);"
                "document.getElementById('morse').innerText = data.morse;"
                "document.getElementById('decoded').innerText = data.decoded;"
                "};"
                "function sendMsg(){"
                "fetch('/send?m='+encodeURIComponent(document.getElementById('msg').value));"
                "document.getElementById('msg').value='';"
                "}"
                "</script></body></html>";
  webServer.send(200, "text/html", html);
}

void handleLive() {
  webServer.sendHeader("Content-Type", "text/event-stream");
  webServer.sendHeader("Cache-Control", "no-cache");
  webServer.sendHeader("Connection", "keep-alive");
  
  String data = "{\"morse\":\"" + liveMorse + "\",\"decoded\":\"" + decodedMessage + "\"}";
  webServer.send(200, "text/event-stream", "data: " + data + "\n\n");
}

// -------------------- Morse helpers --------------------
char decodeMorseToken(String token){
  if(MORSE_TO_CHAR.count(token)) return MORSE_TO_CHAR[token];
  return '?';
}

// -------------------- Touch input --------------------
void checkTouch(){
  static bool pressed = false;
  static unsigned long pressStart = 0;
  static unsigned long lastInputTime = 0;
  int state = digitalRead(TOUCH_PIN);
  unsigned long now = millis();

  // Touch pressed
  if(state && !pressed){
    pressed = true;
    pressStart = now;
    tone(BUZZER_PIN, 1000);
  }

  // Touch released
  if(!state && pressed){
    pressed = false;
    noTone(BUZZER_PIN);
    unsigned long duration = now - pressStart;
    String symbol = (duration < 200) ? "." : "-";
    currentToken += symbol;
    liveMorse += symbol;
    lastInputTime = now;
  }

  // Letter end
  if(!pressed && currentToken.length() && (now - lastInputTime > 600)){
    char decodedChar = decodeMorseToken(currentToken);
    decodedMessage += decodedChar;
    currentToken = "";
    liveMorse = "";
    lastInputTime = now;
  }

  // Word end
  if(!pressed && (now - lastInputTime > 1400) && decodedMessage.length() && decodedMessage.charAt(decodedMessage.length()-1) != ' '){
    decodedMessage += " ";
  }
}

// -------------------- OLED --------------------
void updateOLED(){
  u8g2.clearBuffer();
  u8g2.setCursor(0,12);
  u8g2.print("Decoded:");
  u8g2.setCursor(0,26);
  u8g2.print(decodedMessage);
  u8g2.sendBuffer();
}

// -------------------- Main loop --------------------
void loop(){
  dnsServer.processNextRequest();
  webServer.handleClient();
  checkTouch();
  updateOLED();
}
