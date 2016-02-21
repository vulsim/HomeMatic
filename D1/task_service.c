
#include "task_service.h"

#include "FreeRTOS.h"
#include "task.h"
#include <avr/io.h>
#include <avr/eeprom.h>

typedef struct {
	shared_context_t *shared_context;

} task_service_parameters_t;

task_service_parameters_t task_service_parameters;

/* Some utilities methods */

void request_read_device_settings() {
	task_service_parameters.shared_context->req_read_settings = 1;
}

void request_save_device_settings() {
	task_service_parameters.shared_context->req_save_settings = 1;
}

void read_device_settings(task_service_parameters_t *parameters) {
	parameters->shared_context->req_read_settings = 0;
}

void save_device_settings(task_service_parameters_t *parameters) {
	parameters->shared_context->req_save_settings = 0;
}

void check_hardware(task_service_parameters_t *parameters) {

}

/* Task */

void v_task_service(task_service_parameters_t *parameters) {

	for(;;) {
		check_hardware(parameters);
	
		if (parameters->shared_context->req_save_settings) {
			save_device_settings(parameters);
		} else if (parameters->shared_context->req_read_settings) {
			read_device_settings(parameters);
		}
		
        vTaskDelay(200);
    }

    vTaskDelete(NULL);
}

/* Setup */

void v_setup_task_service(shared_context_t *shared_context) {

	task_service_parameters.shared_context = shared_context;
	
	read_device_settings(&task_service_parameters);
	
	check_hardware(&task_service_parameters);
	
	xTaskCreate(v_task_service, "SV", configMINIMAL_STACK_SIZE, &task_service_parameters, 2, NULL);
}
