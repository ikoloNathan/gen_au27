/**
 * @file message.h
 * @brief Defines the message structure, encoding/decoding functions, and system communication macros.
 *
 * This file provides structures, macros, and functions to facilitate message
 * encoding, decoding, serialization, and deserialization for a 29-bit CAN ID
 * based communication system. It also defines inline functions for constructing
 * specific system state, time, and command messages.
 *
 * @author Nathan Ikolo
 * @date February 6, 2025
 */

#ifndef MESSAGE_H_
#define MESSAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/** @brief Maximum payload size in a message. */
#define MAX_PAYLOAD_SIZE 120

#define AO_SIGNAL(severity,state,type,id)		(uint32_t)(severity << 30 | state <<26 | type << 22 | id)
#define TRANSITION(signal,next_state,action)		{signal,&next_state,action}

typedef enum {
	SIG_SEVERITY_INFO = 1, SIG_SEVERITY_WARNING, SIG_SEVERITY_ERROR
} sig_severity_t;

typedef enum {
	SIG_STATE_INITIALISATION = 1,
	SIG_STATE_OPERATIONAL,
	SIG_STATE_ERROR,
	SIG_STATE_LOADER,
	SIG_STATE_MAINTENANCE
} sig_state_t;

typedef enum {
	SIG_TYPE_MONITORING = 1,
	SIG_TYPE_SNMP,
	SIG_TYPE_HTTP,
	SIG_TYPE_CAN,
	SIG_TYPE_MEMORY,
	SIG_TYPE_DATABASE,
	SIG_TYPE_GPIO,
	SIG_TYPE_FS
} sig_type_t;

typedef enum {
	DB_PUBLISH = 1, DB_READ, DB_WRITE, DB_UPDATE
} db_action_t;

typedef enum {
	SNMP_GET_RECV = 1, SNMP_GET_SENT, SNMP_SET_VAR, SNMP_SEND_TRAP
} snmp_action_t;

typedef enum{
	WS_QUERY_TX = 1,
	WS_QUERY_RX,
	WS_COMMAND
}ws_action_t;

typedef enum {
	FS_READ = 1,
	FS_READ_EXT,
	FS_PENDING_RD,
	FS_WRITE,
	FS_DELETE,
	FS_UPDATE,
	FS_PUBLISH
} fs_action_t;

typedef enum {
	DB_ADC_THRESHOLDS = 1, DB_SYSTEM_INFOS,
} db_sig_tables_t;

typedef enum {
	a
} sig_id_t;


#define DB_MSG_ID(action,table,row)			(action & 0x7) << 13 | (table & 0x1F) << 8 | (row & 0xFF)

#define DB_CHANGE_STATE_OP					AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_OPERATIONAL,			SIG_TYPE_DATABASE,1)
#define DB_CHANGE_STATE_INIT				AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_INITIALISATION,		SIG_TYPE_DATABASE,1)
#define DB_CHANGE_STATE_ERR					AO_SIGNAL(SIG_SEVERITY_ERROR,	SIG_STATE_ERROR,				SIG_TYPE_DATABASE,1)
#define DB_PUBLISH_TABLE(t,r)  				AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_OPERATIONAL,			SIG_TYPE_DATABASE,		DB_MSG_ID(DB_PUBLISH,t,r))
#define DB_READ_TABLE(t,r)					AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_OPERATIONAL,			SIG_TYPE_DATABASE,		DB_MSG_ID(DB_READ,t,r))
#define DB_WRITE_TABLE(t,r)					AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_OPERATIONAL,			SIG_TYPE_DATABASE,		DB_MSG_ID(DB_WRITE,t,r))
#define DB_UPDATE_TABLE(t,r)				AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_OPERATIONAL,			SIG_TYPE_DATABASE,		DB_MSG_ID(DB_UPDATE,t,r))

#define DB_GET_TABLE_ID(signal)				(uint8_t)((signal >> 8) & 0x1F)
#define DB_GET_ROW_IDX(signal)				(uint8_t)(signal & 0xFF)

