import time
import serial

dev = serial.Serial()
dev.port = 'COM8'
dev.baudrate = 9600
dev.write_timeout = 3

try:
  print('opening serial port')
  dev.open()

  for i in range(1, 5):
    time.sleep(5)
    dev.write(bytes([i]))
    print('wrote', i)

except serial.SerialTimeoutException as timeout:
  print(timeout)
except serial.SerialException as error:
  print(error)

dev.close()
