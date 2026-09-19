#pragma once
#include <stdint.h>

enum ScheduleEdge : uint8_t { SCHEDULE_RESYNC, SCHEDULE_NONE, SCHEDULE_START, SCHEDULE_STOP };
struct PumpSchedule {
  uint32_t timestamp = 0;
  uint32_t tick = 0;
  uint16_t start = 0;
  uint16_t stop = 0;
  bool initialized = false;
  bool inside = false;
};

// Observe only: a boot, clock correction or edited schedule never issues a command.
inline ScheduleEdge observePumpSchedule(PumpSchedule &s, uint32_t timestamp,
                                       uint32_t tick, uint16_t minute,
                                       uint16_t start, uint16_t stop) {
  bool inside = start < stop ? (minute >= start && minute < stop)
                            : (minute >= start || minute < stop);
  uint32_t elapsedMs = tick - s.tick;
  int32_t clockError = static_cast<int32_t>(timestamp - s.timestamp) -
                       static_cast<int32_t>(elapsedMs / 1000UL);
  bool resync = !s.initialized || start != s.start || stop != s.stop ||
                timestamp < s.timestamp || elapsedMs > 60000UL ||
                clockError < -3 || clockError > 3;
  bool changed = inside != s.inside;
  s.timestamp = timestamp;
  s.tick = tick;
  s.start = start;
  s.stop = stop;
  s.inside = inside;
  s.initialized = true;
  return resync ? SCHEDULE_RESYNC :
         (changed ? (inside ? SCHEDULE_START : SCHEDULE_STOP) : SCHEDULE_NONE);
}
