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
uint32_t gDailyMinutes(0);

enum class DisplayMode { OFF = 0, TIME, DAILY, TOTAL, ACTIVE };
DisplayMode gDisplayMode = DisplayMode::TIME;

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

void PrintFlash(uint32_t index = kAddrStart) {
  bool byteWritable = false;
  uint32_t counter = 0;
  delay(9000);
  while (!byteWritable) {
    uint8_t state = gFlash.read8(index);
    if (state == 0xFF) {
      byteWritable = true;
    } else {
      Serial.print(counter);
      Serial.print(", ");
      Serial.print(index);
      Serial.print(", ");
      Serial.println((int)(state));
      index++;
      counter++;
    }
  }
}

void DisplayTwoDigits(uint8_t offset, uint8_t number) {
  const uint8_t truncated = number - 100 * (number / 100);  // kill extra digits
  const uint8_t digit1 = truncated / 10;
  const uint8_t digit2 = truncated % 10;
  gDisplay.writeDigitNum(offset, digit1);
  gDisplay.writeDigitNum(offset + 1, digit2);
}

void ControlDots(uint32_t now, uint32_t lastRead, bool connected, bool colon) {
  static uint32_t lastDotsFlip = millis();
  static bool dotsToggle(false);

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
  if (colon) {
    dots |= 0x02;
  }
  gDisplay.writeDigitRaw(2, dots);
}

void DisplayTime() {
  const uint8_t hours = gRealTimeClock.getHours();
  const uint8_t minutes = gRealTimeClock.getMinutes();
  DisplayTwoDigits(0, hours);
  DisplayTwoDigits(3, minutes);
}

void DisplayDaily() {
  if (gDailyMinutes < 1000) {
    gDisplay.print(gDailyMinutes);
    gDisplay.writeDigitAscii(0, 'd');
  } else {
    gDisplay.print("dout");
  }
}

void DisplayTotal() {
  const uint32_t hours = (gAccumulatedMinutes + kMinuteOffset) / 60;
  if (hours < 10000) {
    gDisplay.print(hours);
  } else {
    gDisplay.print("StoP");
  }
}
void DisplayActive(uint32_t now) {
  static uint8_t const pattern[] = {0b00000011, 0b00000110, 0b00001100,
                                    0b00011000, 0b00110000, 0b00100001};
  static uint8_t counter = 0;
  static uint32_t lastAnim = millis();

  if (gDailyMinutes < 900) {
    gDisplay.print(gDailyMinutes);
    if (now - lastAnim > 1000 / 6) {
      lastAnim = now;
      counter++;
      if (counter >= sizeof(pattern)) {
        counter = 0;
      }
    }
  } else {
    gDisplay.print("bad");
  }
  gDisplay.writeDigitRaw(0, pattern[counter]);
}

void UpdateDisplay(uint32_t now, bool active) {
  static uint32_t lastChange = millis();

  if (active) {
    switch (gDisplayMode) {
      case DisplayMode::TIME:
        DisplayTime();
        if (now - lastChange > 3 * 1000) {
          lastChange = now;
          gDisplayMode = DisplayMode::ACTIVE;
        }
        break;
      case DisplayMode::ACTIVE:
        DisplayActive(now);
        if (now - lastChange > 12 * 1000) {
          lastChange = now;
          gDisplayMode = DisplayMode::TIME;
        }
        break;
      default:  // make sure to get in one of the two accepted states
        gDisplayMode = DisplayMode::TIME;
    }
  } else {
    switch (gDisplayMode) {
      case DisplayMode::TIME:
        DisplayTime();
        if (now - lastChange > 8 * 1000) {
          lastChange = now;
          gDisplayMode = DisplayMode::TOTAL;
        }
        break;
      case DisplayMode::TOTAL:
        DisplayTotal();
        if (now - lastChange > 5 * 1000) {
          lastChange = now;
          gDisplayMode = DisplayMode::DAILY;
        }
        break;
      case DisplayMode::DAILY:
        DisplayDaily();
        if (now - lastChange > 2 * 1000) {
          lastChange = now;
          gDisplayMode = DisplayMode::TIME;
        }
        break;
      default:  // make sure to get in one of the two accepted states
        gDisplayMode = DisplayMode::TIME;
    }
  }
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
  delay(3000);
  gDisplay.print(gAccumulatedMinutes);
  gDisplay.writeDisplay();
  delay(3000);

  gStartMs = millis();
}

void loop() {
  static uint32_t lastMinute(0);

  static uint32_t startActive = millis();
  static uint32_t lastRead = millis();

  static uint8_t appStatus(0);
  static bool active(false);
  static bool connected(false);
  static bool dayRecorded(false);

  uint32_t now = millis();

  if (Serial) {
    if (Serial.available()) {
      uint8_t msg = Serial.read();
      lastRead = now;
      connected = true;
      if ((msg & kAppCommandMask) == kAppCommandMask) {  // received command
        uint8_t code = (msg >> 1) & 0x1F;
        if ((code > 24) & (code < 31)) {
          gRealTimeClock.setSeconds(0);
          gRealTimeClock.setMinutes(10 * (code - 25) + 7);
        } else if (code < 24) {
          gRealTimeClock.setHours(code);
        } else if (code == 31) {
          // PrintFlash(gFlashIndex-100ul);
          PrintFlash();
        }
      } else {  // app status
        appStatus = msg;
        if ((appStatus & kAppCounterMask) > 0) {
          if (!active) {
            startActive = now;
            lastMinute = 0;
            active = true;
          }
        } else {
          active = false;
        }
      }
    }
  } else {
    connected = false;
  }
  if (now - lastRead > 50000) {  // comms timeout
    active = false;
    connected = false;
  }

  // Mark new day to flash / reset daily counter
  uint8_t rtcHours = gRealTimeClock.getHours();
  if (rtcHours == 3) {
    if (!dayRecorded) {
      const uint8_t newday = 0x00;
      uint32_t len = gFlash.writeBuffer(gFlashIndex, &newday, 1);
      gFlashIndex++;
      if (len != 1) {
        Error(4);
      }
      gDailyMinutes = 0;
      dayRecorded = true;
    }
  } else {
    dayRecorded = false;
  }

  // Check app status
  if (active) {
    uint32_t numMinutes = (now - startActive) / kMinutePeriodMs;
    if (numMinutes > lastMinute) {
      uint32_t len = gFlash.writeBuffer(gFlashIndex, &appStatus, 1);
      gFlashIndex++;
      if (len != 1) {
        Error(5);
      }
      lastMinute = numMinutes;
      gDailyMinutes++;
      gAccumulatedMinutes++;
    }
  }

  UpdateDisplay(now, active);
  ControlDots(now, lastRead, connected, gDisplayMode == DisplayMode::TIME);

  // Control display luminosity
  if (rtcHours > 22 || rtcHours < 8) {
    if (active) {
      // blink, blink, time to stop gaming!
      gDisplay.setBrightness(15);
      gDisplay.blinkRate(HT16K33_BLINK_1HZ);
    } else {
      // dim display
      gDisplay.blinkRate(HT16K33_BLINK_OFF);
      gDisplay.setBrightness(1);
    }
  } else {
    gDisplay.blinkRate(HT16K33_BLINK_OFF);
    gDisplay.setBrightness(10);
  }

  gDisplay.writeDisplay();
  delay(20);
}
