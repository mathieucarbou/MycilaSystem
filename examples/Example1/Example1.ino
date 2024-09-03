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
}

void loop() {
  delay(1000);
}
