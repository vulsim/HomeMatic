
#include "FreeRTOS.h"

void v_dimmer_task(void *pvParameters) {

	for(;;) {
        vTaskDelay(200);
    }

    vTaskDelete(NULL);
}
