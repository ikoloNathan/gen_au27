/**
 * @file ao_watchdog.h
 * @brief Defines the Watchdog Active Object.
 *
 * This file provides the structure and function declarations for the watchdog
 * active object (`watchdog_obj_t`). It integrates with the active object
 * framework and utilizes a timer to monitor system health.
 *
 * The watchdog ensures periodic checks are performed and can trigger
 * recovery actions if a system component becomes unresponsive.
 *
 * @author Nathan Ikolo
 * @date March 2, 2025
 */

#ifndef AOWATCHDOG_H_
#define AOWATCHDOG_H_

#include "sys_timer.h"
#include "active_object.h"

/**
 * @struct watchdog_obj_t
 * @brief Represents the Watchdog Active Object.
 *
 * This structure extends `base_obj_t`, integrating a system timer for
 * monitoring and managing system watchdog functionality.
 */
typedef struct {
	base_obj_t super; /**< Inherits from base active object. */
	timers_t *timer; /**< Pointer to the system timer instance. */
} watchdog_obj_t;

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
watchdog_obj_t* watchdog_ctor(broker_t *broker, char *name);

#endif /* AOWATCHDOG_H_ */
