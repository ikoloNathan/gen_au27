/*
 * fsm_hpa.h
 *
 *  Created on: 28 Oct 2025
 *      Author: root
 */

#ifndef STATE_MACHINES_INCLUDES_FSM_HPA_H_
#define STATE_MACHINES_INCLUDES_FSM_HPA_H_

#include "fsm.h"
#include "active_object.h"

typedef enum{
	HPA_OUTPUT_OFF,
	HPA_OUTPUT_STANDBY,
	HPA_OUTPUT_TRANSMIT
}hpa_output_state_t;

typedef struct {
	fsm_t super;
	base_obj_t *ao;
	hpa_output_state_t st;
	const char *name;
}fsm_hpa_t;

void fsm_hpa_ctor(fsm_hpa_t *me,base_obj_t *ao,const char *name);


extern state_t fsm_hpa_off_state;
extern state_t fsm_hpa_standby_state;
extern state_t fsm_hpa_transmit_state;
extern state_t fsm_hpa_error_state;

#endif /* STATE_MACHINES_INCLUDES_FSM_HPA_H_ */
