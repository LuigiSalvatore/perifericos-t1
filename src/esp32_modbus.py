from PyQt5.QtCore import *
from PyQt5.QtGui import QPixmap
from PyQt5 import QtWidgets, QtCore
from PyQt5 import uic

app = QtWidgets.QApplication([])
window = uic.loadUi("src/esp32_modbus.ui")

def start_com():
    window.progressbar_serial_com.setValue(100)
    print("start")

def stop_com():
    window.progressbar_serial_com.setValue(50)
    print("stop")

def read_temp():
    print(f"temp read {window.lcd_temp1.value()}")

def read_humid():
    print(f"humid read {window.progressbar_humid1.value()}")

def read_temp_humid():
    print(f"temp humid read {window.lcd_temp2.value()} {window.progressbar_humid2.value()}")

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