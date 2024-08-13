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

#define MYCILA_SYSTEM_VERSION "2.0.7"
#define MYCILA_SYSTEM_VERSION_MAJOR 2
#define MYCILA_SYSTEM_VERSION_MINOR 0
#define MYCILA_SYSTEM_VERSION_REVISION 7

namespace Mycila {
  typedef struct {
      uint32_t total;
      uint32_t used;
      uint32_t free;
      float usage;
  } SystemMemory;

  class SystemClass {
    public:
      void begin(bool initFS = true, const char* fsPartitionName = "fs", const char *basePath = "/littlefs", uint8_t maxOpenFiles = 10);

      void reset();
      void restart(uint32_t delayMillisBeforeRestart = 0);
      bool restartFactory(const char* partitionName);
      void deepSleep(uint64_t delayMicros);

      // returns the uptime in seconds
      inline uint32_t getUptime() const { return static_cast<uint32_t>(esp_timer_get_time() / (int64_t)1000000); }
      const SystemMemory getMemory() const;
      uint32_t getBootCount() const { return _boots; }

#ifdef MYCILA_JSON_SUPPORT
      void toJson(const JsonObject& root) const;
#endif

    public:
      static String getEspID();

    private:
      uint32_t _boots = 0;
      Ticker _delayedTask;
  };

  extern SystemClass System;
} // namespace Mycila
