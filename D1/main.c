/*
    HomeMatic D1/2 V1.0.0 (hw:v1.0) - Copyright (C) 2016 vulsim. All rights reserved	
*/

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"

#include "shared_context.h"
#include "hardware.h"
#include "task_dimmer.h"
#include "task_display.h"
#include "task_input.h"

shared_context_t shared_context;

int main(void) {

	v_setup_hardware();
	
	v_setup_task_dimmer(&shared_context);
	
	v_setup_task_display(&shared_context);
	
	v_setup_task_input(&shared_context);
	
	vTaskStartScheduler();

	return 0;
}
