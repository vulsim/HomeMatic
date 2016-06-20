
#include "FreeRTOS.h"
#include "queue.h"

typedef struct {

	uint8_t fault_state : 1;

	uint8_t temp;

} sensor_message_t;

extern xQueueHandle sensor_queue;

void v_sensor_task_setup(void);
