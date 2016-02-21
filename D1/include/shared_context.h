
#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

#include "settings.h"

typedef struct {
	
	unsigned alarm_hw_error : 1;
	unsigned req_read_settings : 1;
	unsigned req_save_settings : 1;
	
	uint8_t dima_level1;
	uint8_t dima_level2;	
	uint8_t dimb_level1;
	uint8_t dimb_level2;
	
	settings_t *settings;
} shared_context_t;

#endif /* SHARED_CONTEXT_H */