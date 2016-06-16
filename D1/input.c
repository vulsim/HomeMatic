
#include "input.h"

#include "FreeRTOS.h"
#include "task.h"

typedef struct {
	

} input_task_parameters_t;

input_task_parameters_t input_task_parameters;
QueueHandle_t input_queue;

/* Task */

void v_input_task(input_task_parameters_t *parameters) {

	for(;;) {
        vTaskDelay(200);
    }

    vTaskDelete(NULL);
}

/* Setup */

void v_input_task_setup(void) {

	input_queue = xQueueCreate(3, sizeof(input_message_t));
	
	xTaskCreate(v_input_task, "IN", configMINIMAL_STACK_SIZE, &input_task_parameters, 1, NULL);
}
