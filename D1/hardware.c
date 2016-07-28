
#include "hardware.h"
#include "onewire.h"
#include <avr/eeprom.h>
#include <avr/io.h>

settings_t settings;
uint8_t sensor_id[OW_ROMCODE_SIZE];

settings_t ee_settings EEMEM = {
	
	.dim_level_min = 15,

	.dim_level_max = 90,

	.dim_level = 25,

	.overheat_threshold_temp = 70,

	.overheat_release_temp = 55
};

/* Sensor */

uint8_t sensors_search(void) {
	uint8_t diff = OW_SEARCH_FIRST;
	uint8_t sensors = 0;

	ow_reset();

	while (diff != OW_LAST_DEVICE && sensors < 5) {
		DS18X20_find_sensor(&diff, &sensor_id[0]);
		
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

uint8_t sensor_setup(void) {
	if (sensors_search() == 1) {
		if (DS18X20_write_scratchpad(sensor_id, 0x64, 0x8A, 0x1F) == DS18X20_OK) {
			return SENSOR_OK;
		}
	}

	return SENSOR_ERR;
}

uint8_t sensor_measure_temp(void) {
	if (DS18X20_start_meas(DS18X20_POWER_EXTERN, NULL) == DS18X20_OK) {
		return SENSOR_OK;
	}
	
	return SENSOR_ERR;
}

int16_t sensor_read_temp(void) {
	int16_t temp;	
	
	DS18X20_read_decicelsius(sensor_id, &temp);

	if (temp != DS18X20_INVALID_DECICELSIUS) {
		return temp / 10;
	}

	return SENSOR_TEMP_WRONG;	
}

/* Timer0 */

void timer0_setup(void) {
	TCCR0A = 0;
	TCCR0B = 0;
	TCNT0 = 0;
	timer0_enable_isr();
}

void timer0_enable_isr(void) {
	TIMSK0 = 1<<TOIE0;
}

void timer0_disable_isr() {
	TIMSK0 = 0;
}

void timer0_start(void) {	
    TCCR0B = (1<<CS02) | (1<<CS00); // clk/1024
}

void timer0_stop(void) {
	TCCR0B = 0;
}

void timer0_set_counter(uint8_t value) {
    TCNT0 = value;
}

/* Timer1 */

void timer1_setup(void) {	
	TCCR1A = 0;
	TCCR1B = 0;
	TCCR1C = 0;
	TCNT1 = 0;
	timer1_enable_isr();
}

void timer1_enable_isr(void) {
	TIMSK1 = 1<<TOIE1;
}

void timer1_disable_isr() {
	TIMSK1 = 0;
}

void timer1_set_counter(uint16_t value) {
	TCNT1 = value;
}

uint16_t timer1_get_counter(void) {
	return TCNT1;
}

void timer1_start(void) {
    TCCR1B = 1<<CS12; // clk/256
}

void timer1_stop(void) {
	TCCR1B = 0;
}

/* Control key */

uint8_t is_key_pressed(void) {
	return !(PIND & (1<<PD6));
}

/* Dimmer */

void dim_on(void) {
	PORTC |= 1<<PC0;
}

void dim_off(void) {
	PORTC &= 0xFF ^ (1<<PC0);
}

/* LEDs */

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

/* EEPROM */

void read_settings(settings_t *s) {
	eeprom_read_block((s) ? s : &settings, &ee_settings, sizeof(settings_t));
}

void write_settings(void) {
	eeprom_write_block(&settings, &ee_settings, sizeof(settings_t));
}

/* INT0 */

void int0_setup(void) {
	EICRA = (1<<ISC01) | (1<<ISC00);

	int0_enable_isr();
}

void int0_enable_isr(void) {	
	EIMSK = 1<<INT0;
}

void int0_disable_isr(void) {
	EIMSK = 0;	
}

/* I/O ports setup */

void io_setup(void) {
	DDRC = (1<<PC0) | (1<<PC2) | (1<<PC3);
	DDRD = 0;

	dim_off();	
	rled_off();	
	gled_off();	
}

/* General setup */

uint8_t hardware_setup(void) {

	io_setup();

	int0_setup();

	timer0_setup();

	timer1_setup();
	
	ow_set_bus(&PINC, &PORTC, &DDRC, PC1);

	if (sensor_setup() != SENSOR_OK) {
		return 0;
	}

	return 1;
}

