#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "pass.h"
#include "timer.h"

constexpr int GPIO_BUZZER PROGMEM = 5;
constexpr int GPIO_RELAY PROGMEM = 4;

constexpr uint32_t DefaultTimeToCook = 30;//40*60;//40 min
constexpr uint32_t DefaultBuzzerCounter = 5;

static uint32_t timeToCook = DefaultTimeToCook;
static T_OFF cookTimer;
static T_OFF buzzerTimer;
static bool buzzerOn = false;
static uint32_t buzzerCounter = DefaultBuzzerCounter;

static T_OFF mqttTimer;
constexpr const char* MqttServer PROGMEM = "192.168.0.3";
constexpr const uint16_t MqttPort PROGMEM = 1883;

constexpr const char* DeviceId PROGMEM = "unit1_device3";

namespace Topics
{
constexpr const char* SetTime PROGMEM = "unit1/device3/timer/set_time";
constexpr const char* AddTime PROGMEM = "unit1/device3/timer/add_time";
constexpr const char* RemoveTime PROGMEM = "unit1/device3/timer/remove_time";
constexpr const char* StartTimer PROGMEM = "unit1/device3/timer/start";
constexpr const char* StopTimer PROGMEM = "unit1/device3/timer/stop";

constexpr const char *TimerStarted PROGMEM = "unit1/device3/status/timer_started";
constexpr const char *TimeElapsed PROGMEM = "unit1/device3/status/time_elapsed";
constexpr const char *RelayIsOn PROGMEM = "unit1/device3/status/relay_is_on";
}

bool MqttConnect();
void MqttCallback(char* topic, byte* payload, unsigned int length);

WiFiClient espClient;
PubSubClient mqtt(MqttServer, MqttPort, MqttCallback, espClient);

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
  digitalWrite(GPIO_RELAY, HIGH);

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //TMP:
  cookTimer.Start(DefaultTimeToCook);
}

void loop() 
{
  if (cookTimer.IsElapsed())
  {
    Serial.println("Cookie finished!");
    digitalWrite(GPIO_RELAY, LOW);
    cookTimer.Stop();
    buzzerOn = true;
    buzzerCounter = DefaultBuzzerCounter;
    buzzerTimer.Start(0);
  }

  if (buzzerTimer.IsElapsed())
  {
    if (buzzerCounter)
    {
      --buzzerCounter;
      if (buzzerOn)
        tone(GPIO_BUZZER, 3000);
      else
        noTone(GPIO_BUZZER);
      buzzerOn = !buzzerOn;
      buzzerTimer.Start(1);      
    }
    else
    {
        noTone(GPIO_BUZZER);
        buzzerTimer.Stop();
    }
  }

  if (MqttConnect()) 
  {
    mqtt.loop();
  }

  if (mqttTimer.IsElapsed())
  {
    auto isRun = !cookTimer.IsElapsed();
    mqtt.publish(Topics::TimerStarted, String(isRun).c_str());
    if (isRun)
    {
      auto sec = cookTimer.ElapsedSeconds();
      Serial.print("Elapsed: ");
      Serial.print(sec);
      Serial.println(" seconds.");
      mqtt.publish(Topics::TimeElapsed, String(sec).c_str());
    }
    else
    {
      mqtt.publish(Topics::TimeElapsed, 0);
    }
    mqtt.publish(Topics::RelayIsOn, String(digitalRead(GPIO_RELAY)).c_str());
    mqttTimer.Start(1);
  }
}

bool StrEq(const char* s1, const char* s2)
{
  if (strlen(s1) != strlen(s2))
    return false;
  return strcmp(s1, s2) == 0;
}

void MqttCallback(char* topic, byte* payload, unsigned int length) 
{
  static char buffer[1024];
  if (length > 1023)
    length = 1023;
  buffer[length] = 0;
  memcpy(buffer, payload, length);
  String text(buffer);

  if (StrEq(topic, Topics::SetTime))
  {
    timeToCook = text.toInt() * 60;
  }
  else if (StrEq(topic, Topics::AddTime))
  {
    auto sec = text.toInt() * 60;
    cookTimer.AddSeconds(sec);
    timeToCook += sec;
  }
  else if (StrEq(topic, Topics::RemoveTime))
  {
    auto sec = text.toInt() * 60;
    cookTimer.RemoveSeconds(sec);
    if (timeToCook > sec)
      timeToCook -= sec;
  }
  else if (StrEq(topic, Topics::StartTimer))
  {
    cookTimer.Start(timeToCook);
  }
  else if (StrEq(topic, Topics::StopTimer))
  {
    cookTimer.Stop();
  }
}

bool MqttConnect() 
{
  if (mqtt.connected())
    return true;
  
  Serial.print("Attempting MQTT connection...");
  if (mqtt.connect(DeviceId))
  {
    Serial.println("connected");
    mqtt.subscribe(Topics::SetTime);
    mqtt.subscribe(Topics::AddTime);
    mqtt.subscribe(Topics::RemoveTime);
    mqtt.subscribe(Topics::StartTimer);
    mqtt.subscribe(Topics::StopTimer);
    mqttTimer.Start(0);
    return true;
  } 
  Serial.print("Failed, result=");
  Serial.print(mqtt.state());
  Serial.println(" try again in 1 second");
  mqttTimer.Stop();
  delay(1000);
  return false;
}

