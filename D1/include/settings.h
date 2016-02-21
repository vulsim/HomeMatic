
#ifndef SETTINGS_H
#define SETTINGS_H

#include <avr/io.h>

typedef struct {
	uint8_t dima_level1;
	uint8_t dima_level2;	
	uint8_t dimb_level1;
	uint8_t dimb_level2;

} settings_t;

#endif /* SETTINGS_H */