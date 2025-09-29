#ifndef FSM_SERVICE_H
#define FSM_SERVICE_H
#include <zephyr/sys/util.h>

/*List of App Events & Gates*/
#define EVENT_CENTRAL_CONNECTED    BIT(0)
#define EVENT_TEMP_READ            BIT(1)
#define EVENT_AUDIO_DETECTED       BIT(2)
#define EVENT_ERROR_OCCURED        BIT(3)
#define EVENT_CENTRAL_DISCONNECTED BIT(4)
#define EVENT_AUD_SENSE_STARTED    BIT(5)
#define EVENT_AUD_SENSE_END	   BIT(6)
#define EVENT_INFERENCE_START	   BIT(7)
#define EVENT_INFERENCE_END 	   BIT(8)

extern struct k_sem temp_sense_gate;
extern struct k_sem mic_sense_gate;
extern struct k_sem inference_gate;

/*End List Events & gates*/
void fsm_init(void);
int fsm_post_event(uint32_t events);
void fsm_run_thread(void *arg1, void *arg2, void *arg3);
#endif
