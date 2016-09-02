
#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

typedef struct {
	
	uint8_t dim_level_min;

	uint8_t dim_level_max;

	uint8_t dim_level;
	
	uint8_t dim_sweep_threshold;

	uint8_t overheat_threshold_temp;

	uint8_t overheat_release_temp;

} settings_t;

#endif /* SETTINGS_H */