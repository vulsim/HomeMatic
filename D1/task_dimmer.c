
#include "task_dimmer.h"

#include "FreeRTOS.h"
#include "task.h"
#include "hardware.h"
#include <avr/io.h>

typedef struct {
	
	uint16_t prev_tick_count;
	
	uint8_t fsm_state;
	uint32_t fsm_time_us;
	uint32_t fsm_timeout_us;
	
	uint32_t a_start_offset_us;
	uint32_t b_start_offset_us;	
	uint32_t ac_gtg_duration_us;
	
	shared_context_t *shared_context;

} task_dimmer_parameters_t;

task_dimmer_parameters_t task_dimmer_parameters;

/* Utilities */

uint16_t get_time_delta_us(TickType_t tick_count, TickType_t prev_tick_count) {
	if (prev_tick_count > tick_count) {
		return (tick_count + (MAX_TICKS_COUNT - prev_tick_count)) * TICK_SIZE_US;
	} else {
		return (tick_count - prev_tick_count) * TICK_SIZE_US;
	}
}

uint32_t calc_start_offset(uint8_t level, uint32_t ac_duration) {
	return 0;
}

void set_dimmer_start_offset(task_dimmer_parameters_t *parameters) {
	switch (parameters->shared_context->dima_state) {
		case 1:
			parameters->a_start_offset_us = calc_start_offset(parameters->shared_context->dima_level1, parameters->ac_gtg_duration_us);
			break;
		case 2:
			parameters->a_start_offset_us = calc_start_offset(parameters->shared_context->dima_level2, parameters->ac_gtg_duration_us);
			break;
	}
	
	switch (parameters->shared_context->dimb_state) {
		case 1:
			parameters->b_start_offset_us = calc_start_offset(parameters->shared_context->dimb_level1, parameters->ac_gtg_duration_us);
			break;
		case 2:
			parameters->b_start_offset_us = calc_start_offset(parameters->shared_context->dimb_level2, parameters->ac_gtg_duration_us);
			break;
	}
}

void set_fsm_state(task_dimmer_parameters_t *parameters, uint8_t state) {
	
	parameters->fsm_state = state;
	parameters->fsm_time_us = 0;
	
	switch (state) {
		case 0:
			parameters->fsm_timeout_us = DEFAULT_RESPONSE_US;
			break;
			
		case 1:
			parameters->fsm_timeout_us = AC_GATE_THRESHOLD_US;
			break;
			
		case 2:
			parameters->fsm_timeout_us = AC_GATE_THRESHOLD_US;
			break;
			
		case 3:
			parameters->fsm_timeout_us = DEFAULT_RESPONSE_US;
			break;
			
		case 4:
			parameters->fsm_timeout_us = POWER_ON_RESPONSE_US;
			break;
			
		case 5:
			parameters->fsm_timeout_us = AC_GATE_THRESHOLD_US;
			break;
			
		case 6:
			parameters->fsm_timeout_us = parameters->ac_gtg_duration_us - AC_GATE_MIN_US;
			break;
			
		case 7:
			parameters->fsm_timeout_us = AC_GATE_MIN_US * 2;
			break;
			
		case 251:
			parameters->fsm_timeout_us = DEFAULT_RESPONSE_US;
			break;
			
		case 252:
			parameters->fsm_timeout_us = DEFAULT_RESPONSE_US;
			break;
			
		case 253:
			parameters->fsm_timeout_us = DIAG_NO_POWER_TIMEOUT_US;
			break;
			
		case 254:
			parameters->fsm_timeout_us = DEFAULT_RESPONSE_US;
			break;
			
		case 255:
			parameters->fsm_timeout_us = DIAG_NO_AC_GATE_TIMEOUT_US;
			break;
	}
}

/* Task */

