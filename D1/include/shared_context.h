
#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

#include "settings.h"

typedef struct {
	
	unsigned diag_p0_malfunction : 1;
	unsigned diag_p0_no_power : 1;
	unsigned diag_p1_no_ac_gate : 1;	
		
	// Dimmer state: 0 - off, 1 - on (mode1), 2 - on (mode2)
	
	unsigned dima_state : 2;
	unsigned dimb_state : 2; 
	
	uint8_t dima_level1;
	uint8_t dimb_level1;
	
	uint8_t dima_level2;
	uint8_t dimb_level2;
	
	settings_t *settings;
} shared_context_t;

#endif /* SHARED_CONTEXT_H */