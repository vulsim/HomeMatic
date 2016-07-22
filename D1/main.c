/*
    HomeMatic D1/2 V1.0.0 (hw:v1.0) - Copyright (C) 2016 vulsim. All rights reserved	
*/

#include <avr/interrupt.h>

#include "task.h"
#include "hardware.h"
#include "dimmer.h"
#include "display.h"
#include "input.h"
#include "sensor.h"

#define MAX_INT					0xFFFF
#define MAX_SYNC_LOSS_DURATION	(MAX_INT - 938) // 1.5 of sync cycle (~15ms)
#define ONE_MS 					(266 - 63)
#define MIN_DIM_VALUE			5
#define MAX_DIM_VALUE			100
#define MIN_KEY_PRESS_THRESHOLD	100	// 100 ms
#define SLIDE_START_THRESHOLD 	1000 // 1000 ms
#define SLIDE_PERCENT_LENGTH	53	// 53 ms for 1% or 5000 ms for full scale (95%)


/* INFO: Calculated for clk/256 timer and 10 ms duration (100 Hz) */
const uint16_t dim_table[100] PROGMEM ={
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

uint8_t wait_sync : 1;
uint8_t within_dt : 1;

uint8_t on_state : 1;
uint8_t slide_up : 1;
uint8_t slide_down : 1;
uint8_t overheat : 1;

uint8_t dimmer_level;
uint8_t dimmer_ref_level;

uint8_t fast_blink_on : 1;
uint8_t blink_on : 1;

uint8_t fast_blink_counter;
uint16_t blink_counter;


uint16_t key_press_counter;

/* Interrupts handlers */

ISR(INT0_vect) {
	
	timer1_stop();

	wait_sync = 0;
	within_dt = 0;

	dimmer_state_t dimmer_state = get_dimmer_state();

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
		timer1_set_counter(MAX_SYNC_LOSS_DURATION);
		timer1_start();
	} else {
		wait_sync = 1;
		dim_off();
	}
}

ISR(TIMER0_OVF_vect) {

	/* 1 overflow cycle ~= 1 ms */
	timer0_set_counter(ONE_MS)
	
	if (is_key_pressed() && key_press_duration < MAX_INT) {
		key_press_duration++;
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

void reset_blink() {
	timer0_stop();
	blink_on = 1;
	blink_counter = 0;
	fast_blink_on = 1;
	fast_blink_counter = 0;
	timer0_set_counter(ONE_MS);	
	timer0_sart();
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
							key_press_duration > MIN_KEY_PRESS_THRESHOLD / 2 && 
							key_press_duration < MIN_KEY_PRESS_THRESHOLD) {

							dimmer_ref_level = dimmer_level;

							if (dimmer_ref_level > MAX_DIM_VALUE / 2) {
								slide_down = 1;
							} else {
								slide_up = 1;
							}
						} else if ((slide_up || slide_down) && key_press_duration > SLIDE_START_THRESHOLD) {
							uint16_t delta_value = (key_press_duration - SLIDE_START_THRESHOLD) / SLIDE_PERCENT_LENGTH; 

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
					if (key_press_duration > MIN_KEY_PRESS_THRESHOLD && 
						key_press_duration < SLIDE_START_THRESHOLD) {
						on_state = 0;
						reset_blink();
					} else if ((slide_up || slide_down) && 
						key_press_duration > SLIDE_START_THRESHOLD) {						
						settings.dim_level = dimmer_level;
						write_settings();						
					}

					slide_up = 0;
					slide_down = 0;					
					key_press_duration = 0;
				}
			} else if (is_key_pressed()) {
				if (key_press_duration > MIN_KEY_PRESS_THRESHOLD) {
					on_state = 1;
					slide_up = 0;
					slide_down = 0;	
					light_on = 1;
					blink_counter = 0;
				}
			} else {
				slide_up = 0;
				slide_down = 0;	
				key_press_duration = 0;
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
