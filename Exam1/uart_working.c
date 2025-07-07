#include <stdio.h>
#include <string.h> // Required for strcmp

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"

#include "adc_utils.h"
#include "tim_ch_duty.h"

//-----------------Defines----------------------



//----------UART--------------
#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

//----------Queue-------------
static QueueHandle_t uart_queue;
static QueueHandle_t temp_changes_queue;

static uint8_t uart_rx_buffer[RD_BUF_SIZE];

//--------------Structs--------------
typedef struct {
    char color;
    float temp_inf;
    float temp_sup;
} patron_temp_t;

//Global temp threshold array
float temp_levels[3][2] = {
    {40.0f, 80.0f}, // R: min, max
    {20.0f, 40.0f}, // G: min, max
    {0.0f, 20.0f}   // B: min, max
};

patron_temp_t patron_level_temp(char *data) {
    patron_temp_t result_patron = {0}; //Initialize

    sscanf(data, "[%c]%f|%f",
           &result_patron.color,
           &result_patron.temp_inf,
           &result_patron.temp_sup);

    //VERBOSE
    //printf("Parsed: Color=%c, Temp Inf=%.1f, Temp Sup=%.1f\r\n", result_patron.color, result_patron.temp_inf, result_patron.temp_sup);
    return result_patron;
}

// Change global array
void fill_temp_levels(patron_temp_t patron) {
    switch(patron.color){
        case 'R':
            temp_levels[0][0] = patron.temp_inf;
            temp_levels[0][1] = patron.temp_sup;
            printf("Updated Red range: %.1f - %.1f\r\n", temp_levels[0][0], temp_levels[0][1]);
            break;
        case 'G':
            temp_levels[1][0] = patron.temp_inf;
            temp_levels[1][1] = patron.temp_sup;
            printf("Updated Green range: %.1f - %.1f\r\n", temp_levels[1][0], temp_levels[1][1]);
            break;
        case 'B':
            temp_levels[2][0] = patron.temp_inf;
            temp_levels[2][1] = patron.temp_sup;
            printf("Updated Blue range: %.1f - %.1f\r\n", temp_levels[2][0], temp_levels[2][1]);
            break;
        default:
            printf("Error: Use it like: [Led]val|val\r\n");
            break;
    }

    /** VERBOSE
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {

            switch(i) {
                case 0:
                    printf("R-");
                    break;
                case 1:
                    printf("G-");
                    break;
                case 2:
                    printf("B-");
                    break;
            }
            switch(j) {
                case 0:
                    printf("min:");
                    break;
                case 1:
                    printf("max:");
                    break;
            }
            printf(" %.1f\r\n", temp_levels[i][j]);
        }
    }*/
}

// Read UART task
void uart_rx_task(void *arg) {
    //Config UART
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, RD_BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0);

    uart_event_t event;
    while(1) {
        //wait UART RX
        if(xQueueReceive(uart_queue, (void * )&event, (TickType_t)portMAX_DELAY)) {
        
            uart_read_bytes(UART_NUM, uart_rx_buffer, event.size, portMAX_DELAY);
            uart_rx_buffer[event.size] = '\0';

            char *str_buffer = (char *)uart_rx_buffer;
            patron_temp_t received_patron = patron_level_temp(str_buffer);

            xQueueSend(temp_changes_queue, &received_patron, (TickType_t)portMAX_DELAY);
        }
        
    }
}

void handle_duty_task(void *arg) {
    patron_temp_t current_temp;
    while(1) {
        if(xQueueReceive(temp_changes_queue, &current_temp, (TickType_t)portMAX_DELAY)) {
            fill_temp_levels(current_temp);
        }
    }
}

void app_main(void) {
    // Create the queue for temperature changes
    temp_changes_queue = xQueueCreate(10, sizeof(patron_temp_t)); // Queue can hold 10 patron_temp_t items
    if (temp_changes_queue == NULL) {
        printf("Failed to create temp_changes_queue\r\n");
        return;
    }

    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 10, NULL); // Increased stack size for safety
    xTaskCreate(handle_duty_task, "handle_duty_task", 2048, NULL, 5, NULL);
}