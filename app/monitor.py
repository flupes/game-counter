import time
import serial

from threading import Timer

dev = serial.Serial()
dev.port = 'COM8'
dev.baudrate = 9600
dev.write_timeout = 3

class RepeatTimer(Timer):
    def run(self):
        while not self.finished.wait(self.interval):
            self.function(*self.args, **self.kwargs)

def update(dev):
  update.counter += 1
  try:
    dev.write(bytes([update.counter]))
    print('wrote', update.counter)
  except serial.SerialTimeoutException as timeout:
    print(timeout)

update.counter = 0

def connect(dev):
  connect.numtries += 1
  print('trying to open serial port #', connect.numtries)
  updater = None
  try:
    dev.open()
    updater = RepeatTimer(2, update(dev))
    updater.start()
  except serial.SerialException as error:
    print(error)
    dev.close()
    if updater:
      updater.cancel()

connect.numtries = 0

opener = RepeatTimer(5, connect(dev))
opener.start()

dev.close()

#un = os.environ.get('COMPUTERNAME')+'\\'+os.environ.get('USERNAME')   
#pp([(p.pid, p.info['name']) for p in psutil.process_iter(['name', 'username', 'status']) if ( p.info['username'] == un and p.info['status'] == psutil.STATUS_RUNNING)])
