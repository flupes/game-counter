import time
import serial
import psutil
import os

dev = serial.Serial()
dev.port = 'COM8'
dev.baudrate = 9600
dev.write_timeout = 3

# DEBUG = True
DEBUG = False

RECONNECT_WAIT = 30
UPDATE_PERIOD = 10
PROCESSES_MONITORED = {
  1: 'wsl.exe',
  2: 'firefox.exe',
  3: 'msedge.exe',
  4: 'explorer.exe',
  5: 'code.exe'
}

myusername = os.environ.get('COMPUTERNAME')+'\\'+os.environ.get('USERNAME')
if len(PROCESSES_MONITORED) > 6:
  print('cannot monitor more than 6 processes')
  exit(1)

def print_status(byte):
  str = '| '
  for k, v in PROCESSES_MONITORED.items():
    str = str + v
    if byte & 2**k > 0:
      str = str + ': RUNNING'
    else:
      str = str + ': INACTIVE'
    str = str + ' | '
  print(str)

def get_process_status():
  status = 8 * [0]
  for p in psutil.process_iter(['name', 'status', 'username']):
    if p.info['status'] == psutil.STATUS_RUNNING and p.info['username'] == myusername:
      for k, v in PROCESSES_MONITORED.items():
        if status[k] == 0:
          if p.info['name'].lower() == v:
            status[k] = 1
  byte = 0
  for i in range(1, 8):
    if status[i] == 1:
      byte += 2**i
  return byte

stopped = False
numtries = 0
while (not stopped):
  try:
    numtries += 1
    print('trying to open serial port #', numtries)
    if not DEBUG:
      if dev.is_open:
        print('serial port already opened')
      else:
        dev.open()
    connected = True
    updates = 0
    while (connected):
      updates += 1
      status = get_process_status()
      if DEBUG:
        print_status(status)
      else:
        dev.write(bytes([status]))
      print('wrote', status)
      time.sleep(UPDATE_PERIOD)
      if updates > 12:
        connected = False
        stopped = True
  except serial.SerialTimeoutException as timeout:
    print(timeout)
  except serial.SerialException as error:
    print(error)
  if dev:
    dev.close()
  time.sleep(RECONNECT_WAIT)

dev.close()
