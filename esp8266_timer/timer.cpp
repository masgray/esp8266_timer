#include "timer.h"

#include <ESP8266WiFi.h>

constexpr uint32_t OneSecond = 1000;

void T_OFF::Start(uint32_t seconds)
{
  m_isRun = true;
  UpdateTime(seconds);
  if (m_onStartTimer)
    m_onStartTimer();
}

void T_OFF::Stop()
{
  m_isRun = false;
  if (m_onStopTimer)
    m_onStopTimer();
}

void T_OFF::Loop() const
{
  if (m_isRun && (millis() >= m_millisWhenElapsed))
    m_onTimerFinished();
}

void T_OFF::UpdateTime(uint32_t seconds)
{
  m_millisWhenElapsed = millis() + seconds * OneSecond;
}

bool T_OFF::IsRun() const
{
  return m_isRun;
}

uint32_t T_OFF::ElapsedSeconds() const
{
  if (m_isRun)
    return (m_millisWhenElapsed - millis()) / OneSecond;
  return 0;
}

