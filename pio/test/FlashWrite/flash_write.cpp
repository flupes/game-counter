#include "Adafruit_SPIFlash.h"

Adafruit_FlashTransport_SPI gFlashTransport(SS1, SPI1);
Adafruit_SPIFlash gFlash(&gFlashTransport);

const uint32_t kNumberOfWrites = 1000;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);  // wait for native usb

  Serial.println("Test writing to flash");
  
  gFlash.begin();

  Serial.print("JEDEC ID: 0x");
  Serial.println(gFlash.getJEDECID(), HEX);
  Serial.print("Flash size: ");
  Serial.print(gFlash.size() / 1024);
  Serial.println(" KB");

  uint32_t index;
  const uint8_t *addr = reinterpret_cast<uint8_t*>(&index);
  uint32_t start = millis();
  for (index = 0; index<kNumberOfWrites; index++) {
    gFlash.writeBuffer(index*sizeof(index), addr, sizeof(index));
  }
  uint32_t elapsed = millis() - start;
  Serial.print("Time to write ");
  Serial.print(kNumberOfWrites);
  Serial.print(" samples: ");
  Serial.print(elapsed);
  Serial.println("ms");
}

void loop() {
  // nothing to do
}
