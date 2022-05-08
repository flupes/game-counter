#include <Arduino.h>
#include <utils.h>

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_SPIFlash.h"
#include "RTClib.h"

Adafruit_FlashTransport_SPI gFlashTransport(SS1, SPI1);
Adafruit_SPIFlash gFlash(&gFlashTransport);
Adafruit_7segment gDisplay = Adafruit_7segment();
RTC_PCF8523 gRealTimeClock;

// can get the page size from the library, but use as a safety check at startup
const uint32_t kPageSize = 256;
// leave some blocks for potential settings or indexing
const uint32_t kPageStart = 32;  // 2 BLOCKS, each block is 16 pages
const uint32_t kAddrStart = kPageSize * kPageStart;

// should be 1min in ms, but can be descreased to help testing with speedup
const uint32_t kMinutePeriodSeconds = 60;

// Let say we already wasted a lot of gaming hours
const uint32_t kMinuteOffset = 4000 * 60;

const float kDefaultDriftFactor = 1.007;

const uint8_t kBuiltinLed = 13;
const uint8_t kSquareWavePin = 7;

// Only aggregate in the counter the apps active in the following slots
const uint8_t kAppCounterMask = 0b01111110;

// Command mask
const uint8_t kAppCommandMask = 0b10000001;

uint32_t gFlashIndex(0);
uint32_t gAccumulatedMinutes(0);

const DateTime kAbitraryStart(2022, 04, 27);

volatile bool gNewSecond = true;  // start to read time even before interrupt

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

