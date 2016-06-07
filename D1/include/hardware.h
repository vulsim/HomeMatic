
#ifndef HARDWARE_H
#define HARDWARE_H

#include <avr/io.h>
#include "FreeRTOSConfig.h"

#define MAX_TICKS_COUNT				0xFFFF
#define TICK_SIZE_US				(1000000 / configTICK_RATE_HZ)
#define DEFAULT_RESPONSE_US			(1000)

#define AC_FREQ_HZ					50
#define AC_GATE_DURATION_US			(1000000 / AC_FREQ_HZ) 
#define AC_GATE_THRESHOLD_US		(AC_GATE_DURATION_US * 30 / 100)
#define AC_GATE_MIN_US				(2 * 1000)

extern settings_t settings;

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