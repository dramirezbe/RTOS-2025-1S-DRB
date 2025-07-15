/**
 * @file main.c
 * @author David Ramírez Betancourth
 * @details Controlar 1 led rgb, leer temp termistor cada 3seg, Botón que active desactive la impresión por consola de la temp (uart),
 * fijar los límites de la página, tanto por página como por uart (intentar hacer).
 */

#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/queue.h"

#include "adc_utils.h"
#include "tim_ch_duty.h"
#include "io_utils.h"

#include "wifi_app.h"
#include "http_server.h"

//-------------------ADC-----------------------
#define NTC_ADC_CH ADC_CHANNEL_5 //IO 33
#define ADC_UNIT   ADC_UNIT_1
#define ADC_ATTEN  ADC_ATTEN_DB_12

//-------------------RGB-----------------------
#define R_CHANNEL     LEDC_CHANNEL_0
#define G_CHANNEL     LEDC_CHANNEL_1
#define B_CHANNEL     LEDC_CHANNEL_2
#define R_IO          GPIO_NUM_27
#define G_IO          GPIO_NUM_26
#define B_IO          GPIO_NUM_25
#define LED_FREQUENCY 1000

#define IS_RGB_COMMON_ANODE true

//-------------------UART----------------------
#define UART_NUM UART_NUM_0
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

#define BLINK_GPIO 2

//-----------------------------------------Struct----------------------------------------
typedef struct {
    char color;
    float temp_inf;
    float temp_sup;
} patron_temp_t;

//---------------------------------------Global Vars ------------------------------------
QueueHandle_t rgb_event_queue;
static QueueHandle_t uart_queue;
static QueueHandle_t temp_changes_queue;


float temp_levels[3][2] = {
    {40.0f, 80.0f}, // R: min, max
    {20.0f, 40.0f}, // G: min, max
    {0.0f, 20.0f}   // B: min, max
};

bool uart_on = true;

static uint8_t uart_rx_buffer[RD_BUF_SIZE];

//------------------------------------Config Peripherals-------------------------------------

// LEDC Timer and Channels configuration
pwm_timer_config_t timer = {.frequency_hz = LED_FREQUENCY, .resolution_bit = LEDC_TIMER_10_BIT, .timer_num = LEDC_TIMER_0};

rgb_pwm_t led_rgb = {
    .red   = { .channel = R_CHANNEL, .gpio_num = R_IO, .duty_percent = 0 },
    .green = { .channel = G_CHANNEL, .gpio_num = G_IO, .duty_percent = 0 },
    .blue  = { .channel = B_CHANNEL, .gpio_num = B_IO, .duty_percent = 0 }
};

// ADC Configurations and Handles
adc_config_t ntc_adc_conf = {
    .unit_id = ADC_UNIT,
    .channel = NTC_ADC_CH,
    .atten = ADC_ATTEN,
    .bitwidth = ADC_BITWIDTH_12,
};
adc_channel_handle_t ntc_adc_handle = NULL;

//-----------------------------------Helper Functions------------------------------------------

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

static void configure_blink_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
	//rgb_led_pwm_init();
}



void rgb_event_task(void *pvParameters) {
	rgb_values_t rgb_values;
	while (1) {
		if(xQueueReceive(rgb_event_queue, &rgb_values, portMAX_DELAY)) {
			printf("Queue Received RGB values: Red=%d, Green=%d, Blue=%d\n", rgb_values.red_val, rgb_values.green_val, rgb_values.blue_val);

		}
		
	}
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

    printf("UART initialized.\r\n");

    uart_event_t event;
    while(1) {
        //wait UART RX
        if(xQueueReceive(uart_queue, (void * )&event, 20)) {
        
            uart_read_bytes(UART_NUM, uart_rx_buffer, event.size, portMAX_DELAY);
            uart_rx_buffer[event.size] = '\0';

            char *str_buffer = (char *)uart_rx_buffer;

			patron_temp_t received_patron = patron_level_temp(str_buffer);
			xQueueSend(temp_changes_queue, &received_patron, (TickType_t)portMAX_DELAY);
            
        }
    }
}

void app_main(void)
{

	rgb_event_queue = xQueueCreate(3, sizeof(rgb_values_t));
	temp_changes_queue = xQueueCreate(3, sizeof(patron_temp_t));

    // Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	
	configure_blink_led();
	// Start Wifi
	wifi_app_start();

	xTaskCreate(rgb_event_task, "rgb_event_task", 2048, NULL, 5, NULL);

}

