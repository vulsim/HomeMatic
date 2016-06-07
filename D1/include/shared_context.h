
#ifndef SHARED_CONTEXT_H
#define SHARED_CONTEXT_H

typedef struct {
	
	struct {		
		unsigned malfunction : 1;
		unsigned overheat : 1;
		unsigned no_ac_power : 1;
		uint16_t sensor_temp;
	} diag;		
	
	unsigned power_on : 1;
	
	uint8_t dim_level;
	
} shared_context_t;

#endif /* SHARED_CONTEXT_H */