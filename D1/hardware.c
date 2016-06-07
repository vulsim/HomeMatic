
#include "hardware.h"
#include <avr/eeprom.h>

settings_t settings;

settings_t ee_settings EEMEM = {

	.soft_on = 1,	
	
	.t_soft_on = 0x96,
	
	.dim_level_min = 0x0d,

	.dim_level_max = 0xff,

	.dim_level = 0x4d
};

uint8_t is_key_pressed(void) {

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

	read_settings();
}

