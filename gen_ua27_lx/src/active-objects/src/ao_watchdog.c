/**
 * @file ao_watchdog.c
 * @brief Implements the Watchdog Active Object.
 *
 * This file defines the Watchdog Active Object, responsible for monitoring
 * system activity through heartbeat messages and detecting unresponsive components.
 *
 * The watchdog periodically checks system health and can take corrective action
 * if a component fails to send a heartbeat within the defined timeout.
 *
 * The implementation supports both Windows (via system timers) and FreeRTOS
 * (via task scheduling).
 *
 * @author Nathan Ikolo
 * @date March 2, 2025
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <ao_watchdog.h>
#include "broker.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #include <synchapi.h>
    #include <stdio.h> /**< File logging for Windows */
#endif

/** @brief Heartbeat timeout threshold in milliseconds. */
#define HEARTBEAT_TIMEOUT 200

/** @brief Function prototypes for internal watchdog functions. */
static void dispatch(base_obj_t *const me, const message_frame_t *frame);
static void heartbeat_monitor_callback(void *context);
static void heartbeat(void *context);

/**
 * @brief Periodic heartbeat sender.
 *
 * This function is triggered every 10ms. It posts a SYSTEM_TIME_EVENT
 * to the message broker to signal system activity.
 *
 * @param context Pointer to the broker instance.
 */
static void heartbeat(void *context) {
	static uint8_t tick = 0;
//	broker_t *broker = (broker_t*) context;

	if (tick++ > 10) {
//		broker_post(broker, SYSTEM_FRAME_TIME_EVENT(system_id.value, 0),
//				PRIMARY_QUEUE);
		tick = 0;
	}
}

/**
 * @brief Constructs and initializes the Watchdog Active Object.
 *
 * This function initializes the watchdog active object, linking it
 * to a broker for event-based monitoring of system health.
 *
 * @param broker Pointer to the event broker managing communication.
 * @param name Name of the watchdog active object instance.
 * @return Pointer to the initialized watchdog active object.
 */
watchdog_obj_t* watchdog_ctor(broker_t *broker, char *name) {
	static watchdog_obj_t *me;
	if (!me) {
		me = (watchdog_obj_t*) malloc(sizeof(watchdog_obj_t));
		memset(me, 0, sizeof(watchdog_obj_t));
		INIT_BASE(me, broker, name, system_id, NULL);

		me->timer = timer_ctor();

		/* Schedule heartbeat transmission every 10ms */
		timer_callback_entry_t *entry_heartbeat = me->timer->add_callback(
				TIMER_10ms, heartbeat, broker, 1,false);
		me->timer->arm(entry_heartbeat);

		/* Schedule heartbeat monitoring every 100ms */
		timer_callback_entry_t *entry_checkbeat = me->timer->add_callback(
				TIMER_100ms, heartbeat_monitor_callback, NULL, 1,false);
		me->timer->arm(entry_checkbeat);
	}
	return me;
}

/**
 * @brief Dispatch function to handle incoming messages.
 *
 * This function updates the last heartbeat timestamp when a
 * SYSTEM_TIME_EVENT is received.
 *
 * @param me Pointer to the active object instance.
 * @param frame Pointer to the received message frame.
 */
static void dispatch(base_obj_t *const me, const message_frame_t *frame) {
	switch (frame->signal) {
	case 0:
#if defined (_WIN32) || defined(__linux__)
		me->last_heartbeat_time = (uint32_t) get_time_ms();
#else
            me->last_heartbeat_time = xTaskGetTickCount();
#endif
		break;
	}
}

/**
 * @brief Periodic watchdog monitor for detecting unresponsive active objects.
 *
 * This function checks the last heartbeat timestamp of all active objects
 * and identifies any that have exceeded the `HEARTBEAT_TIMEOUT`. If any
 * active object is unresponsive, the system can trigger recovery actions.
 *
 * @param context Unused parameter (for compatibility with callback function signatures).
 */
void heartbeat_monitor_callback(void *context) {
#if defined (_WIN32) ||defined (__linux__)
	uint32_t current_time = (uint32_t) get_time_ms(); /**< Get system time on Windows */
#else
	uint32_t current_time = (uint32_t)xTaskGetTickCount(); /**< Get system time on FreeRTOS */
#endif

	bool wd_trigger = true;

	/* Iterate over all active objects and check last heartbeat timestamps */
	for (int i = 0; i < active_object_count; i++) {
		base_obj_t *ao = active_objects[i];

		if (ao && (current_time - ao->last_heartbeat_time) > HEARTBEAT_TIMEOUT) {
			wd_trigger &= false;
#ifdef _WIN32
            printf("[WATCHDOG] ALERT: %s is unresponsive!\n", ao->name);
#else
			// TODO: Log timeout event from AO
#endif
		} else {
			wd_trigger &= true;
		}
	}

	/* If any component is unresponsive, trigger watchdog recovery */
	if (!wd_trigger) {
#ifdef _WIN32
        printf("[WATCHDOG] TRIGGERED\n");
#else
		// Reset AO or trigger system watchdog reset
#endif
	}
}

#ifdef __cplusplus
}
#endif
