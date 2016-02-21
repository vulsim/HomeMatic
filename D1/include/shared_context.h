
#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

#include "device_settings.h"

typedef struct {
	
	struct {	
		unsigned request_read_device_settings :1;
		unsigned request_save_device_settings :1;
		unsigned alarm_overheat :1;
		unsigned alarm_hardware_error :1;
		unsigned alarm_modbus_error :1;	
		unsigned c :1;
		unsigned b :1;
		unsigned a :1;
	} status_flags;
	
	device_settings_t *device_settings;
} shared_context_t;

#endif /* SHARED_CONTEXT_H */