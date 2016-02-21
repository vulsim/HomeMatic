
#include "service_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include <avr/io.h>
#include <avr/eeprom.h>

typedef struct {
	shared_context_t *shared_context;

} service_task_parameters_t;

service_task_parameters_t service_task_parameters;

void v_service_task(void *pvParameters) {

	for(;;) {
        vTaskDelay(200);
    }

    vTaskDelete(NULL);
}

void v_setup_service_task(shared_context_t *shared_context) {

	service_task_parameters.shared_context = shared_context;
	
	read_device_settings();
	
	xTaskCreate(v_service_task, "SV", configMINIMAL_STACK_SIZE, &service_task_parameters, 2, NULL);
}

#pragma - Some utilities methods

void request_read_device_settings() {
	service_task_parameters.shared_context->status_flags.request_read_device_settings = 1;
}

void request_save_device_settings() {
	service_task_parameters.shared_context->status_flags.request_save_device_settings = 1;
}

void request_check_hardware() {

}

void read_device_settings() {

}

void save_device_settings() {

}

void check_hardware() {

}