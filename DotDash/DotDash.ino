#include <U8g2lib.h>
#include <Wire.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

#define TOUCH_PIN 34
#define BUZZER_PIN 12

// OLED setup (GME12864-40)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// WiFi AP
const char *ssid = "DotDash";
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
AsyncWebServer server(80);

// Morse code mapping
#include <map>
std::map<char, String> E = {
    {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."}, {'E', "."}, {'F', "..-."}, {'G', "--."}, {'H', "...."}, {'I', ".."}, {'J', ".---"}, {'K', "-.-"}, {'L', ".-.."}, {'M', "--"}, {'N', "-."}, {'O', "---"}, {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"}, {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"}, {'Y', "-.--"}, {'Z', "--.."}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"}, {'4', "....-"}, {'5', "....."}, {'6', "-...."}, {'7', "--..."}, {'8', "---.."}, {'9', "----."}, {'0', "-----"}, {' ', "/"}};

String morseInput = "";

void setup()
{
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // OLED
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(0, 20, "DotDash Ready");
  u8g2.sendBuffer();

  // WiFi AP
  WiFi.softAP(ssid);
  dnsServer.start(53, "*", apIP);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", morseInput); });
  server.begin();
}

void loop()
{
  dnsServer.processNextRequest();

  int touchVal = analogRead(TOUCH_PIN);
  if (touchVal > 1000)
  { // adjust threshold
    morseInput += ".";
    tone(BUZZER_PIN, 1000, 150);
    delay(200);
  }
  else if (touchVal < 1000 && touchVal > 500)
  {
    morseInput += "-";
    tone(BUZZER_PIN, 1000, 400);
    delay(500);
  }

  // Display on OLED
  u8g2.clearBuffer();
  u8g2.drawStr(0, 20, morseInput.c_str());
  u8g2.sendBuffer();
}
