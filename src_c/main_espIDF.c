#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART_PORT0 UART_NUM_0
#define UART_PORT2 UART_NUM_2

#define TXD2 17
#define RXD2 16

#define GPIO_DRIVER_PIN 4
#define BUF_SIZE 1024

#define TIMEOUT_MS 500

// ================= CRC =================
uint16_t modbus_crc(uint8_t *buf, int len){
    uint16_t crc = 0xFFFF;

    for (int pos = 0; pos < len; pos++){
        crc ^= buf[pos];
        for (int i = 0; i < 8; i++){
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

// ================= DEBUG =================
void print_hex(uint8_t *buf, int len){
    char byte_str[5];
    for(int i = 0; i < len; i++){
        sprintf(byte_str, "%02X ", buf[i]);
        uart_write_bytes(UART_PORT0, byte_str, strlen(byte_str));
    }
    uart_write_bytes(UART_PORT0, "\n", 1);
}

// ================= RS485 SEND =================
void rs485_send(uint8_t *data, int len){

    gpio_set_level(GPIO_DRIVER_PIN, 1); // TX enable
    vTaskDelay(pdMS_TO_TICKS(10));

    uart_write_bytes(UART_PORT2, (char *)data, len);
    uart_wait_tx_done(UART_PORT2, pdMS_TO_TICKS(100));

    gpio_set_level(GPIO_DRIVER_PIN, 0); // RX enable
}

// ================= RS485 READ =================
int rs485_read(uint8_t *buf){
    int total = 0;
    int elapsed = 0;

    while (elapsed < TIMEOUT_MS){
        int len = uart_read_bytes(UART_PORT2,
                                  buf + total,
                                  BUF_SIZE - total,
                                  pdMS_TO_TICKS(10));

        if(len > 0){
            total += len;
        }

        elapsed += 10;
    }

    return total;
}

// ================= MAIN =================
void app_main(void){

    uint8_t data[BUF_SIZE];
    uint8_t read_buffer[BUF_SIZE];

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    // UART0 (debug)
    uart_driver_install(UART_PORT0, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT0, &uart_config);

    // UART2 (RS485)
    uart_driver_install(UART_PORT2, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT2, &uart_config);

    uart_set_pin(UART_PORT2, TXD2, RXD2,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // GPIO RS485
    gpio_reset_pin(GPIO_DRIVER_PIN);
    gpio_set_direction(GPIO_DRIVER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_DRIVER_PIN, 0);

    // ====== Frame base ======
    uint8_t frame[8];

    while (1){

        int len = uart_read_bytes(UART_PORT0,
                                 data,
                                 BUF_SIZE - 1,
                                 pdMS_TO_TICKS(100));

        if(len > 0){

            data[len] = '\0';

            // ================= TEMPERATURA =================
            if(strstr((char *)data, "T") != NULL){

                uint8_t msg[6] = {0x01, 0x04, 0x00, 0x01, 0x00, 0x01};

                uint16_t crc = modbus_crc(msg, 6);

                frame[0] = msg[0];
                frame[1] = msg[1];
                frame[2] = msg[2];
                frame[3] = msg[3];
                frame[4] = msg[4];
                frame[5] = msg[5];
                frame[6] = crc & 0xFF;          // CRC L
                frame[7] = (crc >> 8) & 0xFF;   // CRC H

                uart_write_bytes(UART_PORT0, "TX: ", 4);
                print_hex(frame, 8);

                rs485_send(frame, 8);

                int read_len = rs485_read(read_buffer);

                uart_write_bytes(UART_PORT0, "RX: ", 4);
                print_hex(read_buffer, read_len);

                if(read_len >= 5){

                    uint16_t crc_calc = modbus_crc(read_buffer, read_len - 2);
                    uint16_t crc_recv = read_buffer[read_len - 2] |
                                        (read_buffer[read_len - 1] << 8);

                    if(crc_calc == crc_recv){
                        uart_write_bytes(UART_PORT0, "CRC OK\n", 7);

                        uint16_t value = (read_buffer[3] << 8) | read_buffer[4];

                        char msg_out[50];
                        sprintf(msg_out, "Value: %d\n", value);
                        uart_write_bytes(UART_PORT0, msg_out, strlen(msg_out));
                    } else {
                        uart_write_bytes(UART_PORT0, "CRC ERROR\n", 10);
                    }
                }
            }

            // ================= HUMIDADE =================
            else if(strstr((char *)data, "H") != NULL){

                uint8_t msg[6] = {0x01, 0x04, 0x00, 0x02, 0x00, 0x01};

                uint16_t crc = modbus_crc(msg, 6);

                for(int i = 0; i < 6; i++) frame[i] = msg[i];
                frame[6] = crc & 0xFF;
                frame[7] = (crc >> 8) & 0xFF;

                uart_write_bytes(UART_PORT0, "TX: ", 4);
                print_hex(frame, 8);

                rs485_send(frame, 8);

                int read_len = rs485_read(read_buffer);

                uart_write_bytes(UART_PORT0, "RX: ", 4);
                print_hex(read_buffer, read_len);
            }

            // ================= TEMP + HUM =================
            else if(strstr((char *)data, "th") != NULL){

                uint8_t msg[6] = {0x01, 0x04, 0x00, 0x01, 0x00, 0x02};

                uint16_t crc = modbus_crc(msg, 6);

                for(int i = 0; i < 6; i++) frame[i] = msg[i];
                frame[6] = crc & 0xFF;
                frame[7] = (crc >> 8) & 0xFF;

                uart_write_bytes(UART_PORT0, "TX: ", 4);
                print_hex(frame, 8);

                rs485_send(frame, 8);

                int read_len = rs485_read(read_buffer);

                uart_write_bytes(UART_PORT0, "RX: ", 4);
                print_hex(read_buffer, read_len);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}