from PyQt5.QtCore import *
from PyQt5.QtGui import QPixmap
from PyQt5 import QtWidgets, QtCore
from PyQt5 import uic
import serial
import time

import sys
import glob


def start_com():
    # devices = find_serial_ports()

    # ser = serial.Serial(devices[0], 9600, timeout=1)

    window.pushbutton_start_com.setEnabled(False)
    window.progressbar_serial_com.setValue(30)

    # Inserir conexão à porta

    window.progressbar_serial_com.setValue(60)

    data = ser.readline().decode().strip() # Clear buffer? só dar read?
    print(data) # debug

    # Inserir algo a mais?

    window.progressbar_serial_com.setValue(100)
    print("start")

    window.pushbutton_stop_com.setEnabled(True)
    window.pushbutton_read_temp.setEnabled(True)
    window.pushbutton_read_humid.setEnabled(True)
    window.pushbutton_read_temp_humid.setEnabled(True)
    window.pushbutton_custom.setEnabled(True)

def stop_com():
    window.pushbutton_stop_com.setEnabled(False)

    window.progressbar_serial_com.setValue(60)

    ser = None

    window.progressbar_serial_com.setValue(0)
    print("stop")

    window.pushbutton_start_com.setEnabled(True)
    window.pushbutton_read_temp.setEnabled(False)
    window.pushbutton_read_humid.setEnabled(False)
    window.pushbutton_read_temp_humid.setEnabled(False)
    window.pushbutton_custom.setEnabled(False)

def read_temp():
    ser.write(b'T\n')
    data = ser.readline().decode().strip()
    print(data) # debug
    temp = float(data.split(":")[1])
    window.lcd_temp1.display(temp)

def read_humid():
    ser.write(b'H\n')
    data = ser.readline().decode().strip()
    print(data) # debug
    hum = int(data.split(":")[1])
    window.progressbar_humid1.setValue(hum)

def read_temp_humid():
    ser.write(b'th\n')
    data = ser.readline().decode().strip()
    print(data) # debug
    t, h = data.split(":")[1].split(",")
    window.lcd_temp2.display(float(t))
    window.progressbar_humid2.setValue(int(h))

def custom_command():
    print(f"custom command: {window.lineedit_custom_command.text()}")

def find_serial_ports():    
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

if __name__ == '__main__':
    device = '/dev/ttyUSB0' # TROCAR O SERIAL PELO QUE A ESP TÀ SENDO MONTADA
    ser = serial.Serial(device, 9600, timeout=1)

    app = QtWidgets.QApplication([])
    window = uic.loadUi("src/esp32_modbus.ui")

    # Desabilita todos botões com excessão do início de conexão
    window.pushbutton_stop_com.setEnabled(False)
    window.pushbutton_read_temp.setEnabled(False)
    window.pushbutton_read_humid.setEnabled(False)
    window.pushbutton_read_temp_humid.setEnabled(False)
    window.pushbutton_custom.setEnabled(False)

    window.pushbutton_start_com.clicked.connect(start_com)
    window.pushbutton_stop_com.clicked.connect(stop_com)
    window.pushbutton_read_temp.clicked.connect(read_temp)
    window.pushbutton_read_humid.clicked.connect(read_humid)
    window.pushbutton_read_temp_humid.clicked.connect(read_temp_humid)
    window.pushbutton_custom.clicked.connect(custom_command)

    window.show()
    app.exec_()