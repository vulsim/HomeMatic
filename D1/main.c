/*
    HomeMatic D1/1 V1.1.0 (hw:v1.0) - Copyright (C) 2016 vulsim. All rights reserved	
*/

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "hardware.h"
		
#define MAX_INT					0xFFFF
#define SYNC_LOSS_DURATION		(0xFFFF - 938)	// 1.5 of sync cycle (~15ms)
#define MS_10					(0xFF - 156)	// 10 ms	
#define US_750                  (0xFFFF - 47)	// ~750 us
#define BLINK_DURATION			50				// 0.5 sec
#define MIN_KEY_PRESS_THRESHOLD	5				// 50 ms
#define SWEEP_START_THRESHOLD 	50				// 0.5 sec
#define STEP_THRESHOLD			5				// 100% -> 5 sec
#define DIM_LEVEL_MIN			10
#define DIM_LEVEL_MAX			99
#define OVERHEAT_MAX_TEMP		85
#define OVERHEAT_MIN_TEMP		10
#define TEMP_MEASURE_END		10				// 100 ms
#define TEMP_MEASURE_INTERVAL	3000			// 30 sec

/* INFO: Calculated for clk/256 timer and 10 ms duration (100 Hz) */

const uint16_t dim_table[100] = {
	0xFD8E, 0xFDB9, 0xFDDD, 0xFDEE, 0xFDFB, 0xFE06, 0xFE0F, 0xFE17, 0xFE1F, 0xFE26,
	0xFE2C, 0xFE32, 0xFE38, 0xFE3D, 0xFE42, 0xFE47, 0xFE4C, 0xFE51, 0xFE55, 0xFE59,
	0xFE5E, 0xFE62, 0xFE66, 0xFE6A, 0xFE6E, 0xFE71, 0xFE75, 0xFE79, 0xFE7D, 0xFE80,
	0xFE83, 0xFE87, 0xFE8A, 0xFE8E, 0xFE91, 0xFE95, 0xFE98, 0xFE9B, 0xFE9F, 0xFEA2,
	0xFEA5, 0xFEA8, 0xFEAB, 0xFEAE, 0xFEB2, 0xFEB5, 0xFEB8, 0xFEBB, 0xFEBE, 0xFEC1,
	0xFEC5, 0xFEC8, 0xFECB, 0xFECE, 0xFED1, 0xFED4, 0xFED7, 0xFEDA, 0xFEDD, 0xFEE1,
	0xFEE4, 0xFEE7, 0xFEEA, 0xFEED, 0xFEF1, 0xFEF4, 0xFEF7, 0xFEFA, 0xFEFE, 0xFF01,
	0xFF05, 0xFF08, 0xFF0C, 0xFF0F, 0xFF13, 0xFF17, 0xFF1B, 0xFF1E, 0xFF22, 0xFF26,
	0xFF2A, 0xFF2E, 0xFF32, 0xFF37, 0xFF3B, 0xFF40, 0xFF44, 0xFF4A, 0xFF4F, 0xFF54,
	0xFF59, 0xFF60, 0xFF66, 0xFF6C, 0xFF73, 0xFF7B, 0xFF85, 0xFF90, 0xFF9C, 0xFFAC
};

typedef enum {
	STATE_OFF = 0,
	STATE_OVERHEAT_OFF = 1,
	STATE_OVERHEAT_ON = 2,
	STATE_HALF_ON = 3,
	STATE_ON = 4,
	STATE_SWEEP_UP = 5,
	STATE_SWEEP_DOWN = 6
} state_t;

typedef enum {
	WAIT_SYNC = 0,
	IDLE = 1,
	ON = 2,
	OFF = 3	
} sync_t;

int16_t temp;
uint8_t blink_on;
uint8_t step_counter;
uint16_t blink_counter;
uint16_t key_press_counter;
uint16_t temp_measure_counter;

state_t state;
sync_t sync_state;

/* Interrupts handlers */

ISR(INT0_vect) {	
	dim_off();
	timer1_stop();

	sync_state = IDLE;

	if (state >= STATE_HALF_ON) {		
		timer1_set_counter(dim_table[settings.dim_level]);
		timer1_start();		
	}
}

