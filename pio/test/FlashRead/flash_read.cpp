#include "Adafruit_SPIFlash.h"

Adafruit_FlashTransport_SPI gFlashTransport(SS1, SPI1);
Adafruit_SPIFlash gFlash(&gFlashTransport);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(100);  // wait for native usb

  Serial.println("Test reading from flash");
  
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

  uint32_t index = 0;
  uint32_t data = 0;
  uint8_t *addr = reinterpret_cast<uint8_t*>(&data);
  uint32_t start = millis();
  while (true) {
    gFlash.readBuffer(index, addr, sizeof(data));
    if (data != 0xFFFFFFFF) {
    index += sizeof(data);
    }
    else {
      break;
    }
  }
  uint32_t elapsed = millis() - start;
  Serial.print("Time to read ");
  Serial.print(index/4);
  Serial.print(" samples: ");
  Serial.print(elapsed);
  Serial.println("ms");

  if (index >= 10*sizeof(data)) {
  Serial.println("First 10 values");
  for (uint32_t i=0; i<sizeof(data)*10; i+=sizeof(data)) {
    Serial.println(gFlash.read32(i));
  }

  Serial.println("Last 10 values");
  for (uint32_t i=index-sizeof(data); i>=index-sizeof(data)*11; i-=sizeof(data)) {
    Serial.println(gFlash.read32(i));
  }
}

}

void loop() {
  // nothing to do
}
