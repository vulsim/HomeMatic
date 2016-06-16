
#include "input.h"

#include "hardware.h"
#include "task.h"

typedef struct {

	uint8_t key_press_duration;

} input_task_parameters_t;

input_task_parameters_t input_task_parameters;

QueueHandle_t input_queue;

/* Task */

void v_input_task(input_task_parameters_t *parameters) {

	for(;;) {
		input_message_t input = { 
			.is_key_pressed = 0, 
			.key_press_duration = 0
		};

		if (is_key_pressed()) {
			if (parameters->key_press_duration < 125) { 
				parameters->key_press_duration += 1;
			}

			input.is_key_pressed = 1;
			input.key_press_duration = parameters->key_press_duration;
		} else {
			parameters->key_press_duration = 0;
		}

		xQueueSend(input_queue, &input, 10);
        vTaskDelay(50000 / TICK_SIZE_US);
    }
}

/* Setup */

void v_input_task_setup(void) {

	input_queue = xQueueCreate(5, sizeof(input_message_t));

	xTaskCreate(v_input_task, "IN", configMINIMAL_STACK_SIZE, &input_task_parameters, 1, NULL);
}
