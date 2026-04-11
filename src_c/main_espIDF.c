#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

#define UART_PORT UART_NUM_0
#define BUF_SIZE 1024

void app_main(void)
{
    uint8_t data[BUF_SIZE];

    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

    uart_driver_install(UART_PORT, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &uart_config);

    while (1)
    {
        int len = uart_read_bytes(UART_PORT, data, BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);

        if (len > 0)
        {
            data[len] = '\0';

            if (strncmp((char *)data, "T", 1) == 0)
            {
                char *resp = "T:25.5\n";
                uart_write_bytes(UART_PORT, resp, strlen(resp));
            }
            else if (strncmp((char *)data, "H", 1) == 0)
            {
                char *resp = "H:60\n";
                uart_write_bytes(UART_PORT, resp, strlen(resp));
            }
            else if (strncmp((char *)data, "TH", 2) == 0)
            {
                char *resp = "TH:25.5,60\n";
                uart_write_bytes(UART_PORT, resp, strlen(resp));
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}