
#include "dimmer_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include <avr/io.h>

typedef struct {
	shared_context_t *shared_context;

} dimmer_task_parameters_t;

dimmer_task_parameters_t dimmer_task_parameters;

void v_dimmer_task(dimmer_task_parameters_t *parameters) {

	for(;;) {
        vTaskDelay(200);
    }

    vTaskDelete(NULL);
}

void v_setup_dimmer_task(shared_context_t *shared_context) {

	dimmer_task_parameters.shared_context = shared_context;
	
	xTaskCreate(v_dimmer_task, "D1", configMINIMAL_STACK_SIZE, &dimmer_task_parameters, 1, NULL);
}