void ControlDots(uint32_t now, uint32_t last, bool connected, bool colon) {
  static uint32_t lastDotsFlip = millis();
  static bool dotsToggle(false);

  uint8_t dots = 0x0;
  if (connected) {
    if (now - last < 100) {
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

bool DisplayTime(const DateTime &dt) {
  DisplayTwoDigits(0, dt.hour());
  DisplayTwoDigits(3, dt.minute());
  return true;
}

bool DisplayMinutes(uint32_t elapsedMinutes) {
  bool colon = false;
  const uint8_t hours = elapsedMinutes / 60;
  const uint8_t minutes = elapsedMinutes % 60;
  if (hours > 0) {
    gDisplay.writeDigitNum(1, hours);
    colon = true;
  } else {
    gDisplay.writeDigitAscii(1, ' ');
  }
  DisplayTwoDigits(3, minutes);
  return colon;
}

bool DisplayDaily(uint32_t elapsedMinutes) {
  bool colon = false;
  if (elapsedMinutes < 10 * 60) {
    colon = DisplayMinutes(elapsedMinutes);
    gDisplay.writeDigitAscii(0, 'd');
  } else {
    gDisplay.print("dout");
  }
  return colon;
}

bool DisplayTotal(uint32_t totalMinutes) {
  const uint32_t hours = (totalMinutes + kMinuteOffset) / 60;
  if (hours < 10000) {
    gDisplay.print(hours);
  } else {
    gDisplay.print("StoP");
  }
  return false;
}

bool DisplayActive(uint32_t elapsedMinutes, uint32_t now) {
  static uint8_t const pattern[] = {0b00000011, 0b00000110, 0b00001100,
                                    0b00011000, 0b00110000, 0b00100001};
  static uint8_t counter = 0;
  static uint32_t lastAnim = millis();
  bool colon = false;

  if (elapsedMinutes < 10 * 60) {
    colon = DisplayMinutes(elapsedMinutes);
  } else {
    gDisplay.print("bad");
  }
  if (now - lastAnim > 1000 / 6) {
    lastAnim = now;
    counter++;
    if (counter >= sizeof(pattern)) {
      counter = 0;
    }
  }
  gDisplay.writeDigitRaw(0, pattern[counter]);
  return colon;
}

enum class DisplayMode { OFF = 0, TIME, DAILY, TOTAL, ACTIVE };

bool UpdateDisplay(uint32_t elapsedMinutes, uint32_t totalMinutes, uint32_t now, const DateTime &dt,
                   bool active) {
  static DisplayMode mode = DisplayMode::TIME;
  static uint32_t lastChange = millis();
  bool colon = false;
  // gDisplay.clear();
  if (active) {
    switch (mode) {
      case DisplayMode::TIME:
        colon = DisplayTime(dt);
        if (now - lastChange > 3 * 1000) {
          lastChange = now;
          mode = DisplayMode::ACTIVE;
        }
        break;
      case DisplayMode::ACTIVE:
        colon = DisplayActive(elapsedMinutes, now);
        if (now - lastChange > 12 * 1000) {
          lastChange = now;
          mode = DisplayMode::TIME;
        }
        break;
      default:  // make sure to get in one of the two accepted states
        mode = DisplayMode::TIME;
    }
  } else {
    switch (mode) {
      case DisplayMode::TIME:
        colon = DisplayTime(dt);
        if (now - lastChange > 5 * 1000) {
          lastChange = now;
          mode = DisplayMode::TOTAL;
        }
        break;
      case DisplayMode::TOTAL:
        colon = DisplayTotal(totalMinutes);
        if (now - lastChange > 8 * 1000) {
          lastChange = now;
          mode = DisplayMode::DAILY;
        }
        break;
      case DisplayMode::DAILY:
        colon = DisplayDaily(elapsedMinutes);
        if (now - lastChange > 2 * 1000) {
          lastChange = now;
          mode = DisplayMode::TIME;
        }
        break;
      default:  // make sure to get in one of the two accepted states
        mode = DisplayMode::TIME;
    }
  }
  return colon;
}

void ControlBrightness(uint8_t hour, bool active) {
  // Control display luminosity
  if (hour > 22 || hour < 7) {
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
}

void AnnounceOnce() {
  static bool welcomed(false);
  if (!welcomed) {
    Serial.println("Game Counter Started :-)");
    Serial.print("Minutes on flash = ");
    Serial.println(gAccumulatedMinutes);
    welcomed = true;
  }
}

void AdjustDrift(uint32_t previousSeconds, uint32_t rtcSeconds, uint32_t hostSeconds) {
  static WeightedAverage<float, 5> deviations(0.0, 1.0);

  float period = (float)(hostSeconds - previousSeconds);
  float drift = (float)(rtcSeconds - previousSeconds) - period;
  float deviation_ppm = drift / period * 1000000;  //  deviation in parts per million (μs)
  Serial.print("DRIFT MEASURE: period=");
  Serial.print(period, 1);
  Serial.print("  drift=");
  Serial.print(drift, 6);
  Serial.print(" --> deviation=");
  Serial.print(deviation_ppm);
  if (period > 6.0 * 3600) {  // only use if at least for 6h offline
    float updated_deviation = deviations.AddSample(deviation_ppm, period / 3600.0);
    const float drift_unit = 4.34;  // use with offset mode PCF8523_TwoHours
    int offset = round(updated_deviation / drift_unit);
    // rtc.calibrate(PCF8523_TwoHours, offset);
    Serial.print(" ==> UPDATE with new weighted average=");
    Serial.print(updated_deviation);
    Serial.print(" (offset=");
    Serial.print(offset);
    Serial.print(")");
  }
  Serial.println();
}

void SecondsTick() { gNewSecond = true; }

void PrintTime(const DateTime &now) {
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" - ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
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

  if (!gRealTimeClock.begin()) {
    Error(7);
  }
  if (gRealTimeClock.lostPower()) {
    gRealTimeClock.adjust(kAbitraryStart);
  }
  gRealTimeClock.writeSqwPinMode(PCF8523_SquareWave1HZ);

  pinMode(kSquareWavePin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(kSquareWavePin), SecondsTick, FALLING);

  delay(1000);
  Serial.begin(9600);
  delay(1000);

  gDisplay.print(gFlashIndex - kAddrStart);
  gDisplay.writeDisplay();
  delay(3000);
  gDisplay.print(gAccumulatedMinutes);
  gDisplay.writeDisplay();
  delay(3000);
}

void loop() {
  static uint32_t lastMinute(0);
  static uint32_t dailyMinutes(0);
  static uint32_t lastTimeFromHostSeconds(0);
  static uint32_t startActive(0);
  static uint32_t lastRead(0);

  static uint8_t appStatus(0);
  static uint8_t cachedHours(100);
  static bool active(false);
  static bool connected(false);
  static bool dayRecorded(false);
  static bool offlineDriftMeasured(true);
  static DateTime rtcDateTime(kAbitraryStart);

  if (gNewSecond) {
    rtcDateTime = gRealTimeClock.now();
    gNewSecond = false;
    // PrintTime(rtcDateTime);
  }
  uint32_t nowMillis = millis();
  uint32_t nowSeconds = rtcDateTime.secondstime();

  if (Serial) {
    if (Serial.available()) {
      uint8_t msg = Serial.read();
      lastRead = nowMillis;
      connected = true;
      AnnounceOnce();
      if ((msg & kAppCommandMask) == kAppCommandMask) {
        // received command
        uint8_t code = (msg >> 1) & 0x1F;
        if ((code > 24) & (code < 31)) {
          if (cachedHours < 24) {
            const uint8_t minutes = 10 * (code - 25) + 7;
            DateTime dt(rtcDateTime.year(), rtcDateTime.month(), rtcDateTime.day(), cachedHours,
                        minutes, 0);
            gRealTimeClock.adjust(dt);
            if(!offlineDriftMeasured && lastTimeFromHostSeconds > 0) {
              AdjustDrift(lastTimeFromHostSeconds, nowSeconds, dt.secondstime());
              offlineDriftMeasured = true;
            }
            lastTimeFromHostSeconds = dt.secondstime();
          }  // command code = minutes
        } else if (code < 24) {
          // command code = hours
          cachedHours = code;
        } else if (code == 31) {
          // Get back the flash content
          PrintFlash();
        }
      } else {
        // received app status
        appStatus = msg;
        if ((appStatus & kAppCounterMask) > 0) {
          if (!active) {
            startActive = nowSeconds;
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
  if (nowMillis - lastRead > 50 * 1000) {  // comms timeout
    active = false;
    connected = false;
    offlineDriftMeasured = false;
    cachedHours = 100;
  }

  // Mark new day to flash / reset daily counter
  uint8_t rtcHours = rtcDateTime.hour();
  if (rtcHours == 3) {
    if (!dayRecorded && lastTimeFromHostSeconds > 0) {
      const uint8_t newday = 0x00;
      uint32_t len = gFlash.writeBuffer(gFlashIndex, &newday, 1);
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
  if (active) {
    uint32_t numMinutes = (nowSeconds - startActive) / kMinutePeriodSeconds;
    if (numMinutes > lastMinute) {
      uint32_t len = gFlash.writeBuffer(gFlashIndex, &appStatus, 1);
      gFlashIndex++;
      if (len != 1) {
        Error(5);
      }
      lastMinute = numMinutes;
      dailyMinutes++;
      gAccumulatedMinutes++;
    }
  }

  bool colon = UpdateDisplay(dailyMinutes, gAccumulatedMinutes, nowMillis, rtcDateTime, active);
  ControlDots(nowMillis, lastRead, connected, colon);
  ControlBrightness(rtcHours, active);

  gDisplay.writeDisplay();
  delay(20);
}
