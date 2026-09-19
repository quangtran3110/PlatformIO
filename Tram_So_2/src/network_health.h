#pragma once
#include <stdint.h>

// Actual Blynk PING/RESPONSE round trip, including time spent waiting to send.
struct NetworkHealth {
  uint8_t level = 0, good = 0, failures = 0;
  uint32_t lastReply = 0, rtt = 0, maxLoopGap = 0;
  bool replied = false;

  void fail() {
    level = 0;
    good = 0;
    if (failures < 255) ++failures;
  }

  void reply(uint32_t now, uint32_t duration) {
    lastReply = now;
    replied = true;
    rtt = duration;
    failures = 0;
    if (duration > 1500) {
      level = 0;
      good = 0;
    } else if (duration > 300) {
      level = 1;
      good = 0;
    } else {
      if (good < 3) ++good;
      level = good >= 3 ? 2 : 1;
    }
  }

  void service(uint32_t now, bool online) {
    if (!online || !replied || uint32_t(now - lastReply) > 8000) {
      level = 0;
      good = 0;
    }
  }

  bool allowControl(uint32_t now) const {
    return level > 0 && replied && uint32_t(now - lastReply) <= 8000;
  }
};
