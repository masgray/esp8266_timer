#pragma once

#include <cstdint>
#include <limits>

class T_OFF
{
public:
  T_OFF() {}

  void Start(uint32_t seconds);
  void Stop();
  void AddSeconds(uint32_t seconds);
  void RemoveSeconds(uint32_t seconds);

  bool IsElapsed() const;
  uint32_t ElapsedSeconds() const;
  
private:
  uint32_t m_millisWhenElapsed = std::numeric_limits<uint32_t>::max();
  bool m_isRun = false;
};


