
#ifndef HARDWARE_H
#define HARDWARE_H

#include "FreeRTOSConfig.h"
#include "settings.h"
#include "ds18x20.h"

#define MAX_TICKS_COUNT				0xFFFF
#define TICK_SIZE_US				(1000000 / configTICK_RATE_HZ)

#define FAULT_TEMP_MIN				40
#define FAULT_TEMP_MAX				60

#define SENSOR_MEASURE_TICKS		(DS18B20_TCONV_12BIT * 10 / TICK_SIZE_US) * 100

extern settings_t settings;

uint8_t sensor_measure_temp(void);

int16_t sensor_read_temp(void);

void timer0_setup(void);

void timer0_enable_isr(void);

void timer0_disable_isr(void);

void timer0_set_counter(uint8_t value);

uint8_t timer0_get_counter(void);

void timer0_start(void);

void timer0_stop(void);

uint8_t is_key_pressed(void);

void dim_on(void);

void dim_off(void);

void rled_on(void);

void rled_off(void);

void gled_on(void);

void gled_off(void);

void read_settings(void);

void write_settings(void);

uint8_t v_hardware_setup(void);

#endif /* HARDWARE_H */