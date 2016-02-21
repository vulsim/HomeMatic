
#include "task_display.h"

#include "FreeRTOS.h"
#include "task.h"
#include <avr/io.h>

typedef struct {
	shared_context_t *shared_context;

} task_display_parameters_t;

task_display_parameters_t task_display_parameters;

/* Task */

void v_task_display(task_display_parameters_t *parameters) {

	for(;;) {
        vTaskDelay(200);
    }

    vTaskDelete(NULL);
}

/* Setup */

void v_setup_task_display(shared_context_t *shared_context) {

	task_display_parameters.shared_context = shared_context;
	
	xTaskCreate(v_task_display, "OU", configMINIMAL_STACK_SIZE, &task_display_parameters, 1, NULL);
}
