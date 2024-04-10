#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdio.h>

QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, -1);
}

void x_task(void *p) {
    adc_gpio_init(27);
    adc_t data;
    data.axis = 0;
    int deadzone_min = -160;
    int deadzone_max = 160;

    while(1){
        adc_select_input(1);
        int result = adc_read();

        
        int average = (result - 2048)/8;

        // Verifica se o valor está dentro da zona morta
        if (average < deadzone_min || average > deadzone_max)
        {
            data.val = average/2;
            xQueueSend(xQueueAdc, &data, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) { 
    adc_gpio_init(26);
    adc_t data;
    data.axis = 1;
    int deadzone_min = -160;
    int deadzone_max = 160;

    while(1){
        adc_select_input(0);
        int result = adc_read();

        
        int average = (result - 2048)/8;

        // Verifica se o valor está dentro da zona morta
        if (average < deadzone_min || average > deadzone_max)
        {
            data.val = average/2;
            xQueueSend(xQueueAdc, &data, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_task(void *p) {
    adc_t data;
    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);
        write_package(data);
    }
}

int main() {
    stdio_init_all();

    adc_init();

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(x_task, "x_task", 4096, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
