// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#pragma once

#include <Ticker.h>
#include <esp_timer.h>

#include <functional>
#include <string>

#ifdef MYCILA_JSON_SUPPORT
  #include <ArduinoJson.h>
#endif

#define MYCILA_SYSTEM_VERSION          "4.1.0"
#define MYCILA_SYSTEM_VERSION_MAJOR    4
#define MYCILA_SYSTEM_VERSION_MINOR    1
#define MYCILA_SYSTEM_VERSION_REVISION 0

namespace Mycila {
  class System {
    public:
      typedef struct {
          size_t total;
          size_t used;
          size_t free;
          size_t minimumFree;
          float usage;
      } Memory;

      typedef struct {
          std::string task;
          std::string reason;
          std::string backtrace;
      } Coredump;

      static void init(bool initFS = true, const char* fsPartitionName = "fs", const char* basePath = "/littlefs", uint8_t maxOpenFiles = 10);

      static void reset(uint32_t delayMillisBeforeRestartMillis = 0);
      static void restart(uint32_t delayMillisBeforeRestartMillis = 0);
      static void deepSleep(uint64_t delayMicros);

      // See MycilaSafeBoot for more info:
      // https://github.com/mathieucarbou/MycilaSafeBoot
      static bool restartFactory(const char* partitionName = "safeboot", uint32_t delayMillisBeforeRestartMillis = 0);

      static uint32_t getChipID();
      static std::string getChipIDStr();
      // returns the uptime in seconds
      static inline uint32_t getUptime() { return static_cast<uint32_t>(esp_timer_get_time() / static_cast<int64_t>(1000000)); }
      static void getMemory(Memory& memory); // NOLINT
      static uint32_t getBootCount() { return _boots; }
      static const char* getLastRebootReason();
      static bool readCoredump(Coredump& coredump); // NOLINT

#ifdef MYCILA_JSON_SUPPORT
      static void toJson(const JsonObject& root);
#endif

    private:
      static Ticker _delayedTask;
      static uint32_t _boots;
      static uint32_t _chipID;
  };
} // namespace Mycila
