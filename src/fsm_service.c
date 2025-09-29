#include <zephyr/logging/log.h>
#include <zephyr/smf.h>

#include "fsm_service.h"
#include "zephyr/kernel.h"

LOG_MODULE_REGISTER(fsm_module);

/*App Gates*/
K_SEM_DEFINE(temp_sense_gate, 0, 1);
K_SEM_DEFINE(mic_sense_gate, 0, 1);
K_SEM_DEFINE(inference_gate, 0, 1);

/*End Gates*/

static const struct smf_state ble_app_states[];

enum ble_app_state {
	IDLE,
	CONNECTED,
	TEMP_READ,
	MIC_CAPTURE,
	INFERING,
	ERROR
};

struct state_object {
	/*Must be first for  casting*/
	struct smf_ctx state_context;

	/*Rest of the State Stuff*/
	struct k_event smf_event;
	int32_t events;

} state_object;

// idle
static void idle_entry(void *o)
{
	LOG_WRN("Entered Idle");
	k_sem_reset(&mic_sense_gate);
	k_sem_reset(&temp_sense_gate);
}
static enum smf_state_result idle_run(void *o)
{
	struct state_object *so = o;

	/*Wait for connect*/
	if (so->events & EVENT_CENTRAL_CONNECTED) {
		LOG_WRN("Go to connected State");
		smf_set_state(SMF_CTX(&so->state_context), &ble_app_states[CONNECTED]);
	}
	return SMF_EVENT_HANDLED;
}
static void idle_exit(void *o)
{
	LOG_WRN("Left Idle");
}

// connected
static void connected_entry(void *o)
{
	LOG_WRN("Entering Connected state");
	/*Once connected open the sense gates*/
	k_sem_give(&mic_sense_gate);
	k_sem_give(&temp_sense_gate);

}

static enum smf_state_result connected_run(void *o)
{
	struct state_object *so = o;
	if (so->events & EVENT_CENTRAL_DISCONNECTED) {
		LOG_WRN("Go to connected State");
		smf_set_state(SMF_CTX(&so->state_context), &ble_app_states[IDLE]);
	} else if (so->events & EVENT_AUD_SENSE_STARTED) {
		LOG_WRN("Go to MIC Capture State");
		smf_set_state(SMF_CTX(&so->state_context), &ble_app_states[MIC_CAPTURE]);
	}
	return SMF_EVENT_HANDLED;
}

static void connected_exit(void *o)
{
	LOG_WRN("Left Connnected");
}

// temp  read

static void temp_read_entry(void *o)
{
	LOG_WRN("Entered Temp_Read");
}
static enum smf_state_result temp_read_run(void *o)
{
	smf_set_state(SMF_CTX(&state_object), &ble_app_states[TEMP_READ]);
	return SMF_EVENT_HANDLED;
}

// mic capture
static void mic_capture_entry(void *o)
{
	LOG_WRN("Entered MIC_CAPTURE"); 
	k_sem_take(&inference_gate, K_NO_WAIT);
}
static enum smf_state_result mic_capture_run(void *o)
{

	struct state_object *so = o;
	if (so->events & EVENT_AUD_SENSE_END) {
		LOG_WRN("Go to Infering State");
		smf_set_state(SMF_CTX(&so->state_context), &ble_app_states[INFERING]);
	}
	return SMF_EVENT_HANDLED;
}

static void mic_capture_exit(void *o)
{
	LOG_WRN("Left MIC Capture");
	k_sem_give(&inference_gate);
	
}



static void infering_entry(void *o)
{
	LOG_WRN("Entered INFERING");
	k_sem_take(&mic_sense_gate, K_NO_WAIT);
}
static enum smf_state_result infering_run(void *o)
{
	struct state_object *so = o;
	if (so->events & EVENT_INFERENCE_END) {
		LOG_WRN("Go to Connected State");
		smf_set_state(SMF_CTX(&so->state_context), &ble_app_states[CONNECTED]);
	}
	return SMF_EVENT_HANDLED;
}
static void infering_exit(void *o)
{
	LOG_WRN("Left Infering");
	k_sem_give(&mic_sense_gate);
}





// error state

static void error_entry(void *o)
{
	LOG_WRN("Entered Error");
}
static enum smf_state_result error_run(void *o)
{
	// Do computation here to determine if you want to be here or not
	// or cause some side effect
	smf_set_state(SMF_CTX(&state_object), &ble_app_states[ERROR]);
	return SMF_EVENT_HANDLED;
}

// make a state table
static const struct smf_state ble_app_states[] = {
	[IDLE] = SMF_CREATE_STATE(idle_entry, idle_run, idle_exit, NULL, NULL),
	[CONNECTED] = SMF_CREATE_STATE(connected_entry, connected_run, connected_exit, NULL, NULL),
	[TEMP_READ] = SMF_CREATE_STATE(temp_read_entry, temp_read_run, NULL, NULL, NULL),
	[MIC_CAPTURE] = SMF_CREATE_STATE(mic_capture_entry, mic_capture_run, mic_capture_exit, NULL, NULL),
	[INFERING] = SMF_CREATE_STATE(infering_entry, infering_run, infering_exit, NULL, NULL),
	[ERROR] = SMF_CREATE_STATE(error_entry, error_run, NULL, NULL, NULL),
};

struct fsm_event {
	uint32_t events;
};

K_MSGQ_DEFINE(fsm_event_q, sizeof(struct fsm_event), 10, 1);

int fsm_post_event(uint32_t events)
{
	LOG_WRN("INCOMING EVENT 0x%08x", events);
	struct fsm_event ev = {
		.events = events,
	};
	return k_msgq_put(&fsm_event_q, &ev, K_NO_WAIT);
}

void fsm_init(void)
{
	/*Initialize events*/
	k_event_init(&state_object.smf_event);

	smf_set_initial(SMF_CTX(&state_object), &ble_app_states[IDLE]);
}

void fsm_run_thread(void *arg1, void *arg2, void *arg3)
{
	while (1) {
		struct fsm_event ev;
		int rc = k_msgq_get(&fsm_event_q, &ev, K_FOREVER);
		if (rc != 0) {
			continue;
		}

		state_object.events = ev.events;

		int ret = smf_run_state(SMF_CTX(&state_object));
		if (ret != 0) {
			LOG_ERR("FSM exploded (%d)", ret);
			break;
		}
	}
}
