#include <Arduino.h>

#include "Adafruit_SPIFlash.h"

Adafruit_FlashTransport_SPI gFlashTransport(SS1, SPI1);
Adafruit_SPIFlash gFlash(&gFlashTransport);

const uint32_t kPageSize = 256;
const uint32_t kPageStart = 16;
const uint32_t kAddrStart = kPageSize * kPageStart;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);  // wait for native usb

  gFlash.begin();
  uint32_t index = 0;
  bool hasData = true;
  while (hasData) {
    uint8_t value = gFlash.read8(kAddrStart+index);
    if (value != 0xFF) {
      Serial.println((int)value);
      index++;
    } else {
      hasData = false;
    }
  }
  Serial.print("count = ");
  Serial.println(index);
}

void loop() {}