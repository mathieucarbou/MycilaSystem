// SPDX-License-Identifier: MIT
/*
 * Copyright (C) Mathieu Carbou
 */
#include <MycilaSystem.h>

#include <LittleFS.h>
#include <Preferences.h>

#include <esp_core_dump.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BLUEDROID_ENABLED)
  #include <esp_bt.h>
  #include <esp_bt_main.h>
#endif

#ifndef SOC_UART_HP_NUM
  #define SOC_UART_HP_NUM SOC_UART_NUM
#endif

#define TAG "SYSTEM"

#define KEY_BOOTS  "boots"
#define KEY_RESETS "resets"

static constexpr char hexUp[] = "0123456789ABCDEF";
static constexpr char hexLow[] = "0123456789abcdef";

Ticker Mycila::System::_delayedTask;
uint32_t Mycila::System::_boots = 0;
uint32_t Mycila::System::_chipID = 0;

void Mycila::System::init(bool initFS, const char* fsPartitionName, const char* basePath, uint8_t maxOpenFiles) {
  ESP_LOGI(TAG, "Initializing NVM...");
  ESP_ERROR_CHECK(nvs_flash_init());

  Preferences prefs;
  if (prefs.begin(TAG, false)) {
    _boots = (prefs.isKey(KEY_BOOTS) ? prefs.getULong(KEY_BOOTS, 0) : 0) + 1;
    prefs.putULong(KEY_BOOTS, _boots);
    ESP_LOGI(TAG, "Booted %" PRIu32 " times", _boots);
    prefs.end();
  }

  if (initFS) {
    ESP_LOGI(TAG, "Initializing File System...");
    if (LittleFS.begin(false, basePath, maxOpenFiles, fsPartitionName))
      ESP_LOGD(TAG, "File System initialized");
    else {
      ESP_LOGW(TAG, "File System initialization failed. Trying to format...");
      if (LittleFS.begin(true, basePath, maxOpenFiles, fsPartitionName)) {
        ESP_LOGW(TAG, "Successfully formatted and initialized. Rebooting...");
        esp_restart();
      } else {
        ESP_LOGE(TAG, "Failed to format and initialize File System!");
      }
    }
  }
}

void Mycila::System::reset(uint32_t delayMillisBeforeRestartMillis) {
  ESP_LOGW(TAG, "Reset!");
  ESP_ERROR_CHECK(nvs_flash_erase());
  ESP_ERROR_CHECK(nvs_flash_init());
  restart(delayMillisBeforeRestartMillis);
}

void Mycila::System::restart(uint32_t delayMillisBeforeRestartMillis) {
  _delayedTask.detach();
  if (delayMillisBeforeRestartMillis == 0) {
    ESP_LOGW(TAG, "Restart!");
    esp_restart();
  } else {
    ESP_LOGW(TAG, "Restart in %" PRIu32 " ms...", delayMillisBeforeRestartMillis);
    _delayedTask.once_ms(delayMillisBeforeRestartMillis, esp_restart);
  }
}

bool Mycila::System::restartFactory(const char* partitionName, uint32_t delayMillisBeforeRestartMillis) {
  const esp_partition_t* partition = esp_partition_find_first(esp_partition_type_t::ESP_PARTITION_TYPE_APP, esp_partition_subtype_t::ESP_PARTITION_SUBTYPE_APP_FACTORY, partitionName);
  if (partition) {
    ESP_LOGW(TAG, "Set boot partition to %s", partitionName);
    ESP_ERROR_CHECK(esp_ota_set_boot_partition(partition));
    restart(delayMillisBeforeRestartMillis);
    return true;
  } else {
    ESP_LOGE(TAG, "Partition not found: %s", partitionName);
    return false;
  }
}

void Mycila::System::deepSleep(uint64_t delayMicros) {
  ESP_LOGI(TAG, "Deep Sleep for %" PRIu64 " us!", delayMicros);

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
  multi_heap_info_t* info = new multi_heap_info_t();
  heap_caps_get_info(info, MALLOC_CAP_INTERNAL);
  memory.total = info->total_free_bytes + info->total_allocated_bytes;
  memory.used = info->total_allocated_bytes;
  memory.free = info->total_free_bytes;
  memory.minimumFree = info->minimum_free_bytes;
  memory.usage = static_cast<float>(memory.used) / static_cast<float>(memory.total) * 100.0f;
  delete info;
}

uint32_t Mycila::System::getChipID() {
  if (!_chipID) {
    for (int i = 0; i < 17; i += 8) {
      _chipID |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
  }
  return _chipID;
}

std::string Mycila::System::getChipIDStr() {
  const uint32_t chipID = getChipID();
  if (chipID == 0) {
    return "0";
  }
  std::string result;
  result.reserve(8);
  bool started = false;
  for (int i = 28; i >= 0; i -= 4) {
    char c = hexUp[(chipID >> i) & 0x0F];
    if (!started && c != '0') {
      started = true;
    }
    if (started) {
      result += c;
    }
  }
  return result.empty() ? "0" : std::move(result);
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

bool Mycila::System::readCoredump(Coredump& coredump) {
  esp_core_dump_summary_t* summary = new esp_core_dump_summary_t();
  if (esp_core_dump_get_summary(summary) == ESP_OK) {
    coredump.task = summary->exc_task;

    std::stringstream ss;
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)
    for (uint32_t i = 0, count = std::min(static_cast<uint32_t>(CONFIG_ESP_COREDUMP_SUMMARY_STACKDUMP_SIZE), summary->exc_bt_info.dump_size); i < count; i++)
      ss << "0x" << std::hex << summary->exc_bt_info.stackdump[i] << " ";
#else
    for (uint32_t i = 0, count = std::min(static_cast<uint32_t>(16), summary->exc_bt_info.depth); i < count; i++)
      ss << "0x" << std::hex << summary->exc_bt_info.bt[i] << " ";
#endif
    coredump.backtrace = std::move(ss.str());

#if ESP_IDF_VERSION_MAJOR >= 5
    char* reason = new char[256];
    if (esp_core_dump_get_panic_reason(reason, 256) == ESP_OK) {
      coredump.reason = reason;
    } else {
      coredump.reason = "Unknown";
    }
    delete[] reason;
#endif

    return true;
  } else {
    return false;
  }
}

#ifdef MYCILA_JSON_SUPPORT
void Mycila::System::toJson(const JsonObject& root) {
  root["chip_cores"] = ESP.getChipCores();
  root["chip_model"] = ESP.getChipModel();
  root["chip_revision"] = ESP.getChipRevision();
  root["cpu_freq"] = ESP.getCpuFreqMHz();

  Coredump* coredump = new Coredump();
  if (readCoredump(*coredump)) {
    root["coredump"]["task"] = coredump->task;
    root["coredump"]["reason"] = coredump->reason;
    root["coredump"]["backtrace"] = coredump->backtrace;
  }
  delete coredump;

  Memory* memory = new Memory();
  getMemory(*memory);
  root["heap_total"] = memory->total;
  root["heap_usage"] = memory->usage;
  root["heap_used"] = memory->used;
  root["heap_free"] = memory->free;
  root["heap_free_min"] = memory->minimumFree;
  delete memory;

  root["reboot_reason"] = getLastRebootReason();
  root["reboot_count"] = _boots;
  root["uptime"] = getUptime();
}
#endif
