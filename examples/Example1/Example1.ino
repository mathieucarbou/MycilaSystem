#include <MycilaSystem.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Mycila::System::init();

  Mycila::System::Memory memory;
  Mycila::System::getMemory(memory);

  Serial.println(Mycila::System::getChipID());
  Serial.println(Mycila::System::getChipIDStr());
  Serial.println(Mycila::System::getUptime());
  Serial.println(Mycila::System::getBootCount());
  Serial.println(Mycila::System::getLastRebootReason());
  Serial.println(memory.total);
  Serial.println(memory.free);
  Serial.println(memory.used);
  Serial.println(memory.usage);

  // Mycila::System::reset();
  // Mycila::System::restart();
  // Mycila::System::deepSleep(1000000);
  // Mycila::System::restartFactory();
}

void loop() {
  delay(1000);
}
