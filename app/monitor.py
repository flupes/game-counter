import datetime
import schedule
import logging
import serial
import psutil
import time
import sys
import os

dev = serial.Serial()
dev.port = 'COM8'
dev.baudrate = 9600
dev.timeout = 1
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

def set_hours():
  global dev
  now = datetime.datetime.now()
  if now.minute > 7: # avoid edge conditions after offline periods
    cmd = (now.hour << 1) | 0x81
    dev.write(bytes([cmd]))
    logging.debug('wrote hours: %s', str(cmd))

def set_minutes():
  global dev
  minutes = datetime.datetime.now().minute
  if minutes >= 7:
    minutes -= 7
  cmd = ( ( 25 + minutes // 10 ) << 1) | 0x81
  dev.write(bytes([cmd]))
  logging.debug('wrote minutes: %s', str(cmd))

def update_status():
  global dev
  status = get_process_status()
  dev.write(bytes([status]))
  logging.debug('wrote status: %s', str(status))

def register_tasks():
  schedule.every(UPDATE_PERIOD).seconds.do(update_status)
  schedule.every(3).minutes.at(":15").do(set_hours)
  schedule.every().hour.at(":07").do(set_minutes)
  schedule.every().hour.at(":17").do(set_minutes)
  schedule.every().hour.at(":27").do(set_minutes)
  schedule.every().hour.at(":37").do(set_minutes)
  schedule.every().hour.at(":47").do(set_minutes)
  schedule.every().hour.at(":57").do(set_minutes)

if __name__ == "__main__":
  logging.basicConfig(filename='.\monitor.log', filemode='w', \
    format='%(asctime)s %(message)s', level=logging.DEBUG)
  
  if len(sys.argv) > 1:
    dev.port = sys.argv[1]

  schedule_logger = logging.getLogger('schedule')
  schedule_logger.setLevel(level=logging.WARNING)

  stopped = False
  while not stopped:
    try:
      logging.info('trying to open serial port ' + dev.port)
      dev.open()
      logging.info(dev.port + ' sucessfully opened')
      register_tasks()
      logging.info('schedule registered')
      while True:
        schedule.run_pending()
        msg = dev.readline()
        if msg:
          logging.info("RECV: "+msg.decode("utf-8").rstrip())
          time.sleep(1)
    except serial.SerialTimeoutException as timeout:
      logging.critical('write timeout ' + str(timeout))
    except serial.SerialException as error:
      logging.critical('serial error: ' + str(error))
    dev.close()
    logging.info('closed serial port ' + dev.port)
    time.sleep(RECONNECT_WAIT)
  
  dev.close()
