import serial
import datetime

dev = serial.Serial()
dev.port = 'COM8'
dev.baudrate = 115200
dev.timeout = 12

dev.open()

start = None
while True:
  msg = dev.readline()
  t = datetime.datetime.now()
  if not start:
    start = t.timestamp()
  print(t.timestamp()-start, msg.decode("utf-8").rstrip())