ISR(TIMER1_OVF_vect) {	
	timer1_stop();
	
	switch (sync_state) {
		case IDLE:
			sync_state = ON;
			dim_on();
			timer1_set_counter(US_750);
			timer1_start();
			break;
		
		case ON:
			sync_state = OFF;
			dim_off();
			timer1_set_counter(SYNC_LOSS_DURATION);
			timer1_start();
			break;
			
		case OFF:
		case WAIT_SYNC:
			dim_off();
			sync_state = WAIT_SYNC;
			break;
	}
}

/* 1 cycle ~= 10 ms */

ISR(TIMER0_OVF_vect) {

	timer0_set_counter(MS_10);
		
	if (is_key_pressed() && key_press_counter < MAX_INT) {
		key_press_counter++;
	}

	if (blink_counter < BLINK_DURATION) {
		blink_counter++;
	} else {
		blink_counter = 0;
		blink_on = (blink_on) ? 0 : 1;
	}
	
	if ((state == STATE_SWEEP_UP || state == STATE_SWEEP_DOWN) && step_counter < STEP_THRESHOLD) {
		step_counter++;	
	}

	if (temp_measure_counter < TEMP_MEASURE_INTERVAL) {
		temp_measure_counter++;

		if (temp_measure_counter == TEMP_MEASURE_END) {
			temp = sensor_read_temp();
		}
	} else if (sensor_measure_temp() == SENSOR_OK) {
		temp_measure_counter = 0;
	} else {
		temp_measure_counter = TEMP_MEASURE_END;
		temp = SENSOR_TEMP_WRONG;
	}
}

/* FSM */

void process_state(uint8_t key_pressed, uint16_t key_press_counter) {
	switch (state) {
		case STATE_OFF: {
			if (key_pressed && key_press_counter > MIN_KEY_PRESS_THRESHOLD) {
				blink_counter = 0;
				blink_on = 0;
				sync_state = WAIT_SYNC;
				state = STATE_HALF_ON;
			}
			break;
		}

		case STATE_OVERHEAT_OFF: {
			if (!key_pressed && temp < settings.overheat_release_temp && temp != SENSOR_TEMP_WRONG) {
				state = STATE_OFF;
			}
			break;
		}

		case STATE_OVERHEAT_ON: {
			if (!key_pressed && key_press_counter > MIN_KEY_PRESS_THRESHOLD) {
				state = STATE_OVERHEAT_OFF;				
			} else if (!key_pressed && temp < settings.overheat_release_temp && temp != SENSOR_TEMP_WRONG) {
				state = STATE_ON;
			}
			break;
		}
		
		case STATE_HALF_ON: {
			if (temp > settings.overheat_threshold_temp  || temp == SENSOR_TEMP_WRONG) {
				state = STATE_OVERHEAT_OFF;
			} else if (!key_pressed) {
				state = STATE_ON;
			}
			break;
		}

		case STATE_ON: {
			if (temp > settings.overheat_threshold_temp || temp == SENSOR_TEMP_WRONG) {
				state = STATE_OVERHEAT_ON;
			} else if (key_pressed) {
				if (key_press_counter > SWEEP_START_THRESHOLD) {
					step_counter = 0;

					if (settings.dim_level > (settings.dim_level_min + settings.dim_level_max) / 2) {
						state =  STATE_SWEEP_DOWN;
					} else {
						state = STATE_SWEEP_UP;
					}					
				}
			} else if (key_press_counter > MIN_KEY_PRESS_THRESHOLD && key_press_counter < SWEEP_START_THRESHOLD) {
				state = STATE_OFF;
				
				settings_t ee_settings;
				read_settings(&ee_settings); 
				
				if (settings.dim_level != ee_settings.dim_level) {
					write_settings();
					rled_on();
					gled_off();
					_delay_ms(BLINK_DURATION * 10);              
				}
			}
			break;
		}

		case STATE_SWEEP_UP: {
			if (temp > settings.overheat_threshold_temp || temp == SENSOR_TEMP_WRONG) {
				state = STATE_OVERHEAT_ON;
			} else if (key_pressed) {
				if (step_counter == STEP_THRESHOLD && settings.dim_level < settings.dim_level_max) {
					settings.dim_level += 1;
					step_counter = 0;
				} 
			} else {
				state = STATE_ON;
			}
			break;
		}

		case STATE_SWEEP_DOWN: {
			if (temp > settings.overheat_threshold_temp || temp == SENSOR_TEMP_WRONG) {
				state = STATE_OVERHEAT_ON;
			} else if (key_pressed) {
				if (step_counter == STEP_THRESHOLD && settings.dim_level > settings.dim_level_min) {
					settings.dim_level -= 1;
					step_counter = 0;
				}
			} else {
				state = STATE_ON;
			}
			break;
		}		
	}
}

