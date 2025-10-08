/**
 * @file fsm.c
 * @brief Implements a Finite State Machine (FSM).
 *
 * This file provides the implementation for a simple event-driven FSM.
 * It supports state transitions, entry and exit actions, and event handling.
 *
 * The FSM follows a jump-table approach to transition between states,
 * ensuring efficient event processing.
 *
 * @author Nathan Ikolo
 * @date February 21, 2025
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <fsm.h>

/**
 * @brief Initializes the FSM with an initial state.
 *
 * This function sets the FSM to its initial state and executes the entry action
 * if one is defined for the state.
 *
 * @param fsm Pointer to the FSM instance.
 * @param initial Pointer to the initial state.
 */
void fsm_init(fsm_t *fsm, state_t *initial) {
	if (initial != NULL) {
		fsm->current_state = initial;

		/* Call entry action */
		if (fsm->current_state->on_entry) {
			fsm->current_state->on_entry(fsm);
		}
	}
}

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
void fsm_handler(fsm_t *fsm, const message_frame_t *event) {
	if (!fsm->current_state)
		return;

	/* Search jump table for matching signal */
	for (uint8_t i = 0; i < fsm->current_state->transition_count; i++) {
		transition_t *t = &fsm->current_state->transitions[i];
		if (t->signal == event->signal) {
			/* Call exit action */
			if (fsm->current_state->on_exit) {
				fsm->current_state->on_exit(fsm);
			}

			/* Call transition action */
			if (t->action) {
				t->action(fsm);
			}

			/* Change state */
			fsm->current_state = t->next_state;

			/* Call entry action */
			if (fsm->current_state->on_entry) {
				fsm->current_state->on_entry(fsm);
			}

			goto handler;
		}
	}
	handler:
	/* If no transition found, call handler */
	if (fsm->current_state->handler) {
		fsm->current_state->handler(fsm, event);
	}
}

#ifdef __cplusplus
}
#endif
