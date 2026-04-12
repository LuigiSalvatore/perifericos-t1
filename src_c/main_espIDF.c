#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART_PORT0 UART_NUM_0
#define UART_PORT2 UART_NUM_2

#define RXD2 16
#define TXD2 17

#define GPIO_DRIVER_PIN 4
#define BUF_SIZE 1024

// #define DEBUG 0

void send_temp_com(void)
{
    uint8_t temp_com[8] = {0x01, 0x04, 0x00, 0x01,
                           0x00, 0x01, 0x60, 0x0A}; // Last two for CRC

    gpio_set_level(GPIO_DRIVER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    uart_write_bytes(UART_PORT2, (char *)temp_com, 8);
    uart_wait_tx_done(UART_PORT2, pdMS_TO_TICKS(100));
}

void send_humidity_com(void)
{
    uint8_t humidity_com[8] = {0x01, 0x04, 0x00, 0x02,
                               0x00, 0x01, 0x90, 0x0A}; // Last two for CRC

    gpio_set_level(GPIO_DRIVER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    uart_write_bytes(UART_PORT2, (char *)humidity_com, 8);
    uart_wait_tx_done(UART_PORT2, pdMS_TO_TICKS(100));
}

void send_temp_humidity_com(void)
{
    uint8_t temp_humid_com[8] = {0x01, 0x04, 0x00, 0x01,
                                 0x00, 0x02, 0x20, 0x0B}; // Last two for CRC

    gpio_set_level(GPIO_DRIVER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    uart_write_bytes(UART_PORT2, (char *)temp_humid_com, 8);
    uart_wait_tx_done(UART_PORT2, pdMS_TO_TICKS(100));
}

int read_data(uint8_t *read_buf, int expected_len)
{
    gpio_set_level(GPIO_DRIVER_PIN, 0);
    int total = 0;
    int timeout = 200; // ms
    int elapsed = 0;
    while (total < expected_len && elapsed < timeout)
    {
        int len = uart_read_bytes(UART_PORT2, read_buf + total, expected_len - total, pdMS_TO_TICKS(20));

        if (len > 0)
        {
            total += len;
        }

        elapsed += 20;
    }

    return total;
}

// Calculates the CRC
uint16_t modbus_crc(uint8_t *buf, int len)
{
    uint16_t crc = 0xFFFF;
    for (int pos = 0; pos < len; pos++)
    {
        crc ^= buf[pos];
        for (int i = 0; i < 8; i++)
        {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
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
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_driver_install(UART_PORT0, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT0, &uart_config);

    uart_driver_install(UART_PORT2, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT2, &uart_config);
    uart_set_pin(UART_PORT2, TXD2, RXD2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    gpio_reset_pin(GPIO_DRIVER_PIN);
    gpio_set_direction(GPIO_DRIVER_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_DRIVER_PIN, 0);

    uart_write_bytes(UART_PORT0, "[Start Code]\n", strlen("[Start Code]\n"));

    int read_len = 0;
    uint16_t crc_received = 0;
    uint16_t crc_expected = 0;

    uint16_t temp;
    uint16_t humid;

    while (1)
    {
        int len = uart_read_bytes(UART_PORT0, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);

        if (len >= 0)
        {
            data[len] = '\0';

            if (strstr((char *)data, "th") != 0)
            {
                send_temp_humidity_com();
                vTaskDelay(50 / portTICK_PERIOD_MS);
                read_len = read_data(read_buffer, 9);

                if (read_len == 9)
                {
#ifdef DEBUG
                    for (int i = 0; i < read_len; i++)
                    {
                        char byte_str[7];
                        sprintf(byte_str, "%02X ", read_buffer[i]);
                        uart_write_bytes(UART_PORT0, byte_str, strlen(byte_str));
                    }
                    uart_write_bytes(UART_PORT0, "\n", 1);
#endif

                    // Verifies CRC and Check if everything is correct
                    crc_expected = modbus_crc(read_buffer, read_len - 2);
                    crc_received = read_buffer[read_len - 2] | (read_buffer[read_len - 1] << 8);

                    if (crc_expected == crc_received)
                    {
#ifdef DEBUG
                        uart_write_bytes(UART_PORT0, "! CRC Okay !\n", strlen("! CRC Okay !\n"));
#endif

                        char send_data[50];
                        temp = read_buffer[3] << 8 | read_buffer[4];
                        humid = (read_buffer[5] << 8 | read_buffer[6]) / 10;
                        float temp_f = temp / 10;
                        sprintf(send_data, "TH:%.1f,%02d\n", temp_f, humid);
                        uart_write_bytes(UART_PORT0, send_data, strlen(send_data));
                    }
                }
                else
                {
#ifdef DEBUG
                    uart_write_bytes(UART_PORT0, "Length Error !\n", strlen("Length Error !\n"));
                    char msg[50];
                    sprintf(msg, "Lenght Read = %d\n", read_len);
                    uart_write_bytes(UART_PORT0, msg, strlen(msg));
#endif
                }
            }
            else if (strstr((char *)data, "T") != 0)
            {
                send_temp_com();
                vTaskDelay(50 / portTICK_PERIOD_MS);
                read_len = read_data(read_buffer, 7);

                if (read_len == 7)
                {
#ifdef DEBUG
                    for (int i = 0; i < read_len; i++)
                    {
                        char byte_str[5];
                        sprintf(byte_str, "%02X ", read_buffer[i]);
                        uart_write_bytes(UART_PORT0, byte_str, strlen(byte_str));
                    }
                    uart_write_bytes(UART_PORT0, "\n", 1);
#endif

                    // Verifies CRC and Check if everything is correct
                    crc_expected = modbus_crc(read_buffer, read_len - 2);
                    crc_received = read_buffer[read_len - 2] | (read_buffer[read_len - 1] << 8);

                    if (crc_expected == crc_received)
                    {
#ifdef DEBUG
                        uart_write_bytes(UART_PORT0, "! CRC Okay !\n", strlen("! CRC Okay !\n"));
#endif

                        char send_data[50];
                        temp = read_buffer[3] << 8 | read_buffer[4];
                        float temp_f = temp / 10;
                        sprintf(send_data, "T:%.1f\n", temp_f);
                        uart_write_bytes(UART_PORT0, send_data, strlen(send_data));
                    }
                }
                else
                {
#ifdef DEBUG
                    uart_write_bytes(UART_PORT0, "Length Error !\n", strlen("Length Error !\n"));
                    char msg[50];
                    sprintf(msg, "Lenght Read = %d\n", read_len);
                    uart_write_bytes(UART_PORT0, msg, strlen(msg));
#endif
                }
            }
            else if (strstr((char *)data, "H") != 0)
            {
                send_humidity_com();
                vTaskDelay(50 / portTICK_PERIOD_MS);
                read_len = read_data(read_buffer, 7);

                if (read_len == 7)
                {
#ifdef DEBUG
                    for (int i = 0; i < read_len; i++)
                    {
                        char byte_str[5];
                        sprintf(byte_str, "%02X ", read_buffer[i]);
                        uart_write_bytes(UART_PORT0, byte_str, strlen(byte_str));
                    }
                    uart_write_bytes(UART_PORT0, "\n", 1);
#endif

                    // Verifies CRC and Check if everything is correct
                    crc_expected = modbus_crc(read_buffer, read_len - 2);
                    crc_received = read_buffer[read_len - 2] | (read_buffer[read_len - 1] << 8);

                    if (crc_expected == crc_received)
                    {
#ifdef DEBUG
                        uart_write_bytes(UART_PORT0, "! CRC Okay !\n", strlen("! CRC Okay !\n"));
#endif

                        char send_data[50];
                        humid = (read_buffer[3] << 8 | read_buffer[4]) / 10;
                        sprintf(send_data, "H:%d\n", humid);
                        uart_write_bytes(UART_PORT0, send_data, strlen(send_data));
                    }
                }
                else
                {
#ifdef DEBUG
                    uart_write_bytes(UART_PORT0, "Length Error !\n", strlen("Length Error !\n"));
                    char msg[50];
                    sprintf(msg, "Lenght Read = %d\n", read_len);
                    uart_write_bytes(UART_PORT0, msg, strlen(msg));
#endif
                }
            }
            else
            {
                // Custom command - data is already in binary format
                if (len >= 6)
                {
// Send Data Process
#ifdef DEBUG
                    uart_write_bytes(UART_PORT0, "Sending custom command: ", strlen("Sending custom command: "));
                    for (int i = 0; i < len; i++)
                    {
                        char byte_str[7];
                        sprintf(byte_str, "%02X ", data[i]);
                        uart_write_bytes(UART_PORT0, byte_str, strlen(byte_str));
                    }
                    uart_write_bytes(UART_PORT0, "\n", 1);
#endif

                    // Calculate CRC and append it
                    uint16_t crc_generated = modbus_crc(data, len);
                    uint8_t cmd_with_crc[BUF_SIZE];
                    memcpy(cmd_with_crc, data, len);
                    cmd_with_crc[len] = (crc_generated & 0x00FF);
                    cmd_with_crc[len + 1] = ((crc_generated & 0xFF00) >> 8);

                    gpio_set_level(GPIO_DRIVER_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(10));
                    uart_write_bytes(UART_PORT2, (char *)cmd_with_crc, len + 2);
                    uart_wait_tx_done(UART_PORT2, pdMS_TO_TICKS(100));
                    vTaskDelay(50 / portTICK_PERIOD_MS);

#ifdef DEBUG
                    uart_write_bytes(UART_PORT0, "CRC added: ", strlen("CRC added: "));
                    char crc_str[10];
                    sprintf(crc_str, "%02X %02X\n", cmd_with_crc[len], cmd_with_crc[len + 1]);
                    uart_write_bytes(UART_PORT0, crc_str, strlen(crc_str));
#endif

                    // Receive Data Process (If command is for this purpose - function codes 0x03 or 0x04)
                    if (data[1] == 0x03 || data[1] == 0x04)
                    {
                        read_len = read_data(read_buffer, 9);

                        // Send raw binary response to Python
                        uart_write_bytes(UART_PORT0, (char *)read_buffer, read_len);
                    }
                }
                else
                {
#ifdef DEBUG
                    uart_write_bytes(UART_PORT0, "Command too short (min 6 bytes)\n", strlen("Command too short (min 6 bytes)\n"));
#endif
                }
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}