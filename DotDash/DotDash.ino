#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <U8g2lib.h>
#include <map>
#include <string>
using namespace std;

const char *apSSID = "DotDash";
const uint8_t DNS_PORT = 53;
IPAddress apIP(8, 8, 8, 8);

#define TOUCH_PIN 34
#define BUZZER_PIN 25
#define SDA_PIN 21
#define SCL_PIN 22

#define DOT_MS 120
#define DASH_MS (DOT_MS * 3)
#define INTER_LETTER (DOT_MS * 3)
#define INTER_WORD (DOT_MS * 7)
#define TOUCH_THRESHOLD 1500

U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);
DNSServer dnsServer;
WebServer server(80);

String currentToken = "";
String decoded = "";
unsigned long lastTouch = 0;
bool touching = false;

std::map<String, char> MORSE = {
    {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'}, {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'}, {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'}, {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'}, {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'}, {"--..", 'Z'}};

void beep(int dur)
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(dur);
  digitalWrite(BUZZER_PIN, LOW);
}

void playMorse(const String &msg)
{
  for (char c : msg)
  {
    if (c == '.')
      beep(DOT_MS);
    else if (c == '-')
      beep(DASH_MS);
    else if (c == ' ')
      delay(INTER_LETTER);
    else if (c == '/')
      delay(INTER_WORD);
    delay(DOT_MS);
  }
}

String encode(String text)
{
  std::map<char, String> E = {
      {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."}, {'E', "."}, {'F', "..-."}, {'G', "--."}, {'H', "...."}, {'I', ".."}, {'J', ".---"}, {'K', "-.-"}, {'L', ".-.."}, {'M', "--"}, {'N', "-."}, {'O', "---"}, {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"}, {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"}, {'Y', "-.--"}, {'Z', "--.."}};
  String out = "";
  for (char c : text)
  {
    c = toupper(c);
    if (c == ' ')
      out += " / ";
    else if (E.count(c))
    {
      if (!out.isEmpty())
        out += " ";
      out += E[c];
    }
  }
  return out;
}

String page = R"(
<html><body>
<h2>DotDash</h2>
<input id='msg' placeholder='Type text'/><button onclick='snd()'>Send</button>
<p id='s'></p>
<script>
function snd(){
  fetch('/send?txt='+encodeURIComponent(msg.value))
  .then(r=>r.text()).then(t=>s.innerText=t);
}
setInterval(()=>fetch('/stat').then(r=>r.text()).then(t=>s.innerText=t),1000);
</script></body></html>)";

void handleRoot() { server.send(200, "text/html", page); }
void handleSend()
{
  String txt = server.arg("txt");
  String m = encode(txt);
  server.send(200, "text/plain", "Sending: " + m);
  xTaskCreate([](void *p)
              {
    String morse = *(String*)p;
    playMorse(morse);
    delete (String*)p;
    vTaskDelete(NULL); }, "beep", 4096, new String(m), 1, NULL);
}
void handleStat()
{
  server.send(200, "text/plain", "Decoded: " + decoded);
}

void startAP()
{
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  dnsServer.start(DNS_PORT, "*", apIP);
  server.on("/", handleRoot);
  server.on("/send", handleSend);
  server.on("/stat", handleStat);
  server.begin();
}

void checkTouch()
{
  int val = analogRead(TOUCH_PIN);
  unsigned long t = millis();

  bool now = (val > TOUCH_THRESHOLD);
  if (now && !touching)
  {
    touching = true;
    lastTouch = t;
  }
  if (!now && touching)
  {
    touching = false;
    int dur = t - lastTouch;
    currentToken += (dur < 300) ? '.' : '-';
    lastTouch = t;
  }

  if (!touching && currentToken.length() > 0 && (t - lastTouch > INTER_LETTER))
  {
    if (MORSE.count(currentToken))
      decoded += MORSE[currentToken];
    else
      decoded += '?';
    currentToken = "";
  }
}

void drawOLED()
{
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x12_tr);
  oled.drawStr(0, 10, "DotDash");
  oled.drawStr(0, 24, ("Seq: " + currentToken).c_str());
  oled.drawStr(0, 38, ("Dec: " + decoded).c_str());
  oled.drawStr(0, 52, "AP: DotDash");
  oled.sendBuffer();
}

void setup()
{
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  analogReadResolution(12);
  Serial.begin(115200);
  oled.begin();
  startAP();
  oled.drawStr(0, 10, "Starting...");
  oled.sendBuffer();
}

void loop()
{
  dnsServer.processNextRequest();
  server.handleClient();
  checkTouch();
  drawOLED();
  delay(50);
}
