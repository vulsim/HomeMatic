
#include "hardware.h"
#include "onewire.h"
#include <avr/eeprom.h>
#include <avr/io.h>

settings_t settings;

settings_t ee_settings /*EEMEM*/ = {
	
	.dim_level_min = 0x05,

	.dim_level_max = 0x64,

	.dim_level = 0x32
};

/* Internal */

uint8_t search_sensors(void) {

	uint8_t id[OW_ROMCODE_SIZE];
	uint8_t diff, sensors;
	
	ow_reset();

	sensors = 0;
	
	diff = OW_SEARCH_FIRST;

	while (diff != OW_LAST_DEVICE && sensors < 5) {
		DS18X20_find_sensor(&diff, &id[0]);
		
		if (diff == OW_PRESENCE_ERR) {
			break;
		}
		
		if (diff == OW_DATA_ERR) {
			break;
		}
		
		sensors++;
	}
	
	return sensors;
}


/* Hardware interaction */

uint8_t sensor_measure_temp(void) {

	if (DS18X20_start_meas(DS18X20_POWER_EXTERN, NULL) == DS18X20_OK) {
		return 1;
	}
	
	return 0;
}

int16_t sensor_read_temp(void) {

	int16_t temp = 30000;
	
	DS18X20_read_decicelsius_single(0, &temp);

	return temp;
}

void timer0_enable_isr(void) {
	TIMSK0 |= 1<<TOIE0;
}

void timer0_disable_isr() {
	TIMSK0 &= 0xFF ^ (1<<TOIE0);
}

void timer0_set_counter(uint8_t value) {
	TCNT0 = value;
}

uint8_t timer0_get_counter(void) {
	return TCNT0;
}

uint8_t is_key_pressed(void) {
	return !(PIND & (1<<PD6));
}

void timer0_start(void) {
    TCCR0B = (1<<CS02) | (1<<CS00);
}

void timer0_stop(void) {
	TCCR0B = 0;
}

void dim_on(void) {
	PORTC |= 1<<PC0;
}

void dim_off(void) {
	PORTC &= 0xFF ^ (1<<PC0);
}

void rled_on(void) {
	PORTC |= 1<<PC3;
}

void rled_off(void) {
	PORTC &= 0xFF ^ (1<<PC3);
}

void gled_on(void) {
	PORTC |= 1<<PC2;
}

void gled_off(void) {
	PORTC &= 0xFF ^ (1<<PC2);
}

void read_settings(void) {
	
	//eeprom_read_block(&settings, &ee_settings, sizeof(settings_t));
}

void write_settings(void) {
	
	//eeprom_write_block(&settings, &ee_settings, sizeof(settings_t));
}

uint8_t v_hardware_setup(void) {

	TCCR0A = 0;
	TCCR0B = 0;
	EICRA = (1<<ISC01) | (1<<ISC00);
	DDRC = (1<<PC0) | (1<<PC2) | (1<<PC3);
	DDRD = 0;

	read_settings();

	timer0_stop();
	timer0_disable_isr();
	timer0_set_counter(0);

	dim_off();
	rled_off();
	gled_off();

	ow_set_bus(&PINC, &PORTC, DDRC, PC1);

	if (search_sensors() != 1) {
		return 0;
	}

	return 1;
}

