import time
import serial

from threading import Timer

dev = serial.Serial()
dev.port = 'COM8'
dev.baudrate = 9600
dev.write_timeout = 3

RECONNECT_WAIT = 5
UPDATE_PERIOD = 2

# From: https://stackoverflow.com/questions/12435211/threading-timer-repeat-function-every-n-seconds
class RepeatTimer(Timer):
    def run(self):
        while not self.finished.wait(self.interval):
            self.function(*self.args, **self.kwargs)

stopped = False
numtries = 0
while (not stopped):
  try:
    numtries += 1
    print('trying to open serial port #', numtries)
    dev.open()
    connected = True
    updates = 0

    while (connected):
      updates += 1
      try:
        dev.write(bytes([updates]))
        print('wrote', updates)
        time.sleep(UPDATE_PERIOD)
      except serial.SerialTimeoutException as timeout:
        print(timeout)
        connected = False
      if updates > 5:
        connected = False
        stopped = True
    dev.close()
  except serial.SerialException as error:
    print(error)
  time.sleep(RECONNECT_WAIT)

dev.close()

#un = os.environ.get('COMPUTERNAME')+'\\'+os.environ.get('USERNAME')   
#pp([(p.pid, p.info['name']) for p in psutil.process_iter(['name', 'username', 'status']) if ( p.info['username'] == un and p.info['status'] == psutil.STATUS_RUNNING)])
