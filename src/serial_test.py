import serial
import time

ser = serial.Serial('/dev/ttyUSB0', 9600, timeout=1) # TROCAR O SERIAL PELO QUE A ESP TÀ SENDO MONTADA

time.sleep(2)

ser.write(b'T\n')

data = ser.readline().decode().strip()
print("Received:", data)

ser.close()