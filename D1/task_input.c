
#include "task_input.h"

#include "FreeRTOS.h"
#include "task.h"
#include <avr/io.h>

typedef struct {
	shared_context_t *shared_context;

} task_input_parameters_t;

task_input_parameters_t task_input_parameters;

/* Task */

void v_task_input(task_input_parameters_t *parameters) {

	for(;;) {
        vTaskDelay(200);
    }

    vTaskDelete(NULL);
}

/* Setup */

void v_setup_task_input(shared_context_t *shared_context) {

	task_input_parameters.shared_context = shared_context;
	
	xTaskCreate(v_task_input, "IN", configMINIMAL_STACK_SIZE, &task_input_parameters, 1, NULL);
}
