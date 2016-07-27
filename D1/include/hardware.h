
#ifndef HARDWARE_H
#define HARDWARE_H

#include "settings.h"
#include "ds18x20.h"

extern settings_t settings;

/* Sensor */

int8_t search_sensors(void);

uint8_t sensor_measure_temp(void);

int16_t sensor_read_temp(void);

/* Timer0 */

void timer0_setup(void);

void timer0_enable_isr(void);

void timer0_disable_isr(void);

void timer0_start(void);

void timer0_stop(void);

void timer0_set_counter(uint8_t value);

/* Timer1 */

void timer1_setup(void);

void timer1_enable_isr(void);

void timer1_disable_isr(void);

void timer1_set_counter(uint16_t value);

uint16_t timer1_get_counter(void);

void timer1_start(void);

void timer1_stop(void);

/* Control key */

uint8_t is_key_pressed(void);

/* Dimmer */

void dim_on(void);

void dim_off(void);

/* LEDs */

void rled_on(void);

void rled_off(void);

void gled_on(void);

void gled_off(void);

/* EEPROM */

void read_settings(settings_t *s);

void write_settings(void);

/* INT0 */


void int0_setup(void);

void int0_enable_isr(void);

void int0_disable_isr(void);

/* General setup */

uint8_t hardware_setup(void);

#endif /* HARDWARE_H */