#pragma once

#include <cstdint>
#include <limits>

typedef void(*CallbackType)();

class T_OFF
{
public:
  T_OFF(CallbackType onTimerFinished, CallbackType onStartTimer = nullptr, CallbackType onStopTimer = nullptr)
    : m_onTimerFinished(onTimerFinished)
    , m_onStartTimer(onStartTimer)
    , m_onStopTimer(onStopTimer) {}

  void Start(uint32_t seconds);
  void Stop();
  void Loop() const;
  void UpdateTime(uint32_t seconds);
  bool IsRun() const;
  uint32_t ElapsedSeconds() const;
  
private:
  CallbackType m_onTimerFinished;
  CallbackType m_onStartTimer;
  CallbackType m_onStopTimer;
  uint32_t m_millisWhenElapsed = std::numeric_limits<uint32_t>::max();
  bool m_isRun = false;
};


