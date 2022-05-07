#include <Arduino.h>

#include "Adafruit_SPIFlash.h"

Adafruit_FlashTransport_SPI gFlashTransport(SS1, SPI1);
Adafruit_SPIFlash gFlash(&gFlashTransport);

const uint32_t kPageSize = 256;

const size_t kSourceSector = 1;
const size_t kDestSector = 2;
const size_t kNumberOfSectors = 1;

const uint8_t kBuiltinLed = 13;

void Error(uint8_t num) {
  while (true) {
    if (num > 0) {
      for (uint8_t i = 0; i < num; i++) {
        delay(350);
        digitalWrite(kBuiltinLed, HIGH);
        delay(150);
        digitalWrite(kBuiltinLed, LOW);
      }
    } else {
      digitalWrite(kBuiltinLed, HIGH);
    }
    delay(3000);
  }
}

void StartFlash() {
  gFlash.begin();
  if (gFlash.pageSize() != kPageSize) {
    Serial.println("Page size mistmatch!");
    Error(1);
  }
}

void setup() {
  pinMode(kBuiltinLed, OUTPUT);

  StartFlash();

  digitalWrite(kBuiltinLed, HIGH);

  uint8_t buffer[SFLASH_SECTOR_SIZE];

  for (size_t s = 0; s < kNumberOfSectors; s++) {
    size_t readAddr = (s + kSourceSector) * SFLASH_SECTOR_SIZE;
    uint8_t t = gFlash.read8(readAddr);
    if (t == 0xFF) {
      Error(2);
    }
    size_t len = gFlash.readBuffer(readAddr, buffer, SFLASH_SECTOR_SIZE);
    if (len != SFLASH_SECTOR_SIZE) {
      Error(s+3);
    }

    size_t writeAddr = (s + kDestSector) * SFLASH_SECTOR_SIZE;
    for (size_t i = 0; i < SFLASH_SECTOR_SIZE; i++) {
      uint8_t b = buffer[i];
      if (b == 0xFF) {
        break;
      }
      b &= 0x7C;
      gFlash.writeBuffer(writeAddr++, &b, 1);
    }
    gFlash.eraseSector(s+kSourceSector);
  }

  digitalWrite(kBuiltinLed, LOW);

}

void loop() {}
