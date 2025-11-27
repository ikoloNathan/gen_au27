/*
 * fsm_hpa.c
 *
 *  Created on: 28 Oct 2025
 *      Author: root
 */


#include <stdlib.h>
#include <cjson/cJSON.h>
#include "fsm_hpa.h"
#include "broker.h"

static char *hpa_state_json(fsm_hpa_t *me);

static void hpa_on_entry_off_st(fsm_t *fsm);
static void hpa_on_exit_off_st(fsm_t *fsm);
static void hpa_handler_off_st(fsm_t *fsm, const message_frame_t *event);

static void hpa_on_entry_standby_st(fsm_t *fsm);
static void hpa_on_exit_standby_st(fsm_t *fsm);
static void hpa_handler_standby_st(fsm_t *fsm, const message_frame_t *event);

static void hpa_on_entry_transmit_st(fsm_t *fsm);
static void hpa_on_exit_transmit_st(fsm_t *fsm);
static void hpa_handler_transmit_st(fsm_t *fsm, const message_frame_t *event);

transition_t off_transitions[] = {
TRANSITION(0, fsm_hpa_standby_state, NULL),
TRANSITION(2, fsm_hpa_transmit_state, NULL)
};

transition_t standby_transitions[] = {
		TRANSITION(0, fsm_hpa_off_state, NULL),
		TRANSITION(2, fsm_hpa_transmit_state, NULL)
};


transition_t transmit_transitions[] = {
		TRANSITION(0, fsm_hpa_standby_state, NULL),
		TRANSITION(2, fsm_hpa_off_state, NULL)
};



state_t fsm_hpa_off_state = {
		.handler = hpa_handler_off_st, .on_entry = hpa_on_entry_off_st, .on_exit = hpa_on_exit_off_st,
		.transitions = off_transitions, .transition_count = 2
};
state_t fsm_hpa_standby_state = {
		.handler = hpa_handler_standby_st, .on_entry = hpa_on_entry_standby_st, .on_exit = hpa_on_exit_standby_st,
		.transitions = standby_transitions, .transition_count = 2
};
state_t fsm_hpa_transmit_state = {
		.handler = hpa_handler_transmit_st, .on_entry = hpa_on_entry_transmit_st, .on_exit = hpa_on_exit_transmit_st,
		.transitions = transmit_transitions, .transition_count = 2
};


char *hpa_state_json(fsm_hpa_t *me){
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root,"cmd","transmit_state");
	cJSON_AddNumberToObject(root, "state", me->st);
	cJSON_AddNumberToObject(root, "id", 0);
	char *txt = cJSON_Print(root);
	cJSON_Delete(root);
	return txt;
}

void hpa_on_entry_off_st(fsm_t *fsm){
	printf("broadcast HPA Off state\n");
}
void hpa_on_exit_off_st(fsm_t *fsm){

}
void hpa_handler_off_st(fsm_t *fsm, const message_frame_t *event){
	printf("in hpa-fsm: %s\n",event->payload);
	message_frame_t msg = {
			.signal = WS_QUERY_RX_CMD(0,0)
	};
	char *txt = hpa_state_json(((fsm_hpa_t*) fsm));
	msg.length = strlen(txt);
	memcpy(msg.payload,txt,msg.length);
	broker_post(((fsm_hpa_t*) fsm)->ao->broker,msg,0);
	free(txt);
}

void hpa_on_entry_standby_st(fsm_t *fsm){

}

void hpa_on_exit_standby_st(fsm_t *fsm){

}
void hpa_handler_standby_st(fsm_t *fsm, const message_frame_t *event){

}
void hpa_on_entry_transmit_st(fsm_t *fsm){

}
void hpa_on_exit_transmit_st(fsm_t *fsm){

}

void hpa_handler_transmit_st(fsm_t *fsm, const message_frame_t *event){

}


void fsm_hpa_ctor(fsm_hpa_t *me,base_obj_t *ao,const char *name){
	me->ao = ao;
	me->st = HPA_OUTPUT_OFF;
	fsm_init((fsm_t*)me,&fsm_hpa_off_state);
}


