
#ifndef HARDWARE_H
#define HARDWARE_H

#include "FreeRTOSConfig.h"
#include "settings.h"

#define MAX_TICKS_COUNT				0xFFFF
#define TICK_SIZE_US				(1000000 / configTICK_RATE_HZ)

#define AC_FREQ_HZ					50
#define AC_GATE_DURATION_US			(1000000 / (AC_FREQ_HZ * 2)) 
#define AC_GATE_THRESHOLD_US		(AC_GATE_DURATION_US * 30 / 100)
#define AC_GATE_MIN_US				(2 * 1000)

extern settings_t settings;

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

void v_hardware_setup(void);

#endif /* HARDWARE_H */