/**
 * @file ao_system.c
 * @brief Implements the System Active Object with FSM-based state transitions.
 *
 * This file defines the states, transitions, and dispatch mechanism for
 * managing the system's operation, including Initialization, Operational,
 * Error, Loader, and Maintenance modes.
 *
 * The system Active Object integrates with a finite state machine (FSM) to
 * manage transitions between different states based on incoming events.
 *
 * @author Nathan Ikolo
 * @date February 21, 2025
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdio.h>
#include <cjson/cJSON.h>
#include "ao_system.h"
#include <broker.h>
#include <sys_defns.h>

#ifdef _WIN32
#include "stdio.h"
#endif

#define INFO_PATH	"/usr/bin/myapp/info.json"
/** @brief Maximum number of system configuration topics to subscribe to. */
#define SYSTEM_CONFIGS_MAX  1
#define SYSTEM_GET_SNMP			SNMP_GET_TX(1<<8)
#define SYSTEM_SET_SNMP			SNMP_SET_VALUE(1<<8)

#define SNMP_GET_RANGE			SNMP_GET_TX(0) ... SNMP_GET_TX(0xFFFF)
#define SNMP_SET_RANGE			SNMP_SET_VALUE(0) ... SNMP_SET_VALUE(0xFFFF)

/** @brief System-wide information structure. */
sys_info_t sys_info;

/** @brief Hardware version information structure. */
hw_info_t hw;

/** @brief Software version information structure. */
sw_info_t sw;


static void sys_operation_handler(fsm_t *fsm,const message_frame_t *event);
static void sys_error_handler(fsm_t *fsm,const message_frame_t *event);

void goto_err(fsm_t *fsm){
	topic_config_t config[] = {{ .topic = SYS_CHANGE_STATE_ERR, .type = EXACT_MATCH },
							{ .topic = SYS_CHANGE_STATE_INIT, .type = EXACT_MATCH }
	};
	message_frame_t msg = {
			.signal = SYS_CHANGE_STATE_ERR
	};
	broker_subscribe(((base_obj_t*) fsm->super)->broker, config,
			sizeof(config) / sizeof(config[0]), ((base_obj_t*) fsm->super));

	post((base_obj_t*)fsm->super,msg);
}

bool read_info(){

	 FILE *fp = fopen(INFO_PATH, "r");
	    if (fp == NULL) {
	        perror("Error opening file");
	        return false;
	    }

	    char buffer[256] = {0};
	    char buff[1024] = {0};
	    int i = 0;
	    while (fgets(buffer, sizeof(buffer), fp)) {
	    	memcpy(buff + i,buffer,strlen(buffer));
	    	i = i + strlen(buffer);
	    }
	    fclose(fp);
	    cJSON *root = cJSON_Parse(buff);
	    if(!root)
	    	return false;
	    const cJSON *hw_id = cJSON_GetObjectItemCaseSensitive(root, "hw_id");
	    if (!cJSON_IsString(hw_id) || !hw_id->valuestring) {
			cJSON_Delete(root);
			return false;
		}
	    const cJSON *hw_rev = cJSON_GetObjectItemCaseSensitive(root, "hw_rev");
	    if (!cJSON_IsString(hw_rev) || !hw_rev->valuestring) {
			cJSON_Delete(root);
			return false;
		}
	    const cJSON *sw_id = cJSON_GetObjectItemCaseSensitive(root, "sw_id");
		if (!cJSON_IsString(sw_id) || !sw_id->valuestring) {
			cJSON_Delete(root);
			return false;
		}
	    const cJSON *sw_version = cJSON_GetObjectItemCaseSensitive(root, "sw_version");
		if (!cJSON_IsString(sw_version) || !sw_version->valuestring) {
			cJSON_Delete(root);
			return false;
		}
	    const cJSON *sw_date = cJSON_GetObjectItemCaseSensitive(root, "sw_date");
		if (!cJSON_IsString(sw_date) || !sw_date->valuestring) {
			cJSON_Delete(root);
			return false;
		}
		strcpy(hw.id,hw_id->valuestring);
		strcpy(hw.revision,hw_rev->valuestring);
		strcpy(sw.id,sw_id->valuestring);
		strcpy(sw.version,sw_version->valuestring);
		strcpy(sw.date,sw_date->valuestring);
		return true;

}

/* --- ACTION FUNCTIONS --- */

/**
 * @brief Handles entry into Initialization State.
 *
 * Subscribes to system topics and posts a status message.
 *
 * @param fsm Pointer to the FSM instance.
 */
