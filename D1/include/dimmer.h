
#include "queue.h"

typedef struct {

	uint8_t on_state : 1;

	uint8_t wait_sync_state : 1;	

	uint8_t fault_state : 1;

} dimmer_message_t;

extern QueueHandle_t dimmer_queue;

void v_dimmer_task_setup(void);
