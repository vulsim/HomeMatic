/*
    HomeMatic D1/2 V1.0.0 (hw:v1.0) - Copyright (C) 2016 vulsim. All rights reserved	
*/

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"

#include "hardware.h"
#include "dimmer.h"
#include "display.h"
#include "input.h"

int main(void) {

	v_hardware_setup();
	
	v_dimmer_task_setup();
	
	v_display_task_setup();
	
	v_input_task_setup();
	
	vTaskStartScheduler();

	return 0;
}
