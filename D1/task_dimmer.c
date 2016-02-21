
#include "task_dimmer.h"

#include "FreeRTOS.h"
#include "task.h"
#include <avr/io.h>

typedef struct {
	unsigned state : 2; // 0 - off, 1 - on (mode1), 2 - on (mode2)

	shared_context_t *shared_context;

} task_dimmer_parameters_t;

task_dimmer_parameters_t task_dimmer_parameters;

/* Task */

void v_task_dimmer(task_dimmer_parameters_t *parameters) {

	for(;;) {
        vTaskDelay(200);
    }

    vTaskDelete(NULL);
}

/* Setup */

void v_setup_task_dimmer(shared_context_t *shared_context) {

	task_dimmer_parameters.shared_context = shared_context;
	
	xTaskCreate(v_task_dimmer, "DA", configMINIMAL_STACK_SIZE, &task_dimmer_parameters, 1, NULL);
}
