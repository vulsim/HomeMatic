/*
    HomeMatic D1/1 V1.1.0 (hw:v1.0) - Copyright (C) 2016 vulsim. All rights reserved	
*/

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "hardware.h"

#define MAX_INT					0xFFFF
#define SYNC_LOSS_DURATION		(MAX_INT - 938) // 1.5 of sync cycle (~15ms)
#define ONE_MS 					(266 - 63)
#define MIN_DIM_VALUE			5
#define MAX_DIM_VALUE			100
#define MIN_KEY_PRESS_THRESHOLD	100				// 100 ms
#define SLIDE_START_THRESHOLD 	1000			// 1000 ms
#define SLIDE_PERCENT_LENGTH	53				// 53 ms for 1% or 5000 ms for full scale (95%)


/* INFO: Calculated for clk/256 timer and 10 ms duration (100 Hz) */
const uint16_t dim_table[100] PROGMEM ={
	0xFD96, 0xFDA1, 0xFDA9, 0xFDB0, 0xFDB7, 0xFDBD, 0xFDC3, 0xFDC8, 0xFDCD, 0xFDD2,
	0xFDD7, 0xFDDC, 0xFDE1, 0xFDE5, 0xFDEA, 0xFDEE, 0xFDF2, 0xFDF7, 0xFDFB, 0xFDFF,
	0xFE03, 0xFE07, 0xFE0B, 0xFE0F, 0xFE13, 0xFE17, 0xFE1B, 0xFE1E, 0xFE22, 0xFE26,
	0xFE2A, 0xFE2E, 0xFE31, 0xFE35, 0xFE39, 0xFE3C, 0xFE40, 0xFE44, 0xFE48, 0xFE4B,
	0xFE4F, 0xFE53, 0xFE56, 0xFE5A, 0xFE5E, 0xFE61, 0xFE65, 0xFE69, 0xFE6C, 0xFE70,
	0xFE74, 0xFE78, 0xFE7B, 0xFE7F, 0xFE83, 0xFE87, 0xFE8A, 0xFE8E, 0xFE92, 0xFE96,
	0xFE9A, 0xFE9E, 0xFEA2, 0xFEA6, 0xFEAA, 0xFEAE, 0xFEB2, 0xFEB7, 0xFEBB, 0xFEBF,
	0xFEC4, 0xFEC8, 0xFECC, 0xFED1, 0xFED6, 0xFEDA, 0xFEDF, 0xFEE4, 0xFEE9, 0xFEEE,
	0xFEF4, 0xFEF9, 0xFEFE, 0xFF04, 0xFF0A, 0xFF10, 0xFF16, 0xFF1D, 0xFF24, 0xFF2B,
	0xFF33, 0xFF3B, 0xFF43, 0xFF4D, 0xFF57, 0xFF62, 0xFF6F, 0xFF7F, 0xFF93, 0xFFB4
};

uint8_t wait_sync;
uint8_t within_dt;
uint8_t on_state;
uint8_t slide_up;
uint8_t slide_down;
uint8_t overheat;
uint8_t dimmer_level;
uint8_t dimmer_ref_level;
uint8_t fast_blink_on;
uint8_t blink_on;
uint8_t fast_blink_counter;
uint16_t blink_counter;
uint16_t key_press_counter;

/* Interrupts handlers */

ISR(INT0_vect) {
	
	timer1_stop();

	wait_sync = 0;
	within_dt = 0;

	if (on_state && !overheat) {
		if (dimmer_level == MAX_DIM_VALUE) {
			dim_on();
			return;
		} else {
			within_dt = 1;
			timer1_set_counter(dim_table[dimmer_level]);
			timer1_start();
		}		
	}

	dim_off();
}

ISR(TIMER1_OVF_vect) {
	
	timer1_stop();
	
	if (within_dt) {
		within_dt = 0;
		dim_on();
		timer1_set_counter(SYNC_LOSS_DURATION);
		timer1_start();
	} else {
		wait_sync = 1;
		dim_off();
	}
}

