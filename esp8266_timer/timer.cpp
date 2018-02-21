#include "timer.h"

#include <ESP8266WiFi.h>

constexpr uint32_t OneSecond = 1000;

void T_OFF::Start(uint32_t seconds)
{
  m_isRun = true;
  m_millisWhenElapsed = millis() + seconds * OneSecond;
}

void T_OFF::Stop()
{
  m_isRun = false;
}

bool T_OFF::IsElapsed() const
{
  return m_isRun && millis() >= m_millisWhenElapsed;
}

uint32_t T_OFF::ElapsedSeconds() const
{
  return (m_millisWhenElapsed - millis()) / OneSecond;
}

void T_OFF::AddSeconds(uint32_t seconds)
{
  m_millisWhenElapsed += seconds * OneSecond;
}

void T_OFF::RemoveSeconds(uint32_t seconds)
{
  if (ElapsedSeconds() > seconds)
    m_millisWhenElapsed -= seconds * OneSecond;
  else
    m_millisWhenElapsed = millis();
}

