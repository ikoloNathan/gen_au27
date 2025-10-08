/**
 * @file ao_system.h
 * @brief Defines the system active object and its state machine.
 *
 * This file provides the structure and function declarations for the
 * system active object (`system_obj_t`). It integrates with the active
 * object framework and manages system states using a finite state machine (FSM).
 *
 * The system active object is responsible for transitioning between
 * different operational states and handling timed events.
 *
 * @author Nathan Ikolo
 * @date February 21, 2025
 */

#ifndef INCLUDE_AO_SYSTEM_H_
#define INCLUDE_AO_SYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "active_object.h"
#include <fsm.h>
#include <sys_timer.h>

/**
 * @struct system_obj_t
 * @brief Represents the system active object.
 *
 * This structure extends `base_obj_t`, integrating a timer for
 * handling scheduled events. It forms the core of the system's
 * event-driven execution model.
 */
typedef struct {
	base_obj_t super; /**< Inherits from base active object. */
	timers_t *timer; /**< Pointer to the system timer instance. */
} system_obj_t;

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
void system_ctor(system_obj_t *const me, broker_t *broker, char *name);

/** @brief State variable representing the initialization state. */
extern struct state initialisation_state;

/** @brief State variable representing the operational state. */
extern struct state operational_state;

/** @brief State variable representing the error state. */
extern struct state error_state;

/** @brief State variable representing the loader state. */
extern struct state loader_state;

/** @brief State variable representing the maintenance state. */
extern struct state maintenance_state;

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AO_SYSTEM_H_ */
