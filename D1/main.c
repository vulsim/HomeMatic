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
	0xFD8E, 0xFD8E, 0xFD8F, 0xFD8F, 0xFD91, 0xFD92, 0xFD93, 0xFD95, 0xFD97, 0xFD99,
	0xFD9B, 0xFD9D, 0xFDA0, 0xFDA2, 0xFDA5, 0xFDA8, 0xFDAB, 0xFDAE, 0xFDB1, 0xFDB4,
	0xFDB7, 0xFDBA, 0xFDBD, 0xFDC0, 0xFDC4, 0xFDC7, 0xFDCA, 0xFDCE, 0xFDD1, 0xFDD4,
	0xFDD8, 0xFDDB, 0xFDDF, 0xFDE2, 0xFDE6, 0xFDE9, 0xFDEC, 0xFDF0, 0xFDF3, 0xFDF7,
	0xFDFA, 0xFDFE, 0xFE01, 0xFE05, 0xFE08, 0xFE0C, 0xFE0F, 0xFE13, 0xFE17, 0xFE1A,
	0xFE1E, 0xFE22, 0xFE26, 0xFE29, 0xFE2D, 0xFE31, 0xFE36, 0xFE3A, 0xFE3E, 0xFE43,
	0xFE47, 0xFE4C, 0xFE51, 0xFE56, 0xFE5B, 0xFE61, 0xFE66, 0xFE6C, 0xFE72, 0xFE79,
	0xFE7F, 0xFE86, 0xFE8D, 0xFE95, 0xFE9C, 0xFEA4, 0xFEAD, 0xFEB6, 0xFEBF, 0xFEC8,
	0xFED3, 0xFEDD, 0xFEE8, 0xFEF3, 0xFEFF, 0xFF0C, 0xFF19, 0xFF26, 0xFF34, 0xFF43,
	0xFF53, 0xFF63, 0xFF73, 0xFF85, 0xFF97, 0xFFAA, 0xFFBE, 0xFFD2, 0xFFE8, 0xFFFE
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
		settings.overheat_threshold_temp = OVERHEAT_MIN_TEMP;
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
