
#include "sensor.h"

#include "hardware.h"
#include "task.h"

#define SENSOR_DELAY_TICKS		25000 / TICK_SIZE_US
#define SENSOR_IDLE_LOOPS		15

typedef struct {

	uint8_t fails_count;

} sensor_task_parameters_t;

sensor_task_parameters_t sensor_params;

xQueueHandle sensor_queue;

/* Task */

void v_sensor_task(sensor_task_parameters_t *parameters) {
		
	for(;;) {
		sensor_message_t sensor;
				
		if (sensor_measure_temp()) {
			vTaskDelay(SENSOR_MEASURE_TICKS);
			
			uint16_t temp = sensor_read_temp();

			if (temp < 30000) {
				parameters->fails_count = 0;
				sensor.temp = temp / 100;
			} else if (parameters->fails_count < 10) {
				parameters->fails_count += 1;
			} else {
				sensor.fault_state = 1;
			}
		} else if (parameters->fails_count < 10) {
			parameters->fails_count += 1;
		} else {
			sensor.fault_state = 1;
		}

		if (parameters->fails_count) {
			xQueueSend(sensor_queue, &sensor, 20);
        	vTaskDelay(SENSOR_DELAY_TICKS);
    	} else {
    		for (uint8_t i = 0; i < SENSOR_IDLE_LOOPS; i++) {
    			xQueueSend(sensor_queue, &sensor, 20);
    			vTaskDelay(SENSOR_DELAY_TICKS);
    		}
    	}
    }
}

/* Setup */

void v_sensor_task_setup(void) {

	sensor_queue = xQueueCreate(5, sizeof(sensor_message_t));

	xTaskCreate(v_sensor_task, "TM", configMINIMAL_STACK_SIZE, &sensor_params, 1, NULL);
}