/* Display state */

void process_display(void) {
	if (sync_state == WAIT_SYNC) {
		if (blink_on) {
			rled_on();
			gled_on();
		} else {
			rled_off();
			gled_off();
		}
		return;
	} 

	switch (state) {
		case STATE_OFF: {
			rled_off();
			gled_off();
			break;
		}

		case STATE_OVERHEAT_OFF: {			
			if (blink_on) {
				rled_on();			
			} else {
				rled_off();
			}
			gled_off();
			break;			
		}

		case STATE_OVERHEAT_ON: {
			if (blink_on) {
				rled_on();
				gled_off();
			} else {
				rled_off();
				gled_on();
			}
			break;
		}

		case STATE_HALF_ON:
		case STATE_ON: {
			rled_off();
			gled_on();
			break;
		}

		case STATE_SWEEP_UP:			
		case STATE_SWEEP_DOWN: {
			rled_on();
			gled_on();
			break;
		}		
	}
}

void validate_settings(void) {
	if (settings.dim_level_max > DIM_LEVEL_MAX) {
		settings.dim_level_max = DIM_LEVEL_MAX;
	} else if (settings.dim_level_max < DIM_LEVEL_MIN + 1) {
		settings.dim_level_max = DIM_LEVEL_MIN + 1;
	}

	if (settings.dim_level_min < DIM_LEVEL_MIN) {
		settings.dim_level_min = DIM_LEVEL_MIN;
	} else if (settings.dim_level_min >= settings.dim_level_max) {
		settings.dim_level_min = settings.dim_level_max - 1;
	}

	if (settings.overheat_threshold_temp > OVERHEAT_MAX_TEMP) {
		settings.overheat_threshold_temp = OVERHEAT_MAX_TEMP;
	} else if (settings.overheat_threshold_temp < OVERHEAT_MIN_TEMP) {
		settings.overheat_threshold_temp = settings.OVERHEAT_MAX_TEMP;
	}

	if (settings.overheat_release_temp > settings.overheat_threshold_temp) {
		settings.overheat_release_temp = settings.overheat_threshold_temp;
	} else if (settings.overheat_release_temp < OVERHEAT_MIN_TEMP) {
		settings.overheat_release_temp = OVERHEAT_MIN_TEMP;
	}
}

/* Main */

int main(void) {

	cli();

	if (hardware_setup()) {
		read_settings(NULL);
		validate_settings();
						
		blink_on = 0;
		blink_counter = 0;
		step_counter = 0;
		key_press_counter = 0;
		temp_measure_counter = 1;

		sync_state = IDLE;		
		state = WAIT_SYNC;

		if (sensor_measure_temp() == SENSOR_OK) {
			_delay_ms(100);
			temp = sensor_read_temp();			
		} else {
			temp = SENSOR_TEMP_WRONG;
		}

		sei();
		timer0_set_counter(MS_10);
		timer0_start();
		
		for (;;) {
			uint8_t key_pressed = is_key_pressed();

			process_state(key_pressed, key_press_counter);
			process_display();

			if (!key_pressed) {
				timer0_disable_isr();
				key_press_counter = 0;
				timer0_enable_isr();
			}
		}
	}

	rled_on();
	gled_off();

	return 0;
}
