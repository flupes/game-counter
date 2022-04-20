import time
import serial

dev = serial.Serial()
dev.port = 'COM8'
dev.baudrate = 9600
dev.write_timeout = 3

RECONNECT_WAIT = 5
UPDATE_PERIOD = 2

stopped = False
numtries = 0
while (not stopped):
  try:
    numtries += 1
    if dev.is_open:
      print('serial port already opened')
    else:
      print('trying to open serial port #', numtries)
      dev.open()
    connected = True
    updates = 0

    while (connected):
      updates += 1
      dev.write(bytes([updates]))
      print('wrote', updates)
      time.sleep(UPDATE_PERIOD)
      if updates > 5:
        connected = False
        stopped = True
  except serial.SerialTimeoutException as timeout:
    print(timeout)
  except serial.SerialException as error:
    print(error)
  dev.close()
  time.sleep(RECONNECT_WAIT)

dev.close()

#un = os.environ.get('COMPUTERNAME')+'\\'+os.environ.get('USERNAME')   
#pp([(p.pid, p.info['name']) for p in psutil.process_iter(['name', 'username', 'status']) if ( p.info['username'] == un and p.info['status'] == psutil.STATUS_RUNNING)])
