#include <stdio.h>
#include "led_lib.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define LED_1_R_CHANNEL         LEDC_CHANNEL_0
#define LED_1_G_CHANNEL         LEDC_CHANNEL_1
#define LED_1_B_CHANNEL         LEDC_CHANNEL_2
#define LED_1_R_GPIO            GPIO_NUM_3
#define LED_1_G_GPIO            GPIO_NUM_4
#define LED_1_B_GPIO            GPIO_NUM_5
#define LED_FREQUENCY           1000
#define GPIO_BUTTON             (1ULL<<GPIO_NUM_0)

static QueueHandle_t gpio_evt_queue = NULL;

void led_rgb_task(){
    //configurar led
    LED_RGB LED_1 ={
        .led_r = {
            .canal = LED_1_R_CHANNEL,
            .gpio = LED_1_R_GPIO,
            .duty_resolution = LEDC_TIMER_10_BIT,
            .duty = 0            
        },
        .led_g = {
            .canal = LED_1_G_CHANNEL,
            .gpio = LED_1_G_GPIO,
            .duty_resolution = LEDC_TIMER_10_BIT,
            .duty = 0            
        },
        .led_b = {
            .canal = LED_1_B_CHANNEL,
            .gpio = LED_1_B_GPIO,
            .duty_resolution = LEDC_TIMER_10_BIT,
            .duty = 0            
        },
        .timer_num = LEDC_TIMER_0,
        .freq_hz = LED_FREQUENCY        
    };
    config_timer(LED_1.timer_num, LED_1.led_g.duty_resolution , LED_1.freq_hz);
    config_led_rgb( LED_1 );
    cambiar_intensidad_led_rgb( &LED_1 , 100, 0, 0 );
    while(1){
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)){
            if (io_num == GPIO_BUTTON){
                LED_1.led_r.duty/100 
                cambiar_intensidad_led_rgb( &LED_1 , duty*100/ 1 << duty_resolution + 25 , 0, 0 ); 
            }
            
        }
        cambiar_intensidad_led_rgb( &LED_1 , 100, 0, 0 );
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        cambiar_intensidad_led_rgb( &LED_1 , 0, 100, 0 );
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        cambiar_intensidad_led_rgb( &LED_1 , 0, 0, 100 );
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        cambiar_intensidad_led_rgb( &LED_1 , 100, 100, 100 );
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}


void config_interrupt(){
     //zero-initialize the config structure.
     gpio_config_t io_conf = {};
     //disable interrupt
     io_conf.intr_type = GPIO_INTR_NEGEDGE;
     //set as output mode
     io_conf.mode = GPIO_MODE_INPUT;
     //bit mask of the pins that you want to set,e.g.GPIO18/19
     io_conf.pin_bit_mask = GPIO_BUTTON;
     //disable pull-down mode
     io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
     //disable pull-up mode
     io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
     //configure GPIO with the given settings
     gpio_config(&io_conf);
     
      //install gpio isr service
     gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
     //hook isr handler for specific gpio pin
     gpio_isr_handler_add(GPIO_BUTTON, gpio_isr_handler, (void*) GPIO_BUTTON));
     //create a queue to handle gpio event from isr
     gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
 
}

void app_main(void)
{
    config_interrupt();
    xTaskCreatePinnedToCore(led_rgb_task, "tarealed", 2048, NULL, 4, NULL, 0);//Se crea una tarea de alta prioridad
    

}