void v_task_dimmer(task_dimmer_parameters_t *parameters) {

	for(;;) {
	
		if (!parameters->fsm_timeout_us) {
			parameters->shared_context->diag.malfunction = 1;
		}
		
		uint16_t tick_count = xTaskGetTickCount();		
		uint16_t time_delta_us = get_time_delta_us(tick_count, parameters->prev_tick_count);
		
		if (parameters->fsm_timeout_us > time_delta_us) {
			parameters->fsm_timeout_us -= time_delta_us;
		} else {
			parameters->fsm_timeout_us = 0;
		}
		
		parameters->prev_tick_count = tick_count;
		parameters->fsm_time_us += time_delta_us; 
		
		if (parameters->shared_context->diag.malfunction) {
			break;
		}
	
		switch (parameters->fsm_state) {
		
			// Init 0: Reset to initial state
			case 0: 
				if (parameters->shared_context->diag.no_power_feedback) {
					set_fsm_state(parameters, 252);
				} else if (parameters->shared_context->diag.no_ac_power) {
					set_fsm_state(parameters, 254);					
				} else {
					set_fsm_state(parameters, 1);
				}
				break;
				
			// Init/Diag 1: Waiting for first AC gate
			case 1: 
				if (parameters->fsm_timeout_us) {
					if (is_ac_gate()) {
						parameters->ac_gtg_duration_us = 0;
						parameters->shared_context->diag.no_ac_power = 0;
						set_fsm_state(parameters, 2);						
					}
				} else {
					set_fsm_state(parameters, 254);
				}
				break;
				
			// Init 2: Waiting for second AC gate
			case 2:
				if (parameters->fsm_timeout_us) {
					if (is_ac_gate()) {
						parameters->ac_gtg_duration_us = parameters->fsm_time_us;
						set_fsm_state(parameters, 3);
					}					
				} else {
					set_fsm_state(parameters, 254);
				}
				break;
			
			// Operation 0: Check for switch on signals
			case 3:
				if (parameters->fsm_timeout_us && 
					(parameters->shared_context->dima_state || 
					parameters->shared_context->dimb_state)) {
					set_dimmer_output(1, 0, 0);
					set_fsm_state(parameters, 4);
				} else {					
					set_dimmer_output(0, 0 ,0);
					set_fsm_state(parameters, 1);
				}
				break;
			
			// Init/Diag 3: Waiting for power on
			case 4: 
				if (parameters->fsm_timeout_us) {
					if (is_power_on()) {
						parameters->shared_context->diag.no_power_feedback = 0;
						set_fsm_state(parameters, 5);
					}
				} else {					
					set_fsm_state(parameters, 252);
				}
				break;
				
			// Operation 2: Waiting for AC gate
			case 5: 
				if (parameters->fsm_timeout_us && parameters->ac_gtg_duration_us) {
					if (is_ac_gate()) {						
						set_dimmer_start_offset(parameters);
						set_fsm_state(parameters, 6);						
					}
				} else {
					set_fsm_state(parameters, 254);
				}
				break;
				
			// Operation 3: Dimming #1
			case 6: 
				if (parameters->fsm_timeout_us) {
					uint8_t a_on = 0;
					uint8_t b_on = 0;
					
					if (parameters->shared_context->dima_state &&
						parameters->fsm_timeout_us > parameters->a_start_offset_us) {
						a_on = 1;
					}
					
					if (parameters->shared_context->dimb_state &&
						parameters->fsm_timeout_us > parameters->b_start_offset_us) {
						b_on = 1;
					}
					
					set_dimmer_output(1, a_on, b_on);
				} else {					
					parameters->ac_gtg_duration_us = parameters->fsm_time_us;
					set_dimmer_output(1, 0, 0);
					set_fsm_state(parameters, 7);
				}
				break;
				
			// Operation 4: Dimming #2
			case 7: 
				if (parameters->fsm_timeout_us) {					
					if (!parameters->shared_context->dima_state && 
						!parameters->shared_context->dimb_state) {						
						set_fsm_state(parameters, 251);
					} else if (!is_power_on()) {
						set_fsm_state(parameters, 252);
					} else if (is_ac_gate()) {
						parameters->ac_gtg_duration_us += parameters->fsm_time_us;
						set_dimmer_start_offset(parameters);
						set_fsm_state(parameters, 6);
					}					
				} else {
					set_fsm_state(parameters, 254);
				}
				break;
				
			// Operation 5: Pending off
			case 251:
				set_dimmer_output(0, 0, 0);
				set_fsm_state(parameters, 1);
				break;
				
			// Idle: Relay not switched on due to hardware problems or overheating
			case 252: 
				parameters->shared_context->diag.no_power_feedback = 1;
				set_dimmer_output(0, 0, 0);
				set_fsm_state(parameters, 253);
				break;
				
			case 253:
				if (!parameters->fsm_timeout_us) {
					set_fsm_state(parameters, 1);
				}
				break;
				
			// Idle: No gate detected
			case 254: 
				parameters->shared_context->diag.no_ac_power = 1;
				set_dimmer_output(0, 0, 0);
				set_fsm_state(parameters, 255);
				break;
				
			case 255:
				if (!parameters->fsm_timeout_us) {
					set_fsm_state(parameters, 1);
				}
				break;
		}
		
        vTaskDelay(5);
    }

	set_dimmer_output(0, 0, 0);
    vTaskDelete(NULL);
}

/* Setup */

void v_setup_task_dimmer(shared_context_t *shared_context) {

	task_dimmer_parameters.shared_context = shared_context;
	
	set_fsm_state(&task_dimmer_parameters, 0);
	
	xTaskCreate(v_task_dimmer, "DA", configMINIMAL_STACK_SIZE, &task_dimmer_parameters, 1, NULL);
}
