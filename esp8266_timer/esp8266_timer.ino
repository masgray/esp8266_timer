#include <ESP8266WiFi.h>

#include "pass.h"

constexpr int GPIO_BUZZER PROGMEM = 5;
constexpr int GPIO_RELAY PROGMEM = 4;

void setup() 
{
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(Ssid, Password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  pinMode(GPIO_BUZZER, OUTPUT);
  pinMode(GPIO_RELAY, OUTPUT);

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() 
{
  tone(GPIO_BUZZER, 3000);
  digitalWrite(GPIO_RELAY, LOW);
  delay(1000);
  noTone(GPIO_BUZZER);
  digitalWrite(GPIO_RELAY, HIGH);
  delay(1000);
}
