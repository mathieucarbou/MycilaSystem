#include <MycilaSystem.h>

#include <vector>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Mycila::System::init();

  Serial.println(Mycila::System::getChipID());
  Serial.println(Mycila::System::getChipIDStr().c_str());
  Serial.println(Mycila::System::getUptime());
  Serial.println(Mycila::System::getBootCount());
  Serial.println(Mycila::System::getLastRebootReason());

  Mycila::System::Memory memory;
  Mycila::System::getMemory(memory);
  Serial.println(memory.total);
  Serial.println(memory.free);
  Serial.println(memory.used);
  Serial.println(memory.usage);

  Mycila::System::Coredump coredump;
  Mycila::System::readCoredump(coredump);
  Serial.println(coredump.task.c_str());
  Serial.println(coredump.reason.c_str());
  Serial.println(coredump.backtrace.c_str());

  // Mycila::System::reset();
  // Mycila::System::restart();
  // Mycila::System::deepSleep(1000000);
  // Mycila::System::restartFactory();
  delay(5000);
  std::vector<uint8_t> crash(512 * 1024);
}

void loop() {
  vTaskDelete(NULL);
}
