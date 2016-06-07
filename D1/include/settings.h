
#ifndef SETTINGS_H
#define SETTINGS_H

typedef struct {
	
	unsigned soft_on : 1;
	
	uint8_t t_soft_on;

	uint8_t dim_level_min;

	uint8_t dim_level_max;

	uint8_t dim_level;

} settings_t;

#endif /* SETTINGS_H */