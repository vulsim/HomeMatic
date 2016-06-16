
#include "display.h"

#include <avr/io.h>
#include "FreeRTOS.h"
#include "task.h"
#include "dimmer.h"
#include "hardware.h"

typedef struct {
	
	uint8_t light_on : 1;

	uint8_t blink_counter;

} display_task_parameters_t;

display_task_parameters_t display_task_parameters;

/* Task */

void v_display_task(display_task_parameters_t *parameters) {

	for(;;) {
		dimmer_message_t dimmer;

		if (parameters->blink_counter < 50) {
			parameters->blink_counter++;
		} else {
			parameters->light_on = parameters->light_on ? 0 : 1;
			parameters->blink_counter = 0;
		}
		
		if (xQueueReceive(dimmer_queue, &dimmer, 10)) {
			taskENTER_CRITICAL();

			if (dimmer.fault_state) {
				if (parameters->light_on) {
					rled_on();					
				} else {
					rled_off();
				}
				gled_off();
			} else if (dimmer.wait_sync_state) {
				if (parameters->light_on) {
					rled_on();					
					gled_on();
				} else {
					rled_off();
					gled_off();
				}				
			} else if (dimmer.on_state) {
				rled_off();
				gled_on();
			} else {
				rled_on();
				gled_on();
			}

			taskEXIT_CRITICAL(); 
		}

        vTaskDelay(20000 / TICK_SIZE_US);
    }
}

/* Setup */

void v_display_task_setup(void) {
	
	xTaskCreate(v_display_task, "DS", configMINIMAL_STACK_SIZE, &display_task_parameters, 1, NULL);
}
