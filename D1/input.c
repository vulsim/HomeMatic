
#include "input.h"

#include "hardware.h"
#include "task.h"

#define INPUT_DELAY_TICKS		25000 / TICK_SIZE_US

typedef struct {

	uint8_t key_press_duration;

} input_task_parameters_t;

input_task_parameters_t input_params;

xQueueHandle input_queue;

/* Task */

void v_input_task(input_task_parameters_t *parameters) {
		
	for(;;) {
		input_message_t input;

		taskENTER_CRITICAL();
		
		if (is_key_pressed()) {
			input.is_key_pressed = 1;
			input.key_press_duration = parameters->key_press_duration;

			if (parameters->key_press_duration < 125) { 
				parameters->key_press_duration += 1;
			}
		} else {
			input.is_key_pressed = 0;			
			input.key_press_duration = parameters->key_press_duration;
			
			parameters->key_press_duration = 0;
		}
		
		taskEXIT_CRITICAL();

		xQueueSend(input_queue, &input, 20);
        vTaskDelay(INPUT_DELAY_TICKS);
    }
}

/* Setup */

void v_input_task_setup(void) {

	input_params.key_press_duration = 0;

	input_queue = xQueueCreate(5, sizeof(input_message_t));

	xTaskCreate(v_input_task, "KS", configMINIMAL_STACK_SIZE, &input_params, 1, NULL);
}
