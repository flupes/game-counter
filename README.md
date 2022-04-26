# game-counter

Provides awareness of total accumulated game time with seven segment display counter.

This is a physical alternative to software app tracking screen time. Rather than
collecting a plethora of data and presenting elaborated statistics, this device
displays a single number of the accumulated hours spent on gaming and a “daily”
minute counter while in game.

## Concept

A simple desktop app sends if some apps are active via the USB/Serial port to an
Arduino based microcontroller which accumulate the time and manage a 7 segment
display.

The app collects which processes are active periodically (10s TBR) and writes a
single byte on the serial port. This byte contains “slots” for 6 different apps
(1 per bit) we are interested to monitor.

| Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|:-----:|-------|-------|-------|-------|-------|-------|:-----:|
|   0   | App 6 | App 5 | App 4 | App 3 | App 2 | App 1 |   0   |

  - App bit == 1 —> app is currently active
  - Status = 0 —> No monitored apps monitored is active
  - Bit 0 and 7 should always be low : this byte is written directly to flash,
    and we need to differentiate from writable bytes (0xFF).

This is the most trivial communication protocol: a single command byte :-) 

The host app also can also send "commands" to the microcontroller. Typically, it
sends the hour of the day to allow write on flash that a days as passed or make
smart decision regarding the display. In addition, sending a minute code every 10 minutes allows to synchronize the on-board clock accurately.

| Bit 7 | Bit 6 |  Bit 5  |  Bit 4  |  Bit 3  |  Bit 2  |  Bit 1  | Bit 0 |
|:-----:|:-----:|:-------:|:-------:|:-------:|:-------:|:-------:|:-----:|
|   1   |   0   | H bit 4 | H bit 3 | H bit 2 | H bit 1 | H bit 0 |   1   |
|   1   |   0   | M bit 4 | M bit 3 | M bit 2 | M bit 1 | M bit 0 |   1   |

  - H[5] < 24: Hour of the day with a 24h format, leave minutes + seconds
  - M[5] > 24: Minute of the hour, reset seconds

| M Code  | 25 | 26 | 27 | 28 | 29 | 30 |
|---------|----|----|----|----|----|----|
| Minutes | 7  | 17 | 27 | 37 | 47 | 57 |
## Storing counts

The system should not rely on some persistence mechanism of the desktop app to
maintain a time count. The microcontroller stores the elapsed time on local
flash. The chosen board (Itsy Betsy M0) has 2MB of SPI FLASH, and this large
capacity allows to select and extremely simple scheme to store time elapsed.

If the received status byte is not null, the firmware simply writes it after the
last non 0xFF byte on flash. Knowing the period of this write (1min, TBR), the
firmware simply accumulates the all the valid bytes on flash at startup to
retrieve the number of minutes elapsed.

In addition,the firmware writes a byte==0x00 on flash after a 6h (TBR) period
off non activity is recorded. This potentially allow to separate “virtual” days
of gaming.

This extremely simple scheme allows to totalise time elapsed with a minute
resolution for up to 6 different applications. The drawback is the time time at
firmware boot to retrieve the a last count.

For example, if the device recorded 10’000 hours (this is way too much time
spent gaming for a few years and the max displayable with 4 digits), it would
need 600’000 bytes (=minutes). The measured read time per byte is 68us, meaning
40.8s !

However, the device is not supposed to reboot more than once per day, and this
boot time is still lower than the granularity of the counter (1min). An
optimization would be to use a few sector to store and index of the last byte,
but this seems un-necessary.

## Computer host app

The “desktop” app is simple Python program, using `psutil` to collect the
information about running games, and `serial` to send the status to the
microcontroller.

The app will be run as a Windows service in order to always be available.




