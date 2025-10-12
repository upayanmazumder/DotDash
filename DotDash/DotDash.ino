#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

// -------------------- PINS --------------------
#define TOUCH_PIN 4         // HW-494 touch DO
#define BUZZER_PIN 15       // Buzzer +
#define SDA_PIN 21
#define SCL_PIN 22

// -------------------- OLED --------------------
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

// -------------------- WiFi AP / Captive portal --------------------
const char* AP_SSID = "DotDash";
const char* AP_PASS = "";
IPAddress apIP(192,168,4,1);
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

std::map<String, char> MORSE_TO_CHAR; // reversed map
String currentToken = "";
String decodedMessage = "";

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  // I2C init
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.setCursor(0,12);
  u8g2.print("Starting DotDash...");
  u8g2.sendBuffer();

  // Pins
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);
  pinMode(BUZZER_PIN, OUTPUT);

  // Build MORSE_TO_CHAR
  for(auto &pair : CHAR_TO_MORSE) MORSE_TO_CHAR[pair.second] = pair.first;

  // WiFi AP + DNS
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  dnsServer.start(53, "*", apIP);

  // Web server routes
  webServer.on("/", [](){
    webServer.send(200, "text/html",
      "<h1>DotDash</h1>"
      "<p>Type Morse code below:</p>"
      "<input id='msg' placeholder='Message'>"
      "<button onclick='sendMsg()'>Send</button>"
      "<script>"
      "function sendMsg(){"
      "let m = document.getElementById('msg').value;"
      "fetch('/send?m='+encodeURIComponent(m));"
      "}"
      "</script>");
  });

  webServer.on("/send", [](){
    String msg = webServer.arg("m");
    decodedMessage += msg;
    webServer.send(200, "text/plain", "OK");
  });

  Serial.println("DotDash ready!");
}

// -------------------- Morse helpers --------------------
String encodeToMorse(String msg){
  msg.toUpperCase();
  String morse = "";
  for(int i=0; i<msg.length(); i++){
    char c = msg[i];
    if(CHAR_TO_MORSE.count(c)) morse += CHAR_TO_MORSE[c] + " ";
  }
  return morse;
}

char decodeMorseToken(String token){
  if(MORSE_TO_CHAR.count(token)) return MORSE_TO_CHAR[token];
  return '?';
}

// -------------------- Touch / Buzzer --------------------
void checkTouch(){
  static unsigned long lastTime = 0;
  static bool pressed = false;
  int state = digitalRead(TOUCH_PIN);
  unsigned long now = millis();

  if(state==HIGH && !pressed){
    pressed = true;
    lastTime = now;
    tone(BUZZER_PIN, 1000);
    currentToken += "."; // simple dot for now
    Serial.println("DOT detected");
  }

  if(state==LOW && pressed){
    pressed = false;
    noTone(BUZZER_PIN);
    decodedMessage += decodeMorseToken(currentToken);
    currentToken = "";
    lastTime = now;
  }
}

// -------------------- OLED display --------------------
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
  delay(50);
}
