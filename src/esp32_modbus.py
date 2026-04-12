from PyQt5.QtCore import *
from PyQt5.QtGui import QPixmap
from PyQt5 import QtWidgets, QtCore
from PyQt5 import uic
import serial.tools.list_ports
import serial
import time

DEBUG = True
RETRIES = 5

def start_com():
    window.pushbutton_start_com.setEnabled(False)
    window.progressbar_serial_com.setValue(30)

    # Abre porta caso ainda não esteja aberta
    if not ser.isOpen():
        ser.open()

    window.progressbar_serial_com.setValue(60)

    # Limpa o buffer
    while ser.read():
        pass

    window.progressbar_serial_com.setValue(100)

    if DEBUG:
        print("Application started")

    # Habilita todos outros botões
    window.pushbutton_stop_com.setEnabled(True)
    window.pushbutton_read_temp.setEnabled(True)
    window.pushbutton_read_humid.setEnabled(True)
    window.pushbutton_read_temp_humid.setEnabled(True)
    window.pushbutton_custom.setEnabled(True)

def stop_com():
    window.pushbutton_stop_com.setEnabled(False)

    window.progressbar_serial_com.setValue(60)

    ser.close()

    window.progressbar_serial_com.setValue(30)

    # Caso necessário esperar parar de ler
    if ser.isOpen():
        while ser.read():
            pass

    window.progressbar_serial_com.setValue(0)
    
    if DEBUG:
        print("Application stopped")

    # Desabilita todos botões com excessão do início de conexão
    window.pushbutton_start_com.setEnabled(True)
    window.pushbutton_read_temp.setEnabled(False)
    window.pushbutton_read_humid.setEnabled(False)
    window.pushbutton_read_temp_humid.setEnabled(False)
    window.pushbutton_custom.setEnabled(False)

def read_temp():
    window.pushbutton_read_temp.setEnabled(False)
    window.label_com_error1.setText("")
    b_data_receive = False

    data = ""
    for i in range(RETRIES + 1):
        ser.write(b'T\n')
        data = ser.readline().decode().strip()

        if (len(data) > 0):
            b_data_receive = True
            break

    if (not b_data_receive):
        window.label_com_error1.setText("Communication Error")
        window.pushbutton_read_temp.setEnabled(True)
        return

    if DEBUG:
        print("Data received: " + data)
    
    temp = float(data.split(":")[1])
    window.lcd_temp1.display(temp)
    window.pushbutton_read_temp.setEnabled(True)

def read_humid():
    window.pushbutton_read_humid.setEnabled(False)
    window.label_com_error2.setText("")
    b_data_receive = False

    data = ""
    for i in range(RETRIES + 1):
        ser.write(b'H\n')
        data = ser.readline().decode().strip()

        if (len(data) > 0):
            b_data_receive = True
            break

    if (not b_data_receive):
        window.label_com_error2.setText("Communication Error")
        window.pushbutton_read_humid.setEnabled(True)
        return
    
    if DEBUG:
        print("Data received: " + data)

    hum = int(data.split(":")[1])
    window.progressbar_humid1.setValue(hum)
    window.pushbutton_read_humid.setEnabled(True)

def read_temp_humid():
    window.pushbutton_read_temp_humid.setEnabled(False)
    window.label_com_error3.setText("")
    b_data_receive = False

    data = ""
    for i in range(RETRIES + 1):
        ser.write(b'th\n')
        data = ser.readline().decode().strip()

        if (len(data) > 0):
            b_data_receive = True
            break
        ser.write(b'th\n')

    if (not b_data_receive):
        window.label_com_error3.setText("Communication Error")
        window.pushbutton_read_temp_humid.setEnabled(True)
        return
    
    if DEBUG:
        print("Data received: " + data)

    t, h = data.split(":")[1].split(",")
    window.lcd_temp2.display(float(t))
    window.progressbar_humid2.setValue(int(h))
    window.pushbutton_read_temp_humid.setEnabled(True)

def custom_command():
    window.label_com_error4.setText("")
    # TODO: Fazer verificação se é hexadecimal
    print(f"custom command: {window.lineedit_custom_command.text()}")

def find_device():
    window.pushbutton_find_device.setEnabled(False)
    window.pushbutton_start_com.setEnabled(False)

    global ser

    device = None 
    for port in serial.tools.list_ports.comports():
        # print(f"{port.device}: {port.description}")
        if ("UART" in port.description):
            device = port.device

            if DEBUG:
                print("Device " + port.device + " found")
            break
    
    if (device == None):
        window.pushbutton_find_device.setEnabled(True)
        window.label_device.setText("Device not found")
    else:
        window.pushbutton_start_com.setEnabled(True)
        window.label_device.setText("Device " + device + " found")
        ser = serial.Serial(device, 9600, timeout=1)
        print(ser)
        

if __name__ == '__main__':
    # Descobre portas que contenham UART na descrição

    app = QtWidgets.QApplication([])
    window = uic.loadUi("src/esp32_modbus.ui")

    ser = None

    find_device()

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
    window.pushbutton_find_device.clicked.connect(find_device)

    window.show()
    app.exec_()