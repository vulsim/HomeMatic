
#include "dimmer.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "task.h"
#include "hardware.h"
#include "input.h"
#include "sensor.h"

#define DIMMER_DELAY_TICKS		20000 / TICK_SIZE_US

const uint8_t power_map_table[100] PROGMEM ={
	0x03, 0x08, 0x0B, 0x0E, 0x11, 0x13, 0x16, 0x18, 0x1A, 0x1C, 
	0x1E, 0x20, 0x22, 0x24, 0x26, 0x28, 0x29, 0x2B, 0x2D, 0x2E, 
	0x30, 0x32, 0x33, 0x35, 0x37, 0x38, 0x3A, 0x3B, 0x3D, 0x3F, 
	0x40, 0x42, 0x43, 0x45, 0x46, 0x48, 0x49, 0x4B, 0x4C, 0x4E, 
	0x4F, 0x51, 0x52, 0x54, 0x55, 0x57, 0x58, 0x5A, 0x5B, 0x5D, 
	0x5E, 0x60, 0x61, 0x63, 0x65, 0x66, 0x68, 0x69, 0x6B, 0x6C, 
	0x6E, 0x70, 0x71, 0x73, 0x75, 0x76, 0x78, 0x7A, 0x7C, 0x7D, 
	0x7F, 0x81, 0x83, 0x85, 0x87, 0x88, 0x8A, 0x8C, 0x8F, 0x91, 
	0x93, 0x95, 0x97, 0x9A, 0x9C, 0x9F, 0xA1, 0xA4, 0xA7, 0xAA, 
	0xAD, 0xB0, 0xB4, 0xB7, 0xBC, 0xC0, 0xC6, 0xCC, 0xD4, 0xFF
};

typedef struct {

	uint8_t dim_ref_value;

	uint8_t dim_value;

	uint8_t dim_level;

	uint8_t from_level;

	uint8_t on_state : 1;

	uint8_t wait_sync_state : 1;	

	uint8_t fault_state : 1;

	uint8_t dimmer_state;
	
} dimmer_task_parameters_t;

dimmer_task_parameters_t dimmer_params;

xQueueHandle dimmer_queue;

/* Base interrupts handlers */

ISR(INT0_vect) {
	
	timer0_stop();
	timer0_disable_isr();

	dimmer_params.wait_sync_state = 0;
	
	if (dimmer_params.on_state && !dimmer_params.fault_state) {		
		dimmer_params.dim_ref_value = dimmer_params.dim_value + timer0_get_counter();		

		if (dimmer_params.dim_level == 100) {
			dimmer_params.dim_value = 0;
			
			timer0_set_counter(0);
			timer0_start();
			dim_on();
			
			return;
		} else if (dimmer_params.dim_ref_value) {
			dimmer_params.dim_value = dimmer_params.dim_ref_value * power_map_table[dimmer_params.dim_level] / 0xff;

			timer0_set_counter(dimmer_params.dim_ref_value - dimmer_params.dim_value);
			timer0_enable_isr();
			timer0_start();

		} else {
			timer0_set_counter(0);
			timer0_start();
		}
	} else {
		dimmer_params.dim_ref_value = 0;
	}

	dim_off();
}

ISR(TIMER0_OVF_vect) {
	if (dimmer_params.on_state) {
		if (dimmer_params.wait_sync_state) {
			dim_off();
			timer0_stop();
			timer0_disable_isr();
			timer0_set_counter(0);		
			dimmer_params.dim_ref_value = 0;
		} else {
			dim_on();
			dimmer_params.wait_sync_state = 1;
		}
	}	
}

/* Task */

void v_dimmer_task(dimmer_task_parameters_t *parameters) {

	for(;;) {
        input_message_t input;

		if (xQueueReceive(input_queue, &input, 20)) {
			taskENTER_CRITICAL();

			if (parameters->on_state) {
				if (parameters->fault_state) {
					if (input.key_press_duration > 3) {
						parameters->on_state = 0;
					}	
				} else if (input.is_key_pressed) {								
					if (input.key_press_duration > 20 && 
						input.key_press_duration < 25) {
						parameters->from_level = parameters->dim_level;
					} else if (input.key_press_duration > 25) {
						uint8_t level = input.key_press_duration - 25;

						if (parameters->from_level < settings.dim_level_max) {
							if (parameters->from_level + level >= settings.dim_level_max) {
								parameters->dim_level = settings.dim_level_max;
							} else {
								parameters->dim_level = parameters->from_level + level;
							}
						} else {
							if (parameters->from_level - level <= settings.dim_level_min) {
								parameters->dim_level = settings.dim_level_min;
							} else {
								parameters->dim_level = parameters->from_level + level;
							}
						}
					}
				} else if (input.key_press_duration > 25) {
					settings.dim_level = parameters->dim_level;
					write_settings();
				} else if (input.key_press_duration > 3) {
					parameters->on_state = 0;
				}
			} else if (!input.is_key_pressed && 
				input.key_press_duration > 3 &&
				!parameters->fault_state) {
				parameters->on_state = 1;
			}

			taskEXIT_CRITICAL();
        }

        sensor_message_t sensor;

        if (xQueueReceive(sensor_queue, &sensor, 20)) {
        	taskENTER_CRITICAL();

        	if (sensor.fault_state) {
        		parameters->fault_state = 1;
        	} else if (parameters->fault_state) {
				if (sensor.temp < FAULT_TEMP_MIN) {
					parameters->fault_state = 0;
				}
			} else if (sensor.temp >= FAULT_TEMP_MAX) {
				parameters->fault_state = 1;
			}

			taskEXIT_CRITICAL();
        }

        dimmer_message_t dimmer = {
			.on_state = parameters->on_state,
			.wait_sync_state = parameters->wait_sync_state,
			.fault_state = parameters->fault_state
		};

		xQueueSend(dimmer_queue, &dimmer, 10);
    	vTaskDelay(DIMMER_DELAY_TICKS);
    }

    dim_off();
}

/* Setup */

void v_dimmer_task_setup(void) {

	dimmer_params.dim_level = settings.dim_level;
	dimmer_params.on_state = 0;
	dimmer_params.wait_sync_state = 1;
	
	dimmer_queue = xQueueCreate(5, sizeof(dimmer_message_t));
	
	xTaskCreate(v_dimmer_task, "DM", configMINIMAL_STACK_SIZE, &dimmer_params, 1, NULL);
}
