import serial
import sys

dev = serial.Serial()
dev.port = 'COM8'
dev.baudrate = 9600
dev.write_timeout = 3

def write_byte(b):
  dev.open()
  dev.write(bytes([b]))
  dev.close()

if __name__ == "__main__":
  if len(sys.argv) != 3:
    print(sys.argv[0], "STATUS=0|CMD=1 code", )
    exit(-1)
  try:
    msgtype = int(sys.argv[1])
    code = int(sys.argv[2])
  except ValueError:
    print("invalid arguments!")
  if msgtype == 0:
    if code >=0 and code <= 2**6:
      write_byte(code << 1)
    else:
      print("invalid status")
      exit(-2)
  elif msgtype == 1:
    if code >= 0 and code < 32:
      write_byte((code << 1) | 0x81)
    else:
      print("invalid time code")
      exit(-3)
  else:
    print("invalid type of message")
    exit(-4)
