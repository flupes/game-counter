#include <Arduino.h>

#include "Adafruit_SPIFlash.h"

Adafruit_FlashTransport_SPI gFlashTransport(SS1, SPI1);
Adafruit_SPIFlash gFlash(&gFlashTransport);

const uint32_t kPageSize = 256;
const uint32_t kPageStart = 16;
const uint32_t kAddrStart = kPageSize * kPageStart;

const uint8_t kBuiltinLed = 13;

uint32_t gIndex = 0;

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
  if (gFlash.numPages() < 4000 + kPageStart) {
    Serial.println("Flash too small!");
    Error(2);
  }
}

uint32_t NextFlashSlot() {
  uint32_t index = 0;
  while (gFlash.read8(kAddrStart + index) != 0xFF) {
    index++;
    if (index > 4000*kPageSize) {
      Error(3);
    }
  }
  return index;
}

void setup() {
  pinMode(kBuiltinLed, OUTPUT);

  StartFlash();
  if (gFlash.read8(kAddrStart) == 0xFF) {
    gIndex = 0;
  } else {
    gIndex = NextFlashSlot();
  }

  Serial.begin(9600);
  while (!Serial) {
    digitalWrite(kBuiltinLed, HIGH);
    delay(100);
    digitalWrite(kBuiltinLed, LOW);
    delay(400);
  }
  // Serial.print("gIndex=");
  // Serial.println(gIndex);
  // Error(4);
}

void loop() {
  uint8_t state = 0;
  if (Serial.available()) {
    state = Serial.read();
    // Serial.print("received: ");
    // Serial.println((int)state);
  }
  if (state != 0) {
    uint32_t len = gFlash.writeBuffer(kAddrStart + gIndex, &state, 1);
    if (len != 1) {
      Error(5);
    }
    gIndex++;
    digitalWrite(kBuiltinLed, HIGH);
    delay(1000);
    digitalWrite(kBuiltinLed, LOW);
  }
  digitalWrite(kBuiltinLed, HIGH);
  delay(200);
  digitalWrite(kBuiltinLed, LOW);
  delay(800);
}
