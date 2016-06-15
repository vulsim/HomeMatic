
#include "hardware.h"
#include <avr/eeprom.h>
#include <avr/io.h>

settings_t settings;

settings_t ee_settings EEMEM = {

	.soft_on = 1,	
	
	.t_soft_on = 0x96,
	
	.dim_level_min = 0x0d,

	.dim_level_max = 0xff,

	.dim_level = 0x4d
};

/* Internal */


/* Hardware interaction */

void timer0_enable_isr(void) {
	TIMSK0 |= 1 << TOIE0;
}

void timer0_disable_isr() {
	TIMSK0 &= 0xff ^ (1<<TOIE0);
}

void timer0_set_counter(uint8_t value) {
	TCNT0 = value;
}

uint8_t timer0_get_counter(void) {
	return TCNT0;
}

uint8_t is_key_pressed(void) {
	return 0;
}

void timer0_start(void) {
    TCCR0B = (1<<CS02) | (1<<CS00);
}

void timer0_stop(void) {
	TCCR0B = 0;
}

void dim_on(void) {

}

void dim_off(void) {

}

void rled_on(void) {

}

void rled_off(void) {

}

void gled_on(void) {

}

void gled_off(void) {
	
}

void read_settings(void) {
	
	eeprom_read_block(&settings, &ee_settings, sizeof(settings_t));
}

void write_settings(void) {
	
	eeprom_write_block(&settings, &ee_settings, sizeof(settings_t));
}

void v_hardware_setup(void) {

	TCCR0A = 0;
	TCCR0B = 0;

	read_settings();

	timer0_stop();
	timer0_disable_isr();
	timer0_set_counter(0);
}

