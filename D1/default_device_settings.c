
#include <avr/eeprom.h>
#include "device_settings.h"

device_settings_t EEMEM ee_default_device_settings = { 
	.modbus_addr = 0x10
};