ISR(TIMER0_OVF_vect) {

	/* 1 overflow cycle ~= 1 ms */
	timer0_set_counter(ONE_MS);
	
	if (is_key_pressed() && key_press_counter < MAX_INT) {
		key_press_counter++;
	}

	if (fast_blink_counter < 50) {
		fast_blink_counter++;
	} else {
		fast_blink_counter = 0;
		fast_blink_on = (fast_blink_on) ? 0 : 1;
	}

	if (blink_counter < 1000) {
		blink_counter++;
	} else {
		blink_counter = 0;
		blink_on = (blink_on) ? 0 : 1;
	}
}

/* Utils */

void reset_blink(void) {
	timer0_stop();
	blink_on = 1;
	blink_counter = 0;
	fast_blink_on = 1;
	fast_blink_counter = 0;
	timer0_set_counter(ONE_MS);	
	timer0_start();
}

/* Main */

int main(void) {

	cli();

	if (hardware_setup()) {
		read_settings();
		
		wait_sync = 1;
		within_dt = 0;
		
		on_state = 0;	
		overheat = 0;
		slide_up = 0;
		slide_down = 0;
		dimmer_level = settings.dim_level;	

		blink_on = 1;	
		blink_counter = 0;
		fast_blink_on = 1;
		fast_blink_counter = 0;
		
		key_press_counter = 0;

		sei();
		timer0_start();
		
		for (;;) {
			if (on_state) {
				if (is_key_pressed()) {
					if (!overheat) {
						if (!slide_up && !slide_down && 
							key_press_counter > MIN_KEY_PRESS_THRESHOLD / 2 && 
							key_press_counter < MIN_KEY_PRESS_THRESHOLD) {

							dimmer_ref_level = dimmer_level;

							if (dimmer_ref_level > MAX_DIM_VALUE / 2) {
								slide_down = 1;
							} else {
								slide_up = 1;
							}
						} else if ((slide_up || slide_down) && key_press_counter > SLIDE_START_THRESHOLD) {
							uint16_t delta_value = (key_press_counter - SLIDE_START_THRESHOLD) / SLIDE_PERCENT_LENGTH; 

							if (slide_up) {
								if (delta_value > MAX_DIM_VALUE - dimmer_ref_level) {
									dimmer_level = MAX_DIM_VALUE;
								} else {
									dimmer_level = dimmer_ref_level + delta_value; 
								}
							} else if (slide_down) {
								if (delta_value > dimmer_ref_level - MIN_DIM_VALUE) {
									dimmer_level = MIN_DIM_VALUE;
								} else {
									dimmer_level = dimmer_ref_level - delta_value; 
								}
							}
						}
					}
				} else {
					if (key_press_counter > MIN_KEY_PRESS_THRESHOLD && 
						key_press_counter < SLIDE_START_THRESHOLD) {
						on_state = 0;
						reset_blink();
					} else if ((slide_up || slide_down) && 
						key_press_counter > SLIDE_START_THRESHOLD) {						
						settings.dim_level = dimmer_level;
						write_settings();						
					}

					slide_up = 0;
					slide_down = 0;					
					key_press_counter = 0;
				}
			} else if (is_key_pressed()) {
				if (key_press_counter > MIN_KEY_PRESS_THRESHOLD) {
					on_state = 1;
					slide_up = 0;
					slide_down = 0;	
					reset_blink();
				}
			} else {
				slide_up = 0;
				slide_down = 0;	
				key_press_counter = 0;
			}

			if (on_state) {
				if (overheat) {
					if (blink_on) {
						rled_on();
						gled_off();
					} else {
						rled_off();
						gled_off();
					}
				} else if (wait_sync) {
					if (blink_on) {
						rled_on();
						gled_on();
					} else {
						rled_on();
						gled_on();
					}
				} else {					
					gled_on();

					if ((slide_up || slide_down) && fast_blink_on) {
						rled_on();
					} else {
						rled_off();
					}
				}
			}
		}
	}

	rled_on();

	return 0;
}
