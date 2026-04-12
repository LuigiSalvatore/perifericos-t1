#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART_PORT0 UART_NUM_0
#define UART_PORT2 UART_NUM_2

#define GPIO_DRIVER_PIN 4
#define BUF_SIZE 1024

void send_temp_com(void){
    uint8_t temp_com[8] = {0x01, 0x04, 0x00, 0x01,
                           0x00, 0x01, 0x60, 0x61}; //Last two for CRC

    gpio_set_level(GPIO_DRIVER_PIN, 1);
    uart_write_bytes(UART_PORT2, (char *)temp_com, 8);
    uart_wait_tx_done(UART_PORT2, pdMS_TO_TICKS(100));
}

void send_humidity_com(void){
    uint8_t humidity_com[8] = {0x01, 0x04, 0x00, 0x02,
                               0x00, 0x01, 0x90, 0x0A}; //Last two for CRC
    
    gpio_set_level(GPIO_DRIVER_PIN, 1);
    uart_write_bytes(UART_PORT2, (char *)humidity_com, 8);
    uart_wait_tx_done(UART_PORT2, pdMS_TO_TICKS(100));
}

void send_temp_humidity_com(void){
    uint8_t temp_humid_com[8] = {0x01, 0x04, 0x00, 0x01,
                                 0x00, 0x02, 0x20, 0x0B}; //Last two for CRC

    gpio_set_level(GPIO_DRIVER_PIN, 1);
    uart_write_bytes(UART_PORT2, (char *)temp_humid_com, 8);
    uart_wait_tx_done(UART_PORT2, pdMS_TO_TICKS(100));
}

int read_data(uint8_t * read_buf){
    gpio_set_level(GPIO_DRIVER_PIN, 0);
    int len = uart_read_bytes(UART_PORT2, read_buf, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
    return len;
}

// Calculates the CRC
uint16_t modbus_crc(uint8_t *buf, int len){
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++)
    {
        crc ^= buf[pos];
        for (int i = 0; i < 8; i++)
        {
            if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

void app_main(void)
{
    uint8_t data[BUF_SIZE];
    uint8_t read_buffer[BUF_SIZE];

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_driver_install(UART_PORT0, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT0, &uart_config);

    uart_driver_install(UART_PORT2, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT2, &uart_config);

    gpio_reset_pin(GPIO_DRIVER_PIN);
    gpio_set_direction(GPIO_DRIVER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_DRIVER_PIN, 0);

    int read_len = 0;
    uint16_t crc_received = 0;
    uint16_t crc_expected = 0;

    while (1)
    {
        int len = uart_read_bytes(UART_PORT0, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);

        if (len >= 0)
        {
            data[len] = '\0';

            if(strstr((char *)data, "th") != 0)
            {
                send_temp_humidity_com();
                vTaskDelay(50 / portTICK_PERIOD_MS);
                read_len = read_data(read_buffer);

                if(read_len == 9){
                    for(int i = 0; i < read_len; i++){
                        char byte_str[7];
                        sprintf(byte_str, "%02X ", read_buffer[i]);
                        uart_write_bytes(UART_PORT0, byte_str, strlen(byte_str));
                    }
                    uart_write_bytes(UART_PORT0, "\n", 1);
                    
                    // Verifies CRC and Check if everything is correct
                    crc_expected = modbus_crc(read_buffer, read_len - 2);
                    crc_received = (read_buffer[read_len - 2] << 8) | (read_buffer[read_len - 1]);

                    if(crc_expected == crc_received){
                        uart_write_bytes(UART_PORT0, "! CRC Okay !\n", strlen("! CRC Okay !\n"));
                    }
                }else{
                    uart_write_bytes(UART_PORT0, "Length Error !\n", strlen("Length Error !\n"));
                    char msg[50];
                    sprintf(msg, "Lenght Read = %d\n", read_len);
                    uart_write_bytes(UART_PORT0, msg, strlen(msg));
                }
            }
            else if(strstr((char *)data, "T") != 0)
            {
                send_temp_com();
                vTaskDelay(50 / portTICK_PERIOD_MS);
                read_len = read_data(read_buffer);

                if(read_len == 7){
                    for(int i = 0; i < read_len; i++){
                        char byte_str[5];
                        sprintf(byte_str, "%02X ", read_buffer[i]);
                        uart_write_bytes(UART_PORT0, byte_str, strlen(byte_str));
                    }
                    uart_write_bytes(UART_PORT0, "\n", 1);

                    // Verifies CRC and Check if everything is correct
                    crc_expected = modbus_crc(read_buffer, read_len - 2);
                    crc_received = (read_buffer[read_len - 2] << 8) | (read_buffer[read_len - 1]);

                    if(crc_expected == crc_received){
                        uart_write_bytes(UART_PORT0, "! CRC Okay !\n", strlen("! CRC Okay !\n"));
                    }
                }else{
                    uart_write_bytes(UART_PORT0, "Length Error !\n", strlen("Length Error !\n"));
                    char msg[50];
                    sprintf(msg, "Lenght Read = %d\n", read_len);
                    uart_write_bytes(UART_PORT0, msg, strlen(msg));
                }
            } 
            else if(strstr((char *)data, "H") != 0)
            {
                    send_humidity_com();
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    read_len = read_data(read_buffer);

                if(read_len == 7){
                    for(int i = 0; i < read_len; i++){
                        char byte_str[5];
                        sprintf(byte_str, "%02X ", read_buffer[i]);
                        uart_write_bytes(UART_PORT0, byte_str, strlen(byte_str));
                    }
                    uart_write_bytes(UART_PORT0, "\n", 1);

                    // Verifies CRC and Check if everything is correct
                    crc_expected = modbus_crc(read_buffer, read_len - 2);
                    crc_received = (read_buffer[read_len - 2] << 8) | (read_buffer[read_len - 1]);

                    if(crc_expected == crc_received){
                        uart_write_bytes(UART_PORT0, "! CRC Okay !\n", strlen("! CRC Okay !\n"));
                    }
                }else{
                    uart_write_bytes(UART_PORT0, "Length Error !\n", strlen("Length Error !\n"));
                    char msg[50];
                    sprintf(msg, "Lenght Read = %d\n", read_len);
                    uart_write_bytes(UART_PORT0, msg, strlen(msg));
                }
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}