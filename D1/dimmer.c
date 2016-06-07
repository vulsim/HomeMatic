
#include "dimmer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include <avr/io.h>

typedef struct {
	
} dimmer_task_parameters_t;

dimmer_task_parameters_t dimmer_task_parameters;

/* Task */

void v_dimmer_task(dimmer_task_parameters_t *parameters) {

	for(;;) {

        vTaskDelay(5);
    }

    dim_off();
    vTaskDelete(NULL);
}

/* Setup */

void v_dimmer_task_setup(void) {
	
	xTaskCreate(v_dimmer_task, "DM", configMINIMAL_STACK_SIZE, &dimmer_task_parameters, 1, NULL);
}
