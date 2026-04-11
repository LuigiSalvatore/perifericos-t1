from PyQt5.QtCore import *
from PyQt5.QtGui import QPixmap
from PyQt5 import QtWidgets, QtCore
from PyQt5 import uic
import serial

ser = None
device = '/dev/ttyUSB0'  # TROCAR O SERIAL PELO QUE A ESP TÀ SENDO MONTADA
app = QtWidgets.QApplication([])
window = uic.loadUi("src/esp32_modbus.ui")

def start_com():
    global ser
    baud = int(window.combo_baudrate.currentText())
    ser = serial.Serial(device, baud, timeout=1)
    window.progressbar_serial_com.setValue(100)

def stop_com():
    global ser
    if ser and ser.is_open:
        ser.close()
    window.progressbar_serial_com.setValue(0)

def read_temp():
    ser.write(b'T\n')
    data = ser.readline().decode().strip()
    temp = float(data.split(":")[1])
    window.lcd_temp1.display(temp)

def read_humid():
    ser.write(b'H\n')
    data = ser.readline().decode().strip()
    hum = int(data.split(":")[1])
    window.progressbar_humid1.setValue(hum)

def read_temp_humid():
    ser.write(b'TH\n')
    data = ser.readline().decode().strip()
    t, h = data.split(":")[1].split(",")
    window.lcd_temp2.display(float(t))
    window.progressbar_humid2.setValue(int(h))
    
def custom_command():
    print(f"custom command: {window.lineedit_custom_command.text()}")

window.pushbutton_start_com.clicked.connect(start_com)
window.pushbutton_stop_com.clicked.connect(stop_com)
window.pushbutton_read_temp.clicked.connect(read_temp)
window.pushbutton_read_humid.clicked.connect(read_humid)
window.pushbutton_read_temp_humid.clicked.connect(read_temp_humid)
window.pushbutton_custom.clicked.connect(custom_command)

window.show()
app.exec_()