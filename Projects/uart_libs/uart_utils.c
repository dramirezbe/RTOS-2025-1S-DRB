#include "uart_utils.h"

void init_uart(uart_port_t port, uint32_t baud_rate, QueueHandle_t *queue, uint32_t buffer_size) {

    uart_config_t config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(port, &config);
    uart_driver_install(port, buffer_size, buffer_size, 10, queue, 0); // Pass the queue pointer directly
}