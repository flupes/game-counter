#include <Arduino.h>

#include "Adafruit_SPIFlash.h"

Adafruit_FlashTransport_SPI gFlashTransport(SS1, SPI1);
Adafruit_SPIFlash gFlash(&gFlashTransport);

const size_t kSourceSector = 2;
const size_t kNumberOfSectors = 1;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);  // wait for native usb

  gFlash.begin();

  Serial.print("JEDEC ID: 0x");
  Serial.println(gFlash.getJEDECID(), HEX);
  Serial.print("Flash size: ");
  Serial.print(gFlash.size() / 1024);
  Serial.println(" KB");
  Serial.print("Number of pages: ");
  Serial.println(gFlash.numPages());
  Serial.print("Page size: ");
  Serial.println(gFlash.pageSize());
  Serial.print("SECTOR_SIZE: ");
  Serial.println(SFLASH_SECTOR_SIZE);
  Serial.print("BLOCK_SIZE: ");
  Serial.println(SFLASH_BLOCK_SIZE);

  uint8_t buffer[SFLASH_SECTOR_SIZE];
  for (size_t s = 0; s < kNumberOfSectors; s++) {
    size_t readAddr = (s + kSourceSector) * SFLASH_SECTOR_SIZE;

    Serial.println();
    Serial.println("====================");
    Serial.print("Content of sector: ");
    Serial.println(s);

    size_t len = gFlash.readBuffer(readAddr, buffer, SFLASH_SECTOR_SIZE);
    if (len != SFLASH_SECTOR_SIZE) {
      Serial.println("#### READ ERROR !");
      while (1)
        ;
    }
    for (size_t i = 0; i < SFLASH_SECTOR_SIZE; i++) {
      uint8_t b = buffer[i];
      if (b != 0xFF) {
        Serial.print(i);
        Serial.print(", ");
        Serial.print(readAddr++);
        Serial.print(", ");
        Serial.println((int)b);
      }
    }
  }
}

void loop() {}
