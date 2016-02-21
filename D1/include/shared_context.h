
#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

#include "settings.h"

typedef struct {
	
	struct {
		unsigned malfunction : 1;		
		unsigned no_ac_power : 1;
		unsigned no_power_feedback : 1;
	} diag;		

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