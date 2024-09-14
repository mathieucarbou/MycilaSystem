// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#pragma once

#include <Ticker.h>
#include <WString.h>
#include <esp_timer.h>
#include <functional>

#ifdef MYCILA_JSON_SUPPORT
  #include <ArduinoJson.h>
#endif

#define MYCILA_SYSTEM_VERSION          "3.0.3"
#define MYCILA_SYSTEM_VERSION_MAJOR    3
#define MYCILA_SYSTEM_VERSION_MINOR    0
#define MYCILA_SYSTEM_VERSION_REVISION 3

namespace Mycila {
  class System {
    public:
      typedef struct {
          size_t total;
          size_t used;
          size_t free;
          float usage;
      } Memory;

      static void init(bool initFS = true, const char* fsPartitionName = "fs", const char* basePath = "/littlefs", uint8_t maxOpenFiles = 10);

      static void reset(uint32_t delayMillisBeforeRestartMillis = 0);
      static void restart(uint32_t delayMillisBeforeRestartMillis = 0);
      static void deepSleep(uint64_t delayMicros);

      // See MycilaSafeBoot for more info:
      // https://github.com/mathieucarbou/MycilaSafeBoot
      static bool restartFactory(const char* partitionName = "safeboot", uint32_t delayMillisBeforeRestartMillis = 0);

      static uint32_t getChipID();
      static String getChipIDStr();
      // returns the uptime in seconds
      static inline uint32_t getUptime() { return static_cast<uint32_t>(esp_timer_get_time() / (int64_t)1000000); }
      static void getMemory(Memory& memory);
      static uint32_t getBootCount() { return _boots; }
      static const char* getLastRebootReason();

#ifdef MYCILA_JSON_SUPPORT
      static void toJson(const JsonObject& root);
#endif

    private:
      static uint32_t _boots;
      static Ticker _delayedTask;
  };
} // namespace Mycila
