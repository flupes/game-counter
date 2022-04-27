#include <Arduino.h>
#include <RTCZero.h>

RTCZero gRealTimeClock;

void setup() {
  gRealTimeClock.begin();
  gRealTimeClock.setDate(0, 0, 0);
  gRealTimeClock.setTime(0, 0, 0);

  Serial.begin(115200);
  while (!Serial);
}

void loop() {
  static uint32_t last = millis();
  static uint32_t counter = 0;
  uint32_t now = millis();
  if ((now - last) > 10 * 1000) {
    counter++;
    uint32_t s = gRealTimeClock.getSeconds();
    uint32_t m = gRealTimeClock.getMinutes();
    uint32_t h = gRealTimeClock.getHours();
    uint32_t d = gRealTimeClock.getDay();
    Serial.print(counter);
    Serial.print(' ');
    Serial.print(now);
    Serial.print(' ');
    Serial.println(h*3600ul+m*60ul+s+(d-1)*24ul*3600ul);
    last = now;
  }
}
