void setup()
{
  // Runs once when the ESP32 boots
  // You can put initialization code here
  Serial.begin(115200);
  Serial.println("ESP32 started!");
}

void loop()
{
  // Runs repeatedly after setup()
  Serial.println("Hello from ESP32");
  delay(1000); // wait 1 second
}
