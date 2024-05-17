#include <MycilaSystem.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Mycila::System.getMemory();
  Mycila::System.getBootCount();
}

void loop() {
  delay(1000);
}
