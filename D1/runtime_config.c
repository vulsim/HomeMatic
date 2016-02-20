
#include <avr/eeprom.h>
#include "runtime_config.h"

runtime_config_t EEMEM ee_default_runtime_config = { 
	.modbus_addr = 0x10
};
