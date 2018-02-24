#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "pass.h"
#include "timer.h"

constexpr int GPIO_BUZZER PROGMEM = 5;
constexpr int GPIO_RELAY PROGMEM = 4;

constexpr uint32_t DefaultTimeToCook = 40*60;//40 min
constexpr uint32_t DefaultBuzzerCounter = 5;

void OnStartTimer();
void OnStopTimer();
void OnCookTimerFinished();

static uint32_t timeToCook = DefaultTimeToCook;
static T_OFF cookTimer(&OnCookTimerFinished, &OnStartTimer, OnStopTimer);

void OnBuzzerTimerFinished();

static T_OFF buzzerTimer(&OnBuzzerTimerFinished);
static bool buzzerOn = false;
static uint32_t buzzerCounter = DefaultBuzzerCounter;

static bool MqttStartTimerCmd = false;

void OnMqttTimerFinished();

static T_OFF mqttTimer(&OnMqttTimerFinished);
constexpr const char* MqttServer PROGMEM = "192.168.0.3";
constexpr const uint16_t MqttPort PROGMEM = 1883;

constexpr const char* DeviceId PROGMEM = "unit1_device3";

namespace Topics
{
constexpr const char* SetTime PROGMEM         = "unit1/device3/timer/set_time";
constexpr const char* StartTimer PROGMEM      = "unit1/device3/timer/start";

constexpr const char* TimerStarted PROGMEM    = "unit1/device3/status/timer_started";
constexpr const char* TimeElapsed PROGMEM     = "unit1/device3/status/time_elapsed";
constexpr const char* RelayIsOn PROGMEM       = "unit1/device3/status/relay_is_on";
}

void MqttCallback(char* topic, byte* payload, unsigned int length);
bool MqttConnect();
void PublishCurrentStatus();
void PublishCurrentSettings();

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
}

void loop() 
{
  if (MqttConnect())
  {
    mqtt.loop();
  }

  cookTimer.Loop();

  buzzerTimer.Loop();

  mqttTimer.Loop();
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
    Serial.print("MQTT SetTime: ");
    Serial.print(timeToCook);
    Serial.println(" sec.");
    if (MqttStartTimerCmd)
    {
      cookTimer.UpdateTime(timeToCook);
    }
  }
  else if (StrEq(topic, Topics::StartTimer))
  {
    Serial.print("MQTT StartTimer: ");
    auto isOn = text.toInt()  != 0;
    Serial.println(isOn ? "On" : "Off");
    if (MqttStartTimerCmd != isOn)
    {
      MqttStartTimerCmd = isOn;
      Serial.println("  ...new timer on comand.");
      if (isOn)
        cookTimer.Start(timeToCook);
      else
        cookTimer.Stop();
    }
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
    
    PublishCurrentStatus();
    PublishCurrentSettings();

    mqtt.subscribe(Topics::SetTime);
    mqtt.subscribe(Topics::StartTimer);
    
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

String NumToStr(uint32_t num)
{
  auto s = String(num);
  if (s.length() < 2)
    s = '0' + s;
  return s;
}

void PublishCurrentStatus()
{
  Serial.print("Publish is run: ");
  auto isRun = cookTimer.IsRun();
  Serial.println(isRun);
  mqtt.publish(Topics::TimerStarted, String(isRun).c_str());
  if (isRun)
  {
    auto sec = cookTimer.ElapsedSeconds();
    auto s = NumToStr(sec / 3600) + ':';
    s += NumToStr((sec % 3600) / 60) + ':';
    s += NumToStr((sec % 3600) % 60);
    Serial.print("Publish elapsed: ");
    Serial.println(s);
    mqtt.publish(Topics::TimeElapsed, s.c_str(), true);
  }
  else
  {
    mqtt.publish(Topics::TimeElapsed, "00:00:00", true);
  }

  Serial.print("Publish RelayIsOn: ");
  auto r = digitalRead(GPIO_RELAY);
  Serial.println(r);
  mqtt.publish(Topics::RelayIsOn, String(r == 0).c_str(), true);
}

void PublishCurrentSettings()
{
  Serial.print("PublishCurrentSettings. Is timer start: ");
  Serial.println(MqttStartTimerCmd);
  mqtt.publish(Topics::SetTime, String(timeToCook / 60).c_str(), true);
  mqtt.publish(Topics::StartTimer, String(MqttStartTimerCmd).c_str(), true);
}

void OnStartTimer()
{
  Serial.println("Power ON relay.");
  digitalWrite(GPIO_RELAY, LOW);//inverse relay
}

void OnStopTimer()
{
  Serial.println("Power OFF relay.");
  digitalWrite(GPIO_RELAY, HIGH);//inverse relay
}

void OnCookTimerFinished()
{
  Serial.println("Cooks finished!");
  cookTimer.Stop();
  buzzerOn = true;
  buzzerCounter = DefaultBuzzerCounter;
  buzzerTimer.Start(0);
  MqttStartTimerCmd = false;
  PublishCurrentSettings();
}

void OnBuzzerTimerFinished()
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

void OnMqttTimerFinished()
{
  PublishCurrentStatus();
  mqttTimer.Start(1);
}

