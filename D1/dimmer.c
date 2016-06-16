
#include "dimmer.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"

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

	uint8_t dim_percent_value;

	uint8_t on_state : 2;

	uint8_t err_state : 1;
	
} dimmer_task_parameters_t;

dimmer_task_parameters_t dimmer_task_parameters;

#define params dimmer_task_parameters

/* Internal */


/* Base interrupts handlers */

ISR(INT0_vect) {
	
	timer0_stop();
	timer0_disable_isr();
	
	if (params.on_state && !params.err_state) {
		params.on_state = 1;
		params.dim_ref_value = params.dim_value + timer0_get_counter();		

		if (params.dim_percent_value == 100) {
			params.dim_value = 0;
			
			timer0_set_counter(0);
			timer0_start();
			dim_on();
			
			return;
		} else if (params.dim_ref_value) {
			params.dim_value = params.dim_ref_value * power_map_table[params.dim_percent_value] / 0xff;

			timer0_set_counter(params.dim_ref_value - params.dim_value);
			timer0_enable_isr();
			timer0_start();

		} else {
			timer0_set_counter(0);
			timer0_start();
		}
	}

	dim_off();
}

ISR(TIMER0_OVF_vect) {

	//timer0_set_counter(0); 

	if (params.on_state == 1) {
		dim_on();
		params.on_state = 2;
	} else if (params.on_state == 2) {
		dim_off();
		timer0_stop();
		timer0_disable_isr();
		timer0_set_counter(0);		

		params.dim_ref_value = 0;
	}	
}

/* Task */

void v_dimmer_task(dimmer_task_parameters_t *parameters) {

	for(;;) {
        vTaskDelay(5);
    }

    dim_off();
    vTaskDelete(NULL);
}

/* Setup */

void v_dimmer_task_setup(void) {
	
	xTaskCreate(v_dimmer_task, "DM", configMINIMAL_STACK_SIZE, &dimmer_task_parameters, 1, NULL);
}
