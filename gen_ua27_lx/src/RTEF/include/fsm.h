/**
 * @file fsm.h
 * @brief Implements a Finite State Machine (FSM) framework.
 *
 * This file provides structures and functions for handling finite state machines.
 * It supports event-driven transitions, state entry/exit actions, and event handling.
 *
 * @author Nathan Ikolo
 * @date February 21, 2025
 */

#ifndef FSM_H_
#define FSM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "message.h"

/**
 * @typedef fsm_t
 * @brief Defines a type for a finite state machine (FSM) instance.
 *
 * The `fsm_t` structure represents an active FSM and holds a reference
 * to the current state and additional context if needed.
 */
typedef struct fsm fsm_t;

/**
 * @typedef state_t
 * @brief Defines a type for a state in the FSM.
 *
 * The `state_t` structure represents a state in the FSM, including event handling,
 * entry and exit actions, and a list of valid transitions.
 */
typedef struct state state_t;

/**
 * @typedef transition_t
 * @brief Defines a type for a transition between FSM states.
 *
 * The `transition_t` structure holds information about valid state transitions,
 * including the triggering event, the target state, and an optional action function.
 */
typedef struct transition transition_t;

/**
 * @typedef action_function
 * @brief Function pointer type for actions executed on state transitions.
 * @param fsm Pointer to the FSM instance.
 */
typedef void (*action_function)(fsm_t *fsm);

/**
 * @typedef state_handler
 * @brief Function pointer type for state-specific event handling.
 * @param fsm Pointer to the FSM instance.
 * @param event Pointer to the event structure.
 */
typedef void (*state_handler)(fsm_t *fsm, const message_frame_t *event);

/**
 * @struct event
 * @brief Defines an event that triggers state transitions in the FSM.
 */
struct event {
    uint16_t signal; /**< Unique signal identifier for the event. */
    void *data;      /**< Optional pointer to event-specific data. */
};

/**
 * @struct transition
 * @brief Defines a transition between states in the FSM.
 */
struct transition {
    uint32_t signal;        /**< The event signal that triggers this transition. */
    state_t *next_state;    /**< Pointer to the next state after transition. */
    action_function action; /**< Action function executed before transitioning. */
};

/**
 * @struct state
 * @brief Represents a state in the FSM with optional event handling.
 */
struct state {
    state_handler handler;      /**< Optional function to handle state-specific events. */
    action_function on_entry;   /**< Function executed upon entering the state. */
    action_function on_exit;    /**< Function executed upon exiting the state. */
    transition_t *transitions;  /**< Pointer to an array of valid transitions. */
    uint8_t transition_count;   /**< Number of available transitions. */
};

/**
 * @struct fsm
 * @brief Represents a finite state machine instance.
 */
struct fsm {
    state_t *current_state; /**< Pointer to the current active state. */
    void *super;            /**< Pointer to the parent object (if any). */
};

/**
 * @brief Initializes the FSM with an initial state.
 *
 * This function sets the FSM to its initial state and executes the entry action
 * if one is defined for the state.
 *
 * @param fsm Pointer to the FSM instance.
 * @param initial Pointer to the initial state.
 */
void fsm_init(fsm_t *fsm, state_t *initial);

/**
 * @brief Handles an event and processes state transitions.
 *
 * This function checks the transition table of the current state for a matching
 * event signal. If a transition exists, it executes the state's exit action,
 * calls the transition action (if defined), switches to the new state, and
 * executes the new state's entry action.
 *
 * If no transition is found, the current state's handler function is invoked.
 *
 * @param fsm Pointer to the FSM instance.
 * @param event Pointer to the event to be processed.
 */
void fsm_handler(fsm_t *fsm, const message_frame_t *event);

#ifdef __cplusplus
}
#endif

#endif /* FSM_H_ */
