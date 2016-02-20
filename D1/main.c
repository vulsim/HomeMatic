/*
    HomeMatic D1 V1.0.0 - Copyright (C) 2016 vulsim. All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/delay.h>
#include <avr/eeprom.h>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"

#include "runtime_config.h"
#include "dimmer_task.h"

runtime_config_t ee_runtime_config;

int main(void) {

	initialize();
	
	read_runtime_config();
	
	xTaskCreate(v_dimmer_task, "dim", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

	return 0;
}

void initialize(void) {

}

void read_runtime_config() {

}

void write_runtime_config() {

}