static void on_enter_initialisation(fsm_t *fsm) {
//	if(!read_info()){
//		//system error missing system info
//		goto_err(fsm);
//		perror("Missing system info");
//		return;
//	}
	topic_config_t config[] = {
			{ .topic = SYSTEM_GET_SNMP, .start = SYSTEM_GET_SNMP, .type = MASK },		// subscrib to all system GET snmp requests
			{ .topic = SYSTEM_SET_SNMP, .start = SYSTEM_SET_SNMP, .type = MASK}, 		// subscrib to all system SET snmp requests
			{ .topic = WS_QUERY_TX_CMD(1, 1), .type = EXACT_MATCH }, 					// subscrib to all websocket tx commands
			{ .topic = WS_EVT_WS_OPEN, .type = EXACT_MATCH},							// subscrib to webscoket open event
			{ .topic = SYS_CHANGE_STATE_INIT, .type = EXACT_MATCH},						// subscrib to system change state to move into initialisation
	};
    broker_subscribe(((base_obj_t*) fsm->super)->broker, config,
    		sizeof(config) / sizeof(config[0]), ((base_obj_t*) fsm->super));

    message_frame_t msg = {
			.signal = SYS_CHANGE_STATE_OP
	};

	post((base_obj_t*)fsm->super,msg); // move to operational
}

/**
 * @brief Handles exit from Initialization State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void on_exit_initialisation(fsm_t *fsm) {
#ifdef _WIN32
    printf("POST Completed.\n");
#endif
}

/**
 * @brief Handles entry into Operational State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void on_enter_operational(fsm_t *fsm) {
#ifdef _WIN32
    printf("System Now Operational.\n");
#endif
}

/**
 * @brief Handles exit from Operational State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void on_exit_operational(fsm_t *fsm) {
#ifdef _WIN32
    printf("Leaving Operational Mode.\n");
#endif
}

/**
 * @brief Handles entry into Error State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void on_enter_error(fsm_t *fsm) {
#ifdef _WIN32
    printf("System Encountered an Error!\n");
#endif
}

/**
 * @brief Handles exit from Error State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void on_exit_error(fsm_t *fsm) {
#ifdef _WIN32
    printf("Error Resolved.\n");
#endif
}

/**
 * @brief Handles entry into Loader State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void on_enter_loader(fsm_t *fsm) {
#ifdef _WIN32
    printf("Entering Firmware Update Mode.\n");
#endif
}

/**
 * @brief Handles exit from Loader State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void on_exit_loader(fsm_t *fsm) {
#ifdef _WIN32
    printf("Exiting Firmware Update Mode.\n");
#endif
}

/**
 * @brief Handles entry into Maintenance State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void on_enter_maintenance(fsm_t *fsm) {
#ifdef _WIN32
    printf("Entering Maintenance Mode.\n");
#endif
}

/**
 * @brief Handles exit from Maintenance State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void on_exit_maintenance(fsm_t *fsm) {
#ifdef _WIN32
    printf("Exiting Maintenance Mode.\n");
#endif
}

/* --- TRANSITION ACTIONS --- */

/**
 * @brief Transition action from Initialization to Operational State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void transition_operational(fsm_t *fsm) {
#ifdef _WIN32
    printf("POST Passed. Transitioning to Operational Mode.\n");
#endif
}

/**
 * @brief Transition action from Initialization to Error State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void transition_error(fsm_t *fsm) {
#ifdef _WIN32
    printf("POST Failed. Entering Error Mode.\n");
#endif
}

/**
 * @brief Transition action to Loader State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void transition_loader(fsm_t *fsm) {
#ifdef _WIN32
    printf("User Requested Loader Mode.\n");
#endif
}

/**
 * @brief Transition action to Maintenance State.
 *
 * @param fsm Pointer to the FSM instance.
 */
void transition_maintenance(fsm_t *fsm) {
#ifdef _WIN32
    printf("User Requested Maintenance Mode.\n");
#endif
}

void sys_error_handler(fsm_t *fsm,const message_frame_t *event){

}

void sys_operation_handler(fsm_t *fsm,const message_frame_t *event){
	system_obj_t *me = (system_obj_t*) fsm->super;
	switch(event->signal){
		case SNMP_GET_RANGE:{
			char* outstr = "{\"name\":\"unit1date\",\"mode\":\"GET\",\"value\":\"hello\"}";
			message_frame_t msg = {
					.signal = SNMP_GET_RX(0),
					.length = strlen(outstr)
			};
			memcpy(msg.payload,outstr,strlen(outstr));
			broker_post(me->super.broker,msg,PRIMARY_QUEUE);
		}break;
		case SNMP_SET_RANGE:
			break;
		case WS_QUERY_TX_CMD(1, 1):
			fsm_handler((fsm_t*)&me->hpa_output, event);
		break;
	}

}

