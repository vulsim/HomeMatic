
#include "FreeRTOS.h"
#include "queue.h"

typedef struct {

	uint8_t is_key_pressed : 1;

	uint8_t key_press_duration;

} input_message_t;

extern QueueHandle_t input_queue;

void v_input_task_setup(void);
