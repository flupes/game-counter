#include <Arduino.h>
#include <RTCZero.h>

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_SPIFlash.h"

Adafruit_FlashTransport_SPI gFlashTransport(SS1, SPI1);
Adafruit_SPIFlash gFlash(&gFlashTransport);
Adafruit_7segment gDisplay = Adafruit_7segment();
RTCZero gRealTimeClock;

const uint32_t kPageSize = 256;
const uint32_t kPageStart = 16;
const uint32_t kAddrStart = kPageSize * kPageStart;

const uint32_t kMinutePeriodMs = 1000 * 60;  // to help testing with speedup
const uint32_t kMinuteOffset = 4000 * 60;

const uint8_t kBuiltinLed = 13;

// Only aggregate in the counter the apps active in the following slots
const uint8_t kAppCounterMask = 0b01111100;

// Command mask
const uint8_t kAppCommandMask = 0b10000001;

uint32_t gFlashIndex(0);
uint32_t gStartMs(0);
uint32_t gAccumulatedMinutes(0);
bool gAppsActive(false);

void Wheel() {
  static uint8_t const pattern[] = {0b00000011, 0b00000110, 0b00001100,
                                    0b00011000, 0b00110000, 0b00100001};
  static uint8_t counter = 0;
  gDisplay.writeDigitRaw(0, pattern[counter]);
  counter++;
  if (counter >= sizeof(pattern)) {
    counter = 0;
  }
}

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

// Returns the number of minutes recorded on flash from the current addr
// and update the addr to be the next writable slot.
uint32_t MinutesOnFlash(uint32_t &index) {
  const uint32_t maxAddr = gFlash.numPages() * gFlash.pageSize() - 1;
  uint32_t minutes = 0;
  bool byteWritable = false;
  while (!byteWritable) {
    if (index > maxAddr) {
      Error(3);
    }
    uint8_t state = gFlash.read8(index);
    if (state == 0xFF) {
      byteWritable = true;
    } else {
      index++;
      if ((state & kAppCounterMask) > 0) {
        minutes++;
      }
    }
  }
  return minutes;
}

void setup() {
  pinMode(kBuiltinLed, OUTPUT);

  gDisplay.begin(0x70);
  gDisplay.setBrightness(12);
  gDisplay.print("boot");
  gDisplay.writeDisplay();
  delay(1000);

  StartFlash();
  gFlashIndex = kAddrStart;
  gAccumulatedMinutes = MinutesOnFlash(gFlashIndex);

  gRealTimeClock.begin();
  gRealTimeClock.setDate(1, 9, 4);
  gRealTimeClock.setTime(0, 0, 0);

  delay(1000);
  Serial.begin(9600);
  delay(1000);

  gDisplay.print(gFlashIndex - kAddrStart);
  gDisplay.writeDisplay();
  delay(2000);

  gStartMs = millis();
}

void loop() {
  static uint32_t dailyMinutes(0);
  static uint32_t lastMinute(0);

  static uint32_t startActive = millis();
  static uint32_t lastDotsFlip = millis();
  static uint32_t lastDisplayFlip = millis();
  static uint32_t lastRead = millis();
  static uint32_t lastAnim = millis();

  static uint8_t state(0);
  static uint8_t lastHour(0);
  static bool dotsToggle(false);
  static bool displayToggle(false);
  static bool connected(false);
  static bool dayRecorded(false);

  uint32_t now = millis();

  if (Serial) {
    if (Serial.available()) {
      state = Serial.read();
      lastRead = now;
      connected = true;
      if ((state & kAppCommandMask) == kAppCommandMask) {  // received command
        uint8_t hours = (state >> 1) & 0x1F;
        if (hours != lastHour) {
          lastHour = hours;
          gRealTimeClock.setTime(hours, 0, 0);
        }
      } else {  // app status
        if ((state & kAppCounterMask) > 0) {
          if (!gAppsActive) {
            startActive = now;
            lastMinute = 0;
            gAppsActive = true;
          }
        } else {
          gAppsActive = false;
        }
      }
    }
  } else {
    connected = false;
  }

  // Mark days to flash
  uint8_t rtcHours = gRealTimeClock.getHours();
  if (rtcHours == 3) {
    if (!dayRecorded) {
      uint32_t len = gFlash.writeBuffer(gFlashIndex, 0x00, 1);
      gFlashIndex++;
      if (len != 1) {
        Error(4);
      }
      dailyMinutes = 0;
      dayRecorded = true;
    }
  } else {
    dayRecorded = false;
  }

  // Check app status
  if (gAppsActive) {
    uint32_t numMinutes = (now - startActive) / kMinutePeriodMs;
    if (numMinutes > lastMinute) {
      uint32_t len = gFlash.writeBuffer(gFlashIndex, &state, 1);
      gFlashIndex++;
      if (len != 1) {
        Error(5);
      }
      lastMinute = numMinutes;
      dailyMinutes++;
      gAccumulatedMinutes++;
    }
    if (now - lastAnim > 1000 / 6) {
      gDisplay.print(dailyMinutes);
      lastAnim = now;
      Wheel();
    }
    lastDisplayFlip = now;
  } else {
    if ((now - lastDisplayFlip) > 1000 * 15) {
      lastDisplayFlip = now;
      displayToggle = !displayToggle;
    }
    gDisplay.writeDigitRaw(0, 0x0);
    if (displayToggle) {
      gDisplay.print(dailyMinutes);
      gDisplay.writeDigitAscii(0, 'd');
    } else {
      gDisplay.print((gAccumulatedMinutes + kMinuteOffset) / 60);
    }
  }

  if (now - lastRead > 40000) {  // comms timeout
    gAppsActive = false;
    connected = false;
  }

  // control the dots *after* the rest of the digits since the Adafruit
  // library seems to clear them with prints.
  uint8_t dots = 0x0;
  if (connected) {
    if (now - lastRead < 100) {
      dots = 0b00010000;
    }  // dot will be turned off from the default state otherwise
  } else {
    // flips dots to show that we are waiting for data
    if ((now - lastDotsFlip) > 1000) {
      lastDotsFlip = now;
      dotsToggle = !dotsToggle;
    }
    if (dotsToggle) {
      dots &= ~(1u << 3);
      dots |= 1u << 2;
    } else {
      dots &= ~(1u << 2);
      dots |= 1u << 3;
    }
  }
  gDisplay.writeDigitRaw(2, dots);

  // Control display luminosity
  if (rtcHours > 22 || rtcHours < 8) {
    if (gAppsActive) {
      // blink, blink, time to stop gaming!
      gDisplay.setBrightness(15);
      gDisplay.blinkRate(HT16K33_BLINK_2HZ);
    } else {
      // dim display
      gDisplay.blinkRate(HT16K33_BLINK_OFF);
      gDisplay.setBrightness(2);
    }
  } else {
    gDisplay.blinkRate(HT16K33_BLINK_OFF);
    gDisplay.setBrightness(12);
  }

  gDisplay.writeDisplay();
  delay(20);
}