/* --- TRANSITION TABLES --- */
/**
 * @brief Transition table for Initialization State.
 *
 * Defines possible transitions from Initialization to other states.
 */
transition_t initialisation_transitions[] = {
TRANSIT(SYS_CHANGE_STATE_OP, operational_state, transition_operational),
TRANSIT(SYS_CHANGE_STATE_ERR, error_state, transition_error) };

/**
 * @brief Transition table for Operational State.
 *
 * Defines possible transitions from Operational to other states.
 */
transition_t operational_transitions[] = {
TRANSIT(2, loader_state, transition_loader),
TRANSIT(3, maintenance_state, transition_maintenance) };

/**
 * @brief Transition table for Error State.
 *
 * Defines possible transitions from Error to other states.
 */
transition_t error_transitions[] = {
TRANSIT(4, loader_state, transition_loader),
TRANSIT(0, maintenance_state, transition_maintenance),
TRANSIT(0, operational_state, NULL) };

/**
 * @brief Transition table for Loader State.
 *
 * Defines possible transitions from Loader to other states.
 */
transition_t loader_transitions[] = {
TRANSIT(0, operational_state, NULL),
TRANSIT(0, error_state, NULL) };

/**
 * @brief Transition table for Maintenance State.
 *
 * Defines possible transitions from Maintenance to other states.
 */
transition_t maintenance_transitions[] = {
TRANSIT(0, operational_state, NULL),
TRANSIT(0, error_state, NULL),
TRANSIT(0, loader_state, NULL),
TRANSIT(0, initialisation_state, NULL) };

/* --- STATE DEFINITIONS --- */
struct state initialisation_state = { .handler = NULL, .on_entry =
		on_enter_initialisation, .on_exit = on_exit_initialisation,
		.transitions = initialisation_transitions, .transition_count = 2 };

struct state operational_state = { .handler = sys_operation_handler, .on_entry =
		on_enter_operational, .on_exit = on_exit_operational, .transitions =
		operational_transitions, .transition_count = 2 };

struct state error_state = { .handler = sys_error_handler, .on_entry = on_enter_error,
		.on_exit = on_exit_error, .transitions = error_transitions,
		.transition_count = 3 };

struct state loader_state = { .handler = NULL, .on_entry = on_enter_loader,
		.on_exit = on_exit_loader, .transitions = loader_transitions,
		.transition_count = 2 };

struct state maintenance_state = { .handler = NULL, .on_entry =
		on_enter_maintenance, .on_exit = on_exit_maintenance, .transitions =
		maintenance_transitions, .transition_count = 4 };

/**
 * @brief Dispatches incoming messages to the FSM.
 *
 * @param me Pointer to the ActiveObject.
 * @param frame Message frame received.
 */
static void dispatch(base_obj_t *const me, const message_frame_t *frame) {
	fsm_handler(&me->fsm, frame);
}

static void timer_callback_10ms(void *context) {
	base_obj_t *me = (base_obj_t*)context;
//	printf("timer callback 10ms called %s 1\n",me->name);
}

static void timer_callback_100ms(void *context) {
	base_obj_t *me = (base_obj_t*)context;
//	printf("timer callback 100ms called %s 2\n",me->name);
}

/**
 * @brief Constructs and initializes the system active object.
 *
 * This function initializes the system active object, linking it
 * to a broker for event-based communication.
 *
 * @param me Pointer to the system active object instance.
 * @param broker Pointer to the event broker managing communication.
 * @param name Name of the system active object.
 */
void system_ctor(system_obj_t *const me, broker_t *broker, char *name) {
	INIT_BASE(me, broker, name, system_id, NULL);
	MsgQueue_Init(&me->super.msgQueue);
	me->super.initialisation_state = &initialisation_state;
	fsm_hpa_ctor(&me->hpa_output,(base_obj_t*)me,"hpa_states");
	me->timer = timer_ctor();
	timer_callback_entry_t *entry1 = me->timer->add_callback(TIMER_10ms,
			timer_callback_10ms, me, 1);
	timer_callback_entry_t *entry2 = me->timer->add_callback(TIMER_100ms,
			timer_callback_100ms, me, 1);
	me->timer->arm(entry1);
	me->timer->arm(entry2);
}

#ifdef __cplusplus
}
#endif
