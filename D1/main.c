/*
    HomeMatic D1 V1.0.0 (hw:v1.0) - Copyright (C) 2016 vulsim. All rights reserved
	
*/

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"

#include "shared_context.h"
#include "dimmer_task.h"
#include "service_task.h"

shared_context_t shared_context;

void setup_hardware(void) {

}

int main(void) {

	setup_hardware();
	
	v_setup_service_task(&shared_context);
	
	v_setup_dimmer_task(&shared_context);
	
	vTaskStartScheduler();

	return 0;
}
