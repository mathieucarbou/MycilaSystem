// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou and others
 */
#include <MycilaSystem.h>

#include <LittleFS.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)
#include <esp_bt.h>
#include <esp_bt_main.h>
#endif

#ifdef MYCILA_LOGGER_SUPPORT
#include <MycilaLogger.h>
extern Mycila::Logger logger;
#define LOGD(tag, format, ...) logger.debug(tag, format, ##__VA_ARGS__)
#define LOGI(tag, format, ...) logger.info(tag, format, ##__VA_ARGS__)
#define LOGW(tag, format, ...) logger.warn(tag, format, ##__VA_ARGS__)
#define LOGE(tag, format, ...) logger.error(tag, format, ##__VA_ARGS__)
#else
#define LOGD(tag, format, ...) ESP_LOGD(tag, format, ##__VA_ARGS__)
#define LOGI(tag, format, ...) ESP_LOGI(tag, format, ##__VA_ARGS__)
#define LOGW(tag, format, ...) ESP_LOGW(tag, format, ##__VA_ARGS__)
#define LOGE(tag, format, ...) ESP_LOGE(tag, format, ##__VA_ARGS__)
#endif

#define TAG "SYSTEM"

#define KEY_BOOTS "boots"
#define KEY_RESETS "resets"

#ifndef MYCILA_NVM_RESET_BOOT_DELAY
#define MYCILA_NVM_RESET_BOOT_DELAY 3
#endif

void Mycila::SystemClass::begin() {
  LOGI(TAG, "Initializing File System...");
  nvs_flash_init();
  if (LittleFS.begin(false))
    LOGD(TAG, "File System initialized");
  else {
    LOGW(TAG, "File System initialization failed. Trying to format...");
    if (LittleFS.begin(true))
      LOGW(TAG, "Successfully formatted and initialized");
    else
      LOGE(TAG, "Failed to format");
  }

  Preferences prefs;
  prefs.begin(TAG, false);

  _boots = (prefs.isKey(KEY_BOOTS) ? prefs.getULong(KEY_BOOTS, 0) : 0) + 1;
  prefs.putULong(KEY_BOOTS, _boots);
  LOGI(TAG, "Booted %" PRIu32 " times", _boots);

#ifdef MYCILA_SYSTEM_BOOT_WAIT_FOR_RESET
  const int count = (prefs.isKey(KEY_RESETS) ? prefs.getInt(KEY_RESETS, 0) : 0) + 1;
  prefs.putInt(KEY_RESETS, count);
  if (count >= 3) {
    prefs.end();
    reset();
  } else {
    Logger.warn(TAG, "WAITING FOR HARD RESET...");
    for (uint32_t d = 0; d < MYCILA_NVM_RESET_BOOT_DELAY * 1000UL; d += 500) {
      delay(500);
    }
    prefs.remove(KEY_RESETS);
    LOGD(TAG, "No hard reset");
  }
#endif

  prefs.end();
}

void Mycila::SystemClass::reset() {
  LOGD(TAG, "Triggering System Reset...");
  nvs_flash_erase();
  nvs_flash_init();
  esp_restart();
}

void Mycila::SystemClass::restart(uint32_t delayMillisBeforeRestart) {
  LOGD(TAG, "Triggering System Restart...");
  if (delayMillisBeforeRestart == 0)
    esp_restart();
  else {
    _delayedTask.once_ms(delayMillisBeforeRestart, esp_restart);
  }
}

void Mycila::SystemClass::deepSleep(uint64_t delayMicros) {
  LOGI(TAG, "Deep Sleep for %" PRIu64 " us!", delayMicros);

#if SOC_UART_NUM > 2
  Serial2.end();
#endif
#if SOC_UART_NUM > 1
  Serial1.end();
#endif
  Serial.end();

  esp_wifi_stop();

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)
  esp_bt_controller_disable();
  esp_bluedroid_disable();
#endif

  // equiv to esp_deep_sleep(delayMicros);
  // esp_sleep_enable_timer_wakeup(seconds * (uint64_t)1000000ULL);
  // esp_deep_sleep_start();

  esp_deep_sleep(delayMicros);

  esp_restart();
}

const Mycila::SystemMemory Mycila::SystemClass::getMemory() const {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
  return {
    .total = info.total_free_bytes + info.total_allocated_bytes,
    .used = info.total_allocated_bytes,
    .free = info.total_free_bytes,
    .usage = round(static_cast<float>(info.total_allocated_bytes) / static_cast<float>(info.total_free_bytes + info.total_allocated_bytes) * 10000) / 100};
}

#ifdef MYCILA_JSON_SUPPORT
void Mycila::SystemClass::toJson(const JsonObject& root) const {
  SystemMemory memory = getMemory();
  root["boots"] = _boots;
  root["chip_cores"] = ESP.getChipCores();
  root["chip_model"] = ESP.getChipModel();
  root["chip_revision"] = ESP.getChipRevision();
  root["cpu_freq"] = ESP.getCpuFreqMHz();
  root["heap_total"] = memory.total;
  root["heap_usage"] = memory.usage;
  root["heap_used"] = memory.used;
  root["uptime"] = getUptime();
}
#endif

namespace Mycila {
  SystemClass System;
} // namespace Mycila
