
#include "display.h"

#include "FreeRTOS.h"
#include "task.h"
#include <avr/io.h>

typedef struct {

} display_task_parameters_t;

display_task_parameters_t display_task_parameters;

/* Task */

void v_display_task(display_task_parameters_t *parameters) {

	for(;;) {
        vTaskDelay(200);
    }

    vTaskDelete(NULL);
}

/* Setup */

void v_display_task_setup(void) {
	
	xTaskCreate(v_display_task, "DS", configMINIMAL_STACK_SIZE, &display_task_parameters, 1, NULL);
}
