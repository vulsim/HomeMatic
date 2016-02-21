
#ifndef HARDWARE_H
#define HARDWARE_H

#include <avr/io.h>
#include "FreeRTOSConfig.h"

#define MAX_TICKS_COUNT				0xFFFF
#define TICK_SIZE_US				(1000000 / configTICK_RATE_HZ)
#define DEFAULT_RESPONSE_US			(1000)

#define AC_FREQ_HZ					50
#define AC_GATE_DURATION_US			(1000000 / AC_FREQ_HZ) 
#define AC_GATE_THRESHOLD_US		(AC_GATE_DURATION_US * 3 / 2)
#define AC_GATE_THRESHOLD2_US		(AC_GATE_DURATION_US * 3 / 4)
#define AC_GATE_MIN_US				(2 * 1000)

#define POWER_ON_RESPONSE_US		(100 * 1000)
#define DIAG_NO_POWER_TIMEOUT_US	(60 * 1000000)
#define DIAG_NO_AC_GATE_TIMEOUT_US	(5 * 1000000)

uint8_t is_ac_gate(void);

uint8_t is_power_on(void);

void set_dimmer_output(uint8_t power_on, uint8_t a_on, uint8_t b_on);

void v_setup_hardware(void);

#endif /* HARDWARE_H */