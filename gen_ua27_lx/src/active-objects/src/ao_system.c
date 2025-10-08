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
#include "ao_system.h"
#include <broker.h>
#include <sys_defns.h>

#ifdef _WIN32
#include "stdio.h"
#endif

/** @brief Maximum number of system configuration topics to subscribe to. */
#define SYSTEM_CONFIGS_MAX  1

/** @brief Macro to define a state transition. */
#define TRANSIT(signal, state, action) { signal, &state, action }

/** @brief System-wide information structure. */
sys_info_t sys_info;

/** @brief Hardware version information structure. */
hw_info_t hw;

/** @brief Software version information structure. */
sw_info_t sw;

/* --- ACTION FUNCTIONS --- */

/**
 * @brief Handles entry into Initialization State.
 *
 * Subscribes to system topics and posts a status message.
 *
 * @param fsm Pointer to the FSM instance.
 */
static void on_enter_initialisation(fsm_t *fsm) {
	printf("sys_init\n");
//    topic_config_t configs[SYSTEM_CONFIGS_MAX] = {
//    				{.topic = AO_SIGNAL(SIG_STATE_ERROR,SIG_TYPE_DATABASE,0),.type = EXACT_MATCH}};
//    	broker_subscribe(((base_obj_t*) fsm->super)->broker, configs,
//    				1, ((base_obj_t*) fsm->super));

//	broker_post(((base_obj_t*) fsm->super)->broker,
//			SYSTEM_FRAME_STATUS(system_id.value, SIGNAL_POST_PASS),
//			PRIMARY_QUEUE);
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

/* --- TRANSITION TABLES --- */
/**
 * @brief Transition table for Initialization State.
 *
 * Defines possible transitions from Initialization to other states.
 */
transition_t initialisation_transitions[] = {
TRANSIT(0, operational_state, transition_operational),
TRANSIT(1, error_state, transition_error) };

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

struct state operational_state = { .handler = NULL, .on_entry =
		on_enter_operational, .on_exit = on_exit_operational, .transitions =
		operational_transitions, .transition_count = 2 };

struct state error_state = { .handler = NULL, .on_entry = on_enter_error,
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
//	printf("timer callback 10ms called %d\n",me);
}

static void timer_callback_100ms(void *context) {
	base_obj_t *me = (base_obj_t*)context;
//	printf("timer callback 100ms called %d\n",me);
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
	topic_config_t configs[SYSTEM_CONFIGS_MAX] = {
				{.topic = AO_SIGNAL(SIG_SEVERITY_ERROR,SIG_STATE_ERROR,SIG_TYPE_DATABASE,0),
						.start = AO_SIGNAL(SIG_SEVERITY_ERROR,SIG_STATE_ERROR,SIG_TYPE_DATABASE,0)
						,.type = MASK}};
	broker_subscribe(broker, configs,
				1, ((base_obj_t*) me));
	me->super.initialisation_state = &initialisation_state;
	me->timer = timer_ctor(RTOS);
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