#define SNMP_MGS_ID(action,oid)				(action & 0x3F) << 16 | (oid)

#define SNMP_CHANGE_STATE_INIT				AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_INITIALISATION,		SIG_TYPE_SNMP,1)
#define SNMP_CHANGE_STATE_OP				AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_OPERATIONAL,			SIG_TYPE_SNMP,1)
#define SNMP_CHANGE_STATE_ERR(dest)			AO_SIGNAL(SIG_SEVERITY_ERROR,	SIG_STATE_ERROR,SIG_TYPE_SNMP,	SNMP_MGS_ID(0,dest))
#define SNMP_GET_RX(dest)					AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_OPERATIONAL,			SIG_TYPE_SNMP,			SNMP_MGS_ID(SNMP_GET_RECV,dest))
#define SNMP_GET_TX(dest)					AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_OPERATIONAL,			SIG_TYPE_SNMP,			SNMP_MGS_ID(SNMP_GET_SENT,dest))
#define SNMP_SET_VALUE(dest)				AO_SIGNAL(SIG_SEVERITY_INFO,	SIG_STATE_OPERATIONAL,			SIG_TYPE_SNMP,			SNMP_MGS_ID(SNMP_SET_VAR,dest))

#define WS_MGS_ID(action,fd,oid)			(action & 0x3) << 20 | fd << 15 | (oid)
/* --- Signals for the WebServer AO (reuse your SIG_TYPE_HTTP bucket) --- */
#define WS_CHANGE_STATE_INIT  				AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_INITIALISATION, 		SIG_TYPE_HTTP, 1)
#define WS_CHANGE_STATE_OP    				AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_OPERATIONAL,    		SIG_TYPE_HTTP, 2)
#define WS_CHANGE_STATE_ERR   				AO_SIGNAL(SIG_SEVERITY_ERROR, 	SIG_STATE_ERROR,          		SIG_TYPE_HTTP, 3)

/* Pump -> AO runtime events */
#define WS_EVT_NEW_CONN       				AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_OPERATIONAL,    		SIG_TYPE_HTTP, 10)
#define WS_EVT_WS_OPEN        				AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_OPERATIONAL,    		SIG_TYPE_HTTP, 11)
#define WS_EVT_WS_MSG_RX      				AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_OPERATIONAL,    		SIG_TYPE_HTTP, 12)
#define WS_EVT_CLIENT_CLOSED  				AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_OPERATIONAL,    		SIG_TYPE_HTTP, 13)

/* AO -> Pump commands (async send) */
#define WS_CMD_SEND_TO_ONE    				AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_OPERATIONAL,    		SIG_TYPE_HTTP, 20)
#define WS_CMD_BROADCAST      				AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_OPERATIONAL,    		SIG_TYPE_HTTP, 21)

#define WS_QUERY_TX_CMD(fd,dest)      		AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_OPERATIONAL,    		SIG_TYPE_HTTP, 		WS_MGS_ID(WS_QUERY_TX,fd,dest))
#define WS_QUERY_RX_CMD(fd,dest)      		AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_OPERATIONAL,    		SIG_TYPE_HTTP, 		WS_MGS_ID(WS_QUERY_RX,fd,dest))
#define WS_SET_CMD(fd,dest)		      		AO_SIGNAL(SIG_SEVERITY_INFO,  	SIG_STATE_OPERATIONAL,    		SIG_TYPE_HTTP, 		WS_MGS_ID(WS_COMMAND,fd,dest))

/**
 * @struct message_frame_t
 * @brief Represents a complete message frame.
 */
typedef struct {
	uint32_t signal;
	uint32_t length;
	uint8_t *ptr;
	uint8_t payload[MAX_PAYLOAD_SIZE]; /**< Message payload. */
} message_frame_t;

#ifdef __cplusplus
}
#endif

#endif /* MESSAGE_H_ */
