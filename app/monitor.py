import datetime
import logging
import serial
import psutil
import time
import sys
import os

dev = serial.Serial()
dev.port = 'COM8'
dev.baudrate = 9600
dev.write_timeout = 3

RECONNECT_WAIT = 60
UPDATE_PERIOD = 10
PROCESSES_MONITORED = {
  1: 'discord.exe',
  2: 'valorant.exe',
  3: 'fortnite',
  4: 'minecraft',
  6: 'msedge',
}

myusername = os.environ.get('COMPUTERNAME')+'\\'+os.environ.get('USERNAME')
if len(PROCESSES_MONITORED) > 6:
  logging.critical('cannot monitor more than 6 processes')
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
          if p.info['name'].lower().find(v) != -1:
            status[k] = 1
  byte = 0
  for i in range(1, 8):
    if status[i] == 1:
      byte += 2**i
  return byte

def monitor():
  numtries = 0
  while (not stopped):
    try:
      numtries += 1
      logging.info('trying to open %s serial port #%s', dev.port, str(numtries))
      if dev.is_open:
        logging.info('serial port already opened')
      else:
        dev.open()
        logging.info('serial port %s successfuly opened', dev.port)
      connected = True
      updates = 0
      while (connected):
        if updates % 6 == 0:
          # send hour command
          now = datetime.datetime.now()
          hour = now.hour
          cmd = (hour << 1) | 0x81
          dev.write(bytes([cmd]))
          logging.debug('wrote hour: %s', str(cmd))
        else:
          # send app status
          status = get_process_status()
          dev.write(bytes([status]))
          logging.debug('wrote status: %s', str(status))
        updates += 1
        time.sleep(UPDATE_PERIOD)
    except serial.SerialTimeoutException as timeout:
      logging.critical(timeout)
    except serial.SerialException as error:
      logging.critical(error)
    if dev:
      dev.close()
    time.sleep(RECONNECT_WAIT)

dev.close()

if __name__ == "__main__":
  logging.basicConfig(filename='.\monitor.log', filemode='w', level=logging.DEBUG)
  if len(sys.argv) > 1:
    dev.port = sys.argv[1]
  stopped = False
  monitor()
