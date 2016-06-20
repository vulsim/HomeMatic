
#include "display.h"

#include <avr/io.h>
#include "dimmer.h"
#include "task.h"
#include "hardware.h"

#define DISPLAY_DELAY_TICKS		20000 / TICK_SIZE_US

typedef struct {
	
	uint8_t light_on : 1;

	uint8_t blink_counter;

	uint8_t display_state;

} display_task_parameters_t;

display_task_parameters_t display_params;

/* Task */

void v_display_task(display_task_parameters_t *parameters) {

	for(;;) {
		dimmer_message_t dimmer;
		
		if (xQueueReceive(dimmer_queue, &dimmer, 10)) {
			uint8_t display_state = 0;

			if (dimmer.fault_state) {
				display_state = 3;
			} else if (dimmer.on_state) {
				if (dimmer.wait_sync_state) {
					display_state = 2;	
				} else {
					display_state = 1;
				}
			} else {
				display_state = 0;
			}

			if (parameters->display_state != display_state) {
				parameters->light_on = 1;
				parameters->blink_counter = 0;
				parameters->display_state = display_state;
			}
		}
		
		taskENTER_CRITICAL();

		switch (display_params.display_state) {
			case 0:
				rled_off();
				gled_off();
				break;
			case 1:
				rled_off();
				gled_on();
				break;
			case 2:	
				if (display_params.light_on) {
					rled_on();
					gled_on();
				} else {
					rled_off();
					gled_off();
				}
				break;		
			case 3:
				if (display_params.light_on) {
					rled_on();
					gled_off();
				} else {
					rled_off();
					gled_off();
				}
				break;
		}
			
		taskEXIT_CRITICAL(); 

		if (display_params.blink_counter < 50) {
			display_params.blink_counter += 1;
		} else {
			display_params.light_on = display_params.light_on ? 0 : 1;
			display_params.blink_counter = 0;
		}
		
        vTaskDelay(DISPLAY_DELAY_TICKS);
    }
}

/* Setup */

void v_display_task_setup(void) {

	display_params.light_on = 1;
	display_params.blink_counter = 0;
	
	xTaskCreate(v_display_task, "DS", configMINIMAL_STACK_SIZE, &display_params, 1, NULL);
}
