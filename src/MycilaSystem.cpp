// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2024 Mathieu Carbou
 */
#include <MycilaSystem.h>

#include <LittleFS.h>
#include <Preferences.h>

#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)
  #include <esp_bt.h>
  #include <esp_bt_main.h>
#endif

#ifndef SOC_UART_HP_NUM
  #define SOC_UART_HP_NUM SOC_UART_NUM
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

#define KEY_BOOTS  "boots"
#define KEY_RESETS "resets"

uint32_t Mycila::System::_boots = 0;
Ticker Mycila::System::_delayedTask;

void Mycila::System::init(bool initFS, const char* fsPartitionName, const char* basePath, uint8_t maxOpenFiles) {
  LOGI(TAG, "Initializing NVM...");
  ESP_ERROR_CHECK(nvs_flash_init());

  Preferences prefs;
  if (prefs.begin(TAG, false)) {
    _boots = (prefs.isKey(KEY_BOOTS) ? prefs.getULong(KEY_BOOTS, 0) : 0) + 1;
    prefs.putULong(KEY_BOOTS, _boots);
    LOGI(TAG, "Booted %" PRIu32 " times", _boots);
    prefs.end();
  }

  if (initFS) {
    LOGI(TAG, "Initializing File System...");
    if (LittleFS.begin(false, basePath, maxOpenFiles, fsPartitionName))
      LOGD(TAG, "File System initialized");
    else {
      LOGW(TAG, "File System initialization failed. Trying to format...");
      if (LittleFS.begin(true, basePath, maxOpenFiles, fsPartitionName)) {
        LOGW(TAG, "Successfully formatted and initialized. Rebooting...");
        esp_restart();
      } else {
        LOGE(TAG, "Failed to format and initialize File System!");
      }
    }
  }
}

void Mycila::System::reset(uint32_t delayMillisBeforeRestartMillis) {
  LOGW(TAG, "Reset!");
  ESP_ERROR_CHECK(nvs_flash_erase());
  ESP_ERROR_CHECK(nvs_flash_init());
  restart(delayMillisBeforeRestartMillis);
}

void Mycila::System::restart(uint32_t delayMillisBeforeRestartMillis) {
  _delayedTask.detach();
  if (delayMillisBeforeRestartMillis == 0) {
    LOGW(TAG, "Restart!");
    esp_restart();
  } else {
    LOGW(TAG, "Restart in %" PRIu32 " ms...", delayMillisBeforeRestartMillis);
    _delayedTask.once_ms(delayMillisBeforeRestartMillis, esp_restart);
  }
}

bool Mycila::System::restartFactory(const char* partitionName, uint32_t delayMillisBeforeRestartMillis) {
  const esp_partition_t* partition = esp_partition_find_first(esp_partition_type_t::ESP_PARTITION_TYPE_APP, esp_partition_subtype_t::ESP_PARTITION_SUBTYPE_APP_FACTORY, partitionName);
  if (partition) {
    LOGW(TAG, "Set boot partition to %s", partitionName);
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(partition));
    restart(delayMillisBeforeRestartMillis);
    return true;
  } else {
    ESP_LOGE(TAG, "Partition not found: %s", partitionName);
    return false;
  }
}

void Mycila::System::deepSleep(uint64_t delayMicros) {
  LOGI(TAG, "Deep Sleep for %" PRIu64 " us!", delayMicros);

#if SOC_UART_HP_NUM > 2
  Serial2.end();
#endif
#if SOC_UART_HP_NUM > 1
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

void Mycila::System::getMemory(Memory& memory) {
  multi_heap_info_t info;
  heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
  memory.total = info.total_free_bytes + info.total_allocated_bytes;
  memory.used = info.total_allocated_bytes;
  memory.free = info.total_free_bytes;
  memory.usage = static_cast<float>(memory.used) / static_cast<float>(memory.total);
}

uint32_t Mycila::System::getChipID() {
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i += 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  return chipId;
}

String Mycila::System::getChipIDStr() {
  String espId = String(getChipID(), HEX);
  espId.toUpperCase();
  return espId;
}

const char* Mycila::System::getLastRebootReason() {
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_UNKNOWN:
      return "Unknown";
    case ESP_RST_POWERON:
      return "Power On";
    case ESP_RST_EXT:
      return "External Pin";
    case ESP_RST_SW:
      return "Software";
    case ESP_RST_PANIC:
      return "Panic";
    case ESP_RST_INT_WDT:
      return "Interrupt Watchdog";
    case ESP_RST_TASK_WDT:
      return "Task Watchdog";
    case ESP_RST_WDT:
      return "Other Watchdog";
    case ESP_RST_DEEPSLEEP:
      return "Deep Sleep Exit";
    case ESP_RST_BROWNOUT:
      return "Brownout";
    case ESP_RST_SDIO:
      return "SDIO";
#if ESP_IDF_VERSION_MAJOR >= 5
    case ESP_RST_USB:
      return "USB Peripheral";
    case ESP_RST_JTAG:
      return "JTAG";
    case ESP_RST_EFUSE:
      return "EFUSE Error";
    case ESP_RST_PWR_GLITCH:
      return "Power Glitch";
    case ESP_RST_CPU_LOCKUP:
      return "CPU Lockup";
#endif
    default:
      return "Unknown";
  }
}

#ifdef MYCILA_JSON_SUPPORT
void Mycila::System::toJson(const JsonObject& root) {
  Memory memory;
  getMemory(memory);
  root["chip_cores"] = ESP.getChipCores();
  root["chip_model"] = ESP.getChipModel();
  root["chip_revision"] = ESP.getChipRevision();
  root["cpu_freq"] = ESP.getCpuFreqMHz();
  root["heap_total"] = memory.total;
  root["heap_usage"] = memory.usage;
  root["heap_used"] = memory.used;
  root["reboot_reason"] = getLastRebootReason();
  root["reboot_count"] = _boots;
  root["uptime"] = getUptime();
}
#endif
