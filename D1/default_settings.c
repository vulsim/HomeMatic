
#include <avr/eeprom.h>
#include "settings.h"

settings_t EEMEM ee_default_settings = { 
	.dima_level1 = 127,
	.dima_level2 = 255,
	.dimb_level1 = 127,
	.dimb_level2 = 255
};
