/*
 * ao_snmp.c
 *
 *  Created on: Aug 26, 2025
 *      Author: Nathan Ikolo
 */

/**
 * @file ao_snmp.c
 * @brief Net-SNMP Agent Active Object (Linux) — FSM + broker-bridged async GET/SET.
 *
 * @details
 * - Runs a Net-SNMP master or AgentX subagent.
 * - Uses a dedicated pump thread blocking in agent_check_and_process(1).
 * - Implements asynchronous GET completion via Net-SNMP's delegated request pattern:
 *   - Mark varbinds as delegated, create a delegated cache.
 *   - Publish a GET_REQ over the broker with a correlation ID.
 *   - On GET_RSP (from other components), schedule a zero-delay snmp_alarm to
 *     complete the original request on the agent thread.
 *
 */

#ifdef __linux__

#include "ao_snmp.h"

#include <cjson/cJSON.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

/**
 * @def MAX_MIB_ENTRY
 * @brief Maximum number of entries supported in the MIB map.
 */
#define MAX_MIB_ENTRY        255

/**
 * @def MSG_OID
 * @brief Extract a message identifier from an SNMP OID array.
 *
 * Combines three elements of the OID array into a 16-bit
 * message identifier.
 *
 * @param oid Pointer to an OID array (expects length > 10).
 * @return 16-bit message identifier.
 */
#define MSG_OID(oid)        (uint16_t)(oid[8] << 0xF | oid[9] << 4 | oid[10])

/**
 * @def CORR
 * @brief Extract a 32-bit correlation identifier from a payload buffer.
 *
 * Combines four bytes from the payload into a 32-bit integer
 * in network-to-host order.
 *
 * @param payload Pointer to a byte buffer (expects length > 3).
 * @return 32-bit correlation ID.
 */
#define CORR(payload)       (uint32_t)(payload[3] << 24 | payload[2] << 16 | payload[1] << 8 | payload[0])

/**
 * @enum mib_status_t
 * @brief Status codes returned by MIB table operations.
 *
 * Indicates the outcome of insert, lookup, or management
 * operations performed on the MIB entry table.
 */
typedef enum {
	MIB_OK = 0, /**< Operation succeeded. */
	MIB_ERR_FULL, /**< Table is full; no more entries can be added. */
	MIB_ERR_DUPLICATE, /**< An entry with the same key already exists. */
	MIB_ERR_NOT_FOUND /**< Requested entry was not found in the table. */
} mib_status_t;

/**
 * @struct mib_entry
 * @brief Represents a single Management Information Base (MIB) entry.
 *
 * Each entry maps a message identifier to its SNMP symbolic name,
 * type, access rights, and associated value buffer.
 */
typedef struct mib_entry {
	oid msg_id[MAX_OID_LEN]; /**< Unique message identifier (OID portion). */
	char name[64]; /**< SNMP symbol name used as the key. */
	int asn_type; /**< ASN.1 type of the value (e.g., INTEGER, OCTET_STR). */
	int access; /**< Access rights (e.g., read-only, read-write). */
	size_t val_len; /**< Length of the value stored in @c value. */
} mib_entry_t;

typedef struct {
	char name[64];
	uint8_t delegate;
	uint8_t buff[128];
} snmp_payload_t;

/**
 * @struct mib_map_t
 * @brief Container for all registered MIB entries.
 *
 * Provides a simple fixed-size table of pointers to @c mib_entry_t
 * structures along with a count of active entries.
 */
typedef struct {
	mib_entry_t *entry[MAX_MIB_ENTRY]; /**< Array of pointers to MIB entries. */
	uint8_t count; /**< Current number of valid entries. */
} mib_map_t;

/**
 * @var mib_map
 * @brief Global instance of the MIB entry map.
 *
 * Serves as the root table holding all registered MIB entries
 * for the SNMP agent. Initialized to zero at startup.
 */
static mib_map_t mib_map = { 0 };

static void dispatch(base_obj_t *const me, const message_frame_t *frame);
static void* snmp_pump(void *arg);
static mib_status_t add_mib_entry(mib_entry_t *entry);
static mib_entry_t* find_mib_entry_by_name(const char* name);
static void tree_to_json(struct tree *subtree, const char *prefix,
		cJSON *jarray);
static bool register_from_json(snmp_agent_ao_t *me, const char *json_str);
static char* snmp_json_str(const char *name, uint8_t action, const char *value);
static bool snmp_payload_from_json(const char *json_str, snmp_payload_t *out,
		char *errbuf, size_t errbuf_sz);

static void complete_get_cb(unsigned int clientreg, void *clientarg);
static void schedule_get_completion(pending_get_t *pg);
static int snmp_scalar_handler(netsnmp_mib_handler *handler,
		netsnmp_handler_registration *reginfo,
		netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests);
static bool snmp_agent_init(snmp_agent_ao_t *me);
static void snmp_agent_shutdown(snmp_agent_ao_t *me);
static void snmp_append_delegate_agent(snmp_agent_ao_t *me, pending_get_t *pg);
static void snmp_clear_delegate_list(snmp_agent_ao_t *me);
static pending_get_t* snmp_find_delegate_agent(snmp_agent_ao_t *me,
		const char* name);
static void snmp_operational_handler(fsm_t *fsm, const message_frame_t *event);
static void snmp_error_handler(fsm_t *fsm, const message_frame_t *event);
static int snmp_log_callback(int major, int minor, void *serverarg,
		void *clientarg);
static void snmp_on_entry_initialisation(fsm_t *fsm);
static void snmp_on_entry_operational(fsm_t *fsm);
static void snmp_on_entry_error(fsm_t *fsm);

/* --- Transition tables --- */
transition_t snmp_initialisation_transitions[] = {
TRANSITION(SNMP_CHANGE_STATE_OP, snmp_operational_state, NULL),
TRANSITION(SNMP_CHANGE_STATE_ERR(0), snmp_error_state, NULL) };

transition_t snmp_operational_transitions[] = {
TRANSITION(SNMP_CHANGE_STATE_ERR(0), snmp_error_state, NULL),
TRANSITION( SNMP_CHANGE_STATE_INIT, snmp_initialisation_state, NULL ) };

transition_t snmp_error_transitions[] = { {
SNMP_CHANGE_STATE_OP, &snmp_operational_state, NULL } };

/* --- STATE DEFINITIONS --- */
state_t snmp_initialisation_state = { .handler = NULL, .on_entry =
		snmp_on_entry_initialisation, .on_exit = NULL, .transitions =
		snmp_initialisation_transitions, .transition_count = 2 };

/**
 * @brief Transition table for Operational State.
 *
 * Defines possible transitions from Operational to other states.
 */
state_t snmp_operational_state = { .handler = snmp_operational_handler,
		.on_entry = snmp_on_entry_operational, .on_exit = NULL, .transitions =
				snmp_operational_transitions, .transition_count = 2 };

/**
 * @brief Transition table for Error State.
 *
 * Defines possible transitions from Operational to other states.
 */
state_t snmp_error_state = { .handler = snmp_error_handler, .on_entry =
		snmp_on_entry_error, .on_exit = NULL, .transitions =
		snmp_error_transitions, .transition_count = 1 };

/**
 * @brief Convert an SNMP MIB subtree into JSON representation.
 *
 * Recursively walk MIB tree and build JSON array
 * Traverses the given @p subtree and appends entries to the provided
 * JSON array. Each node is prefixed with @p prefix for naming context.
 *
 * @param subtree Pointer to the SNMP tree subtree to convert.
 * @param prefix  String prefix applied to each node name.
 * @param jarray  JSON array object to which node objects are appended.
 */
void tree_to_json(struct tree *subtree, const char *prefix, cJSON *jarray) {
	for (struct tree *t = subtree; t; t = t->next_peer) {
		char newprefix[1024] = { 0 };
		snprintf(newprefix, sizeof(newprefix), "%s.%lu", prefix, t->subid);

		if (!t->child_list) {
			/* Leaf node → build JSON object */
			cJSON *jobj = cJSON_CreateObject();
			cJSON_AddStringToObject(jobj, "oid", newprefix);
			cJSON_AddStringToObject(jobj, "name", t->label ? t->label : "");
			switch(t->type){
				case TYPE_INTEGER32:
				case TYPE_UNSIGNED32:
				case TYPE_UINTEGER:
					cJSON_AddNumberToObject(jobj, "asn_type", ASN_INTEGER);
					break;
				case TYPE_INTEGER:
					cJSON_AddNumberToObject(jobj, "asn_type", ASN_BOOLEAN);
					break;
				case TYPE_OCTETSTR:
					cJSON_AddNumberToObject(jobj, "asn_type", ASN_OCTET_STR);
					break;
				default:
					cJSON_AddNumberToObject(jobj, "asn_type", ASN_NULL);
					break;
			}
			cJSON_AddNumberToObject(jobj, "access", t->access);
			if (t->description)
				cJSON_AddStringToObject(jobj, "descr", t->description);

			cJSON_AddItemToArray(jarray, jobj);
		}

		if (t->child_list) {
			tree_to_json(t->child_list, newprefix, jarray);
		}
	}
}

/**
 * @brief Register SNMP MIB entries from a JSON description.
 *
 * Parses the given JSON string containing MIB definitions and
 * registers them with the SNMP agent instance @p me.
 * Every successfully parsed MIB entry get stored to a MIB entry map
 * and registered to snmp_scalar_handler with its corresponding oid array
 *
 * Supported MIB access type:
 * 	- MIB_ACCESS_READONLY,
 * 	- MIB_ACCESS_READWRITE
 * 	- MIB_ACCESS_WRITEONLY
 *
 * @param me       Pointer to the SNMP agent active object instance.
 * @param json_str JSON string describing MIB entries to register.
 *
 * @retval true  Registration succeeded.
 * @retval false Parsing or registration failed.
 */
bool register_from_json(snmp_agent_ao_t *me, const char *json_str) {
	//TODO assert (json_str != NULL)
	bool success = false;
	cJSON *root = cJSON_Parse(json_str);
	if (!root || !cJSON_IsArray(root)) {
		fprintf(stderr, "invalid JSON\n");
		return success;
	}

	cJSON *item;
	cJSON_ArrayForEach(item, root)
	{
		cJSON *oidj = cJSON_GetObjectItem(item, "oid");
		cJSON *namej = cJSON_GetObjectItem(item, "name");
		cJSON *typej = cJSON_GetObjectItem(item, "asn_type");
		cJSON *accj = cJSON_GetObjectItem(item, "access");

		if (!cJSON_IsString(oidj) || !cJSON_IsString(namej))
			continue;

		oid oid_arr[MAX_OID_LEN] = {0};
		size_t oid_len = MAX_OID_LEN;
		if (!read_objid(oidj->valuestring, oid_arr, &oid_len)) {
			snmp_perror("read_objid");
			continue;
		}

		mib_entry_t *entry = calloc(1, sizeof(*entry));
		memcpy(entry->name, namej->valuestring, strlen(namej->valuestring));
		entry->asn_type = typej ? typej->valueint : ASN_OCTET_STR;
		entry->access = accj ? accj->valueint : HANDLER_CAN_RONLY;
		memcpy(entry->msg_id, oid_arr, MAX_OID_LEN);
		if (add_mib_entry(entry) == MIB_OK) {
			switch (accj ? accj->valueint : HANDLER_CAN_RONLY) {
			case MIB_ACCESS_READONLY:
			case MIB_ACCESS_READWRITE:
			case MIB_ACCESS_WRITEONLY: {
				netsnmp_handler_registration *reg =
						netsnmp_create_handler_registration(entry->name,
								snmp_scalar_handler, oid_arr, oid_len,
								entry->access);
				netsnmp_register_scalar(reg);
				reg->my_reg_void = (void*) me;
				success = true;
				break;
			}
			}
		}
	}
	cJSON_Delete(root);
	return success;
}

char* snmp_json_str(const char *name, uint8_t action, const char *value) {
	cJSON *root = cJSON_CreateObject();

	if (cJSON_IsNull(root))
		return NULL;
	switch (action) {
	case 0: { //GET
		cJSON_AddStringToObject(root, "name", name);
		cJSON_AddStringToObject(root, "mode", "GET");
		char *outstr = cJSON_Print(root);
		cJSON_Delete(root);
		return outstr;
	}
		break;
	case 1: { // SET
		cJSON_AddStringToObject(root, "name", name);
		cJSON_AddStringToObject(root, "mode", "SET");
		cJSON_AddStringToObject(root, "value", value);
		char *outstr = cJSON_Print(root);
		cJSON_Delete(root);
		return outstr;
	}
		break;
	default:
		return NULL;
	}

}

/**
 * Parse JSON into snmp_payload_t.
 *
 * Expected JSON format (examples below):
 * {
 *   "name": "sysName",
 *   "buff": "0xDEADBEEF"     // or "b64:AQID" or [222,173,190,239]
 * }
 *
 * @param json_str         Input JSON string.
 * @param out              Output struct to fill (cleared first).
 * @param errbuf           Optional: buffer for error message.
 * @param errbuf_sz        Size of errbuf.
 * @return true on success; false on failure (errbuf filled if provided).
 */
bool snmp_payload_from_json(const char *json_str, snmp_payload_t *out,
		char *errbuf, size_t errbuf_sz) {
	if (!json_str || !out) {
		if (errbuf && errbuf_sz)
			snprintf(errbuf, errbuf_sz, "Invalid arguments");
		return false;
	}

	memset(out, 0, sizeof(*out));

	cJSON *root = cJSON_Parse(json_str);
	if (!root) {
		if (errbuf && errbuf_sz)
			snprintf(errbuf, errbuf_sz, "Invalid JSON");
		return false;
	}

	bool ok = false;
	// name
	const cJSON *name = cJSON_GetObjectItemCaseSensitive(root, "name");
	if (!cJSON_IsString(name) || !name->valuestring) {
		if (errbuf && errbuf_sz)
			snprintf(errbuf, errbuf_sz, "Missing/invalid 'name'");
		cJSON_Delete(root);
		return false;
	}
	snprintf(out->name, sizeof(out->name), "%s", name->valuestring);

	// buff (bytes)
	const cJSON *buff = cJSON_GetObjectItemCaseSensitive(root, "value");

	if (!cJSON_IsString(buff) || !buff->valuestring) {
		if (errbuf && errbuf_sz)
			snprintf(errbuf, errbuf_sz, "Missing/invalid 'name'");
		cJSON_Delete(root);
		return false;
	}
	snprintf((char*)out->buff, sizeof(out->buff), "%s", buff->valuestring);

	ok = true;

	cJSON_Delete(root);
	return ok;
}

/**
 * @brief Add a new MIB entry into the global MIB map.
 *
 * Registers a Management Information Base (MIB) entry with the
 * SNMP agent. The entry is inserted into the global @c mib_map
 * if space is available and no duplicate @c msg_id exists.
 *
 * @param entry Pointer to the MIB entry structure to insert.
 *
 * @retval MIB_OK            Successfully inserted.
 * @retval MIB_ERR_FULL      Table full; cannot insert.
 * @retval MIB_ERR_DUPLICATE Entry with same msg_id already exists.
 */
mib_status_t add_mib_entry(mib_entry_t *entry) {
	for (uint8_t i = 0; i < MAX_MIB_ENTRY; i++) {
		if (mib_map.entry[i] != NULL) {
			if (strncmp(mib_map.entry[i]->name, entry->name,
					sizeof(entry->name)) == 0) {
				return MIB_ERR_DUPLICATE;
			}
		} else {
			mib_map.entry[i] = entry;
			mib_map.count++;
			return MIB_OK;
		}
	}
	return MIB_ERR_FULL;
}

/**
 * @brief Find a MIB entry by its message identifier.
 *
 * Searches the global @c mib_map table for an entry matching
 * the specified @p name string identifier.
 *
 * @param name string identifier for an OID
 *
 * @return Pointer to the matching MIB entry, or NULL if not found.
 */
mib_entry_t* find_mib_entry_by_name(const char* name) {
	for (uint8_t i = 0; i < MAX_MIB_ENTRY; i++) {
		if (mib_map.entry[i] && !strcmp(mib_map.entry[i]->name, name)) {
			return mib_map.entry[i];
		}
	}
	return NULL; /* not found */
}

/**
 * @brief Callback for Net-SNMP log messages.
 *
 * Receives log events from the Net-SNMP library and forwards them
 * to the application’s logging system or broker.
 *
 * @param major     Major identifier of the log event type.
 * @param minor     Minor identifier providing additional context.
 * @param serverarg Pointer to server-provided log data (type depends on event).
 * @param clientarg Pointer to client-provided context (user data).
 *
 * @return Always 0 to indicate successful handling.
 *
 * @note This callback must be registered with Net-SNMP using
 *       snmp_register_callback().
 */
int snmp_log_callback(int major, int minor, void *serverarg, void *clientarg) {
	(void) major;
	//TODO add assert (clientarg != NULL)
	if ((clientarg == NULL) || (serverarg == NULL))
		return 1;

	snmp_agent_ao_t *me = (snmp_agent_ao_t*) clientarg;
	struct snmp_log_message *m = (struct snmp_log_message*) serverarg;
	const char *msg = m->msg ? m->msg : "";
	if (minor != SNMP_CALLBACK_LOGGING)
		return 1;

	if (strstr(msg, "AgentX subagent connected")) {
		pthread_mutex_lock(&me->agent_mtx);
		me->agent_inited = 1;
		pthread_mutex_unlock(&me->agent_mtx);
		message_frame_t evt = { .signal = SNMP_CHANGE_STATE_ERRINT(0) };
		memcpy(evt.payload, msg, strlen(msg));
		evt.length = strlen(msg);
		post((base_obj_t*) me, evt);
		// post log to broker
	} else if (strstr(msg, "AgentX master disconnected us")
			|| strstr(msg, "Failed to connect to the agentx master agent")
			|| strstr(msg, "AgentX master agent failed to respond to ping")) {
		pthread_mutex_lock(&me->agent_mtx);
		me->agent_inited = 0;
		pthread_mutex_unlock(&me->agent_mtx);
		message_frame_t evt = { .signal = SNMP_CHANGE_STATE_ERR(0) };
		memcpy(evt.payload, msg, strlen(msg));
		evt.length = strlen(msg);
		post((base_obj_t*) me, evt); // Signal self to transition to error state
	}
	return 1;
}

/**
 * @brief Append a delegated SNMP GET request to the agent’s pending list.
 *
 * Stores a reference to a delegated GET request in the agent’s
 * internal tracking structure so it can later be matched against
 * a corresponding response.
 *
 * @param me Pointer to the SNMP agent active object instance.
 * @param pg Pointer to the pending GET structure to append.
 *
 * @note This function does not take ownership of @p pg memory;
 *       the caller must ensure its lifetime until completion.
 */
void snmp_append_delegate_agent(snmp_agent_ao_t *me, pending_get_t *pg) {
	//TODO add assert me
	if (me != NULL) {
		pthread_mutex_lock(&me->pending_mtx);
		pg->next = me->pending_head;
		me->pending_head = pg;
		me->pg_count++;
		pthread_mutex_unlock(&me->pending_mtx);
	}
}

/**
 * @brief Find a delegated SNMP GET request by correlation ID.
 *
 * Searches the agent’s pending delegated GET list for an entry
 * matching the specified correlation identifier.
 *
 * @param me   Pointer to the SNMP agent active object instance.
 * @param corr Correlation ID associated with a delegated request.
 *
 * @return Pointer to the matching pending GET structure,
 *         or NULL if no match is found.
 *
 * @note The returned pointer is owned by the agent and must
 *       not be freed by the caller.
 */
pending_get_t* snmp_find_delegate_agent(snmp_agent_ao_t *me, const char *name) {
	//TODO add assert me, me->pending_head
	pthread_mutex_lock(&me->pending_mtx);
	pending_get_t **pp = &me->pending_head, *p = *pp;
	while (p) {
		if (!strcmp(p->name, name)) {
			*pp = p->next;
			me->pg_count--;
			break;
		}
		pp = &p->next;
		p = *pp;
	}
	pthread_mutex_unlock(&me->pending_mtx);
	return p;
}

/**
 * @brief Clear and release all delegated SNMP GET requests.
 *
 * Iterates through the agent’s pending delegated GET list,
 * removes all entries, and frees associated resources.
 * Used during shutdown or error recovery to ensure no
 * dangling delegated requests remain.
 *
 * @param me Pointer to the SNMP agent active object instance.
 *
 * @note Safe to call even if the list is empty.
 */
void snmp_clear_delegate_list(snmp_agent_ao_t *me) {
	pthread_mutex_lock(&me->pending_mtx);

	pending_get_t *p = me->pending_head;
	while (p) {
		pending_get_t *next = p->next;
		free(p);
		p = next;
		me->pg_count--;
	}
	me->pending_head = NULL;

	pthread_mutex_unlock(&me->pending_mtx);
}

/**
 * @brief Callback invoked when an asynchronous SNMP GET completes.
 *
 * Called by the Net-SNMP library after a delegated or asynchronous
 * GET operation finishes. The callback typically matches the result
 * with a pending request and updates the AO state accordingly.
 *
 * @param clientreg Registration identifier assigned by Net-SNMP for this callback.
 * @param clientarg User-supplied argument provided at registration time
 *                  (usually a pointer to a pending_get_t or context structure).
 *
 * @note Runs in the Net-SNMP callback context, not directly in the AO thread.
 */
void complete_get_cb(unsigned int clientreg, void *clientarg) {
	(void) clientreg;
	pending_get_t *pg = (pending_get_t*) clientarg;
	if (!pg || !pg->cache) {
		free(pg);
		return;
	}

	/* Retrieve the cached request context; bail if expired. */
	netsnmp_delegated_cache *cache = netsnmp_handler_check_cache(pg->cache);
	if (!cache) {
		free(pg->val);
		free(pg);
		return;
	}

	netsnmp_request_info *requests = cache->requests;
	netsnmp_agent_request_info *reqinfo = cache->reqinfo;

	/* Mark no longer delegated; we are finishing it now. */
	requests->delegated = 0;

	switch (reqinfo->mode) {
	case MODE_GET:
	case MODE_GETNEXT:
		if (pg->val && pg->val_len) {
			snmp_set_var_typed_value(requests->requestvb,
			(u_char)pg->asn_type,(const u_char*) pg->val,
					pg->val_len);
		} else {
			/* No data -> map to NOSUCHOBJECT for simple scalar example. */
			netsnmp_set_request_error(reqinfo, requests, SNMP_NOSUCHOBJECT);
		}
		break;

#ifndef NETSNMP_NO_WRITE_SUPPORT
		/* For SET, you would handle the phase-specific completion here. */
	case MODE_SET_RESERVE1:
	case MODE_SET_RESERVE2:

	case MODE_SET_COMMIT:
	case MODE_SET_FREE:
	case MODE_SET_UNDO:
		if (!pg->val) {
			netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_GENERR);
		}
		break;
#endif
	default:
		break;
	}

	netsnmp_free_delegated_cache(cache);
	free(pg->val);
	free(pg);
}

/**
 * @brief Schedule completion handling for a pending SNMP GET request.
 *
 * Queues or triggers follow-up processing for the given delegated
 * GET operation once its result becomes available. Ensures that the
 * completion is executed in the AO’s thread context.
 *
 * @param pg Pointer to the pending GET structure to complete.
 *
 * @note This function does not free @p pg; the caller or completion
 *       logic remains responsible for resource cleanup.
 */
void schedule_get_completion(pending_get_t *pg) {
	snmp_alarm_register(0, 0, complete_get_cb, pg);
}

/**
 * @brief Initialize the SNMP agent active object.
 *
 * Sets up the Net-SNMP environment, registers supported MIBs,
 * and prepares the agent to handle incoming SNMP requests.
 *
 * @param me Pointer to the SNMP agent active object instance.
 *
 * @retval true  Initialization completed successfully.
 * @retval false Initialization failed due to setup or registration errors.
 */
bool snmp_agent_init(snmp_agent_ao_t *me) {
	if (me->agent_inited)
		return true;

	if (me->cfg.stderr_log)
		snmp_enable_stderrlog();
	else
		snmp_disable_log();
	snmp_enable_calllog();
	snmp_register_callback(SNMP_CALLBACK_LIBRARY,
	SNMP_CALLBACK_LOGGING, snmp_log_callback, me);
	if (me->cfg.subagent) {

		netsnmp_enable_subagent();
		if (me->cfg.agentx_socket && me->cfg.agentx_socket[0]) {
			setenv("AGENTX_SOCKET", me->cfg.agentx_socket, 1);
			netsnmp_ds_set_string(NETSNMP_DS_APPLICATION_ID,
			NETSNMP_DS_AGENT_X_SOCKET, me->cfg.agentx_socket);
		}
	}
	const char *app = me->cfg.app_name ? me->cfg.app_name : "snmp_agent";

	if (init_agent(app) != 0) {
		return false;
	}

	/* Switch to master if requested (requires UDP :161 privileges). */
	if (!me->cfg.subagent) {
		if (init_master_agent() != 0) {
			return false;
		}
	}
	setenv("MIBS", "HPA-MIB", 1); //important in order for read_mib to consider path
	init_snmp(app);
	agent_check_and_process(0); /* 100 ms */
	if (me->agent_inited == 0) {
		return false;
	}
	init_mib();

	struct tree *root = read_mib("/usr/share/snmp/mibs/HPA-MIB.mib");
	if (!root) {
		pthread_mutex_lock(&me->agent_mtx);
		me->agent_inited = 0;
		pthread_mutex_unlock(&me->agent_mtx);
		return false;
	}
	cJSON *jarray = cJSON_CreateArray();
	tree_to_json(root, "", jarray);
	char *outstr = cJSON_Print(jarray);
	if (outstr == NULL) {
		pthread_mutex_lock(&me->agent_mtx);
		me->agent_inited = 0;
		pthread_mutex_unlock(&me->agent_mtx);
		return false;
	}
	register_from_json(me, outstr);
	cJSON_Delete(jarray);
	free(outstr);
	free(root);

	return true;
}

/**
 * @brief Shut down the SNMP agent active object.
 *
 * Cleans up Net-SNMP resources, unregisters handlers,
 * and terminates the agent’s internal threads or event loop.
 * Should be called before destroying or restarting the AO.
 *
 * @param me Pointer to the SNMP agent active object instance.
 *
 * @note After shutdown, @p me must be reinitialized with
 *       snmp_agent_init() before reuse.
 */
void snmp_agent_shutdown(snmp_agent_ao_t *me) {
	if (!me->agent_inited)
		return;
	const char *app = me->cfg.app_name ? me->cfg.app_name : "snmp_agent";
	topic_config_t config_op[] = { { .topic = SNMP_CHANGE_STATE_ERRINT(0),
			.type = EXACT_MATCH } };
	broker_subscribe(((base_obj_t*) me)->broker, config_op, 1,
			(base_obj_t*) me);
	shutdown_agent();
	snmp_shutdown(app);
	pthread_mutex_lock(&me->agent_mtx);
	me->pump_running = 0;
	me->agent_inited = 0;
	pthread_mutex_unlock(&me->agent_mtx);
}

/* -------------------------------------------------------------------------- */
/*                               Pump thread                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Background pump thread for the SNMP agent.
 *
 * Runs the Net-SNMP event loop in a dedicated thread, allowing
 * the agent to process requests asynchronously from the AO’s
 * main FSM thread.
 *
 * @param arg Pointer to the SNMP agent active object instance (snmp_agent_ao_t*).
 *
 * @return Always NULL when the thread terminates.
 *
 * @note This thread should be stopped gracefully during shutdown
 *       using snmp_agent_shutdown() to ensure resources are released.
 */
void* snmp_pump(void *arg) {
	//TODO add assert (arg != NULL)
	snmp_agent_ao_t *me = (snmp_agent_ao_t*) arg;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (me->pump_running) {
		(void) agent_check_and_process(1); /* BLOCK until I/O or Net-SNMP alarm */
		pthread_testcancel();
	}
	return NULL;
}

/**
 * @brief Handle SNMP scalar object requests.
 *
 * Called by the Net-SNMP agent when a managed scalar object
 * associated with this handler is accessed. Processes GET/SET
 * operations based on the registered MIB entry.
 *
 * @param handler  Pointer to the Net-SNMP MIB handler.
 * @param reginfo  Registration info for this handler.
 * @param reqinfo  Request info (GET/SET context).
 * @param requests Linked list of SNMP request structures.
 *
 * @return SNMP error/status code indicating success or failure.
 */
int snmp_scalar_handler(netsnmp_mib_handler *handler,
		netsnmp_handler_registration *reginfo,
		netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests) {
	//TODO add assert to (reginfo->my_reg_void != NULL)
	snmp_agent_ao_t *me = (snmp_agent_ao_t*) reginfo->my_reg_void;
	message_frame_t evt = { 0 };
	switch (reqinfo->mode) {
	case MODE_GET:
	case MODE_GETNEXT:{
		requests->delegated = 1;
		pending_get_t *pg = (pending_get_t*) calloc(1, sizeof(*pg));
		if (!pg) {
			netsnmp_set_request_error(reqinfo, requests,
			SNMP_ERR_RESOURCEUNAVAILABLE);
			return SNMP_ERR_GENERR;
		}
		memcpy((char*)pg->name,handler->handler_name,strlen(handler->handler_name));
		pg->cache = netsnmp_create_delegated_cache(handler, reginfo, reqinfo,
				requests, NULL);
		pg->asn_type = requests->requestvb->type;
		snmp_append_delegate_agent(me, pg);
		mib_entry_t *entry = find_mib_entry_by_name(handler->handler_name);
		if(!entry){
			return SNMP_ERR_NOERROR;
		}

		evt.signal = SNMP_GET_TX(entry->msg_id[7] << 8 | (entry->msg_id[8] &0xFF));
		char* payload = snmp_json_str(handler->handler_name,0,NULL);
		if(payload){
			memcpy(evt.payload,payload,strlen(payload));
			evt.length = strlen(payload);
			broker_post(me->super.broker, evt, PRIMARY_QUEUE);

		}
		//TODO handle error here GET
		free(payload);
	}
		break;
	case MODE_SET_ACTION:{
		mib_entry_t *entry = find_mib_entry_by_name(handler->handler_name);
		evt.signal = SNMP_SET_VALUE(entry->msg_id[7] << 8 | (entry->msg_id[8] &0xFF));
		char* payload = snmp_json_str(handler->handler_name,0,(char*)requests->requestvb->val.string);
		if(payload){
			memcpy(evt.payload,payload,strlen(payload));
			evt.length = strlen(payload);
			broker_post(me->super.broker, evt, PRIMARY_QUEUE);
		}
		//TODO handle error here SET
		free(payload);
	}
		break;
#ifndef NETSNMP_NO_WRITE_SUPPORT
	case MODE_SET_RESERVE1:
	case MODE_SET_RESERVE2:
	case MODE_SET_COMMIT:
	case MODE_SET_FREE:
	case MODE_SET_UNDO:
		netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_GENERR);
		break;
#endif
	}
	return SNMP_ERR_NOERROR;
}

/* -------------------------------------------------------------------------- */
/*                              Trap helper                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Send a v2 trap/notification by OID string (coldStart, linkDown, etc.).
 * @param me            SNMP AO (must be running).
 * @param trap_oid_str  OID string such as "1.3.6.1.6.3.1.1.5.1".
 */
void snmp_agent_send_trap(const snmp_agent_ao_t *me, const char *trap_oid_str) {
	if (!me || !me->agent_inited || !trap_oid_str)
		return;

	oid trap_oid[MAX_OID_LEN];
	size_t trap_len = MAX_OID_LEN;
	if (!read_objid(trap_oid_str, trap_oid, &trap_len))
		return;

	netsnmp_variable_list *vars = NULL;

	/* sysUpTime.0 */
	oid sysuptime_oid[] = { 1, 3, 6, 1, 2, 1, 1, 3, 0 };
	u_long uptime = get_uptime();  // hundredths of a second (TimeTicks)
	snmp_varlist_add_variable(&vars, sysuptime_oid, OID_LENGTH(sysuptime_oid),
	ASN_TIMETICKS, (const u_char*) &uptime, sizeof(uptime));

	/* snmpTrapOID.0 */
	oid snmptrap_oid[] = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0 };
	snmp_varlist_add_variable(&vars, snmptrap_oid, OID_LENGTH(snmptrap_oid),
	ASN_OBJECT_ID, (const u_char*) trap_oid, trap_len * sizeof(oid));

	send_v2trap(vars);
	snmp_free_varbind(vars);
}

/**
 * @brief FSM entry action for the SNMP agent’s initialization state.
 *
 * Called automatically when the FSM transitions into the
 * initialization state. Prepares the AO by resetting runtime
 * variables, clearing pending lists, and performing any
 * setup needed before moving to the operational state.
 *
 * @param fsm Pointer to the FSM instance for this AO.
 *
 * @note state transition signals are posted directly to itself
 * using base function post() from active_object
 */
void snmp_on_entry_initialisation(fsm_t *fsm) {
	// On Enter initiaisation, ao subscribs to SNMP_CHANGE_STATE_OP and SNMP_CHANGE_STATE_ERR
	topic_config_t config_op[] = { { .topic = SNMP_CHANGE_STATE_OP, .type =
			EXACT_MATCH }, { .topic = SNMP_CHANGE_STATE_ERR(0), .type =
			EXACT_MATCH } };
	snmp_agent_ao_t *me = (snmp_agent_ao_t*) fsm->super;
	message_frame_t evt = { 0 };

	broker_subscribe(((base_obj_t*) fsm->super)->broker, config_op, 2,
			((base_obj_t*) fsm->super));
	if (!snmp_agent_init(me)) {
		evt.signal = SNMP_CHANGE_STATE_ERR(0); // Signal self to transition to ERROR state
		post(((base_obj_t*) me), evt);
		return;
	}
	if (!me->pump_running) {
		pthread_mutex_lock(&me->agent_mtx);
		me->pump_running = 1;
		pthread_mutex_unlock(&me->agent_mtx);
		if (pthread_create(&me->pump_tid, NULL, snmp_pump, me) != 0) {
			pthread_mutex_lock(&me->agent_mtx);
			me->pump_running = 0;
			me->agent_inited = 0;
			pthread_mutex_unlock(&me->agent_mtx);
			snmp_agent_shutdown(me);
			evt.signal = SNMP_CHANGE_STATE_ERR(0); // Signal self to transition to ERROR state
			post(((base_obj_t*) me), evt);
			return;
		}
		while (me->pump_tid == 0)
			; // Wait for the agent thread pump to be running
		evt.signal = SNMP_CHANGE_STATE_OP; // Signal self to transition to OPERATIONAL state
		post(((base_obj_t*) me), evt);
	}
}

/**
 * @brief FSM entry action for the SNMP agent’s operational state.
 *
 * Executed when the FSM transitions into the operational state.
 * Subscribs to signals that need to be handled in operating state.
 *
 * @param fsm Pointer to the FSM instance for this AO.
 */
void snmp_on_entry_operational(fsm_t *fsm) {
	// Upon successfull initialisation of snmp agent, subscrib ao to SNMP_GET_VALUE signals
	topic_config_t config[] = { { .topic = SNMP_GET_RX(0), .start = SNMP_GET_RX(
			0), .type = MASK }, };
	broker_subscribe(((base_obj_t*) fsm->super)->broker, config, 1,
			(base_obj_t*) fsm->super);
}

/**
 * @brief FSM entry action for the SNMP agent’s error state.
 *
 * Executed when the FSM transitions into the error state.
 * Used to record diagnostics, notify the system of failure,
 * and prepare the AO for potential recovery or shutdown.
 *
 * Unsubscribs to signals handled during operating state
 * Subscribs to signals handled during error state
 *
 * @param fsm Pointer to the FSM instance for this AO.
 *
 * @note Keeps the AO in a safe state until an explicit
 *       recovery or reset event is processed.
 */
void snmp_on_entry_error(fsm_t *fsm) {
	snmp_agent_ao_t *me = (snmp_agent_ao_t*) fsm->super;
	topic_config_t config[] = { { .topic = SNMP_GET_RX(0), .start = SNMP_GET_RX(
			0), .type = MASK }, };
	broker_unsubscribe(((base_obj_t*) fsm->super)->broker, config, 1,
			(base_obj_t*) fsm->super);
	snmp_clear_delegate_list(me);
	if (me->pump_running) {
		pthread_mutex_lock(&me->agent_mtx);
		me->pump_running = 0;
		pthread_mutex_unlock(&me->agent_mtx);
		snmp_agent_shutdown(me);
	}
	// finally, log this to broker
}

/**
 * @brief FSM handler for the SNMP agent’s operational state.
 *
 * Processes broker events while the SNMP agent is running normally.
 * Typical events include requests to start delegated operations,
 * handle responses, or update MIB entries.
 *
 * @param fsm   Pointer to the FSM instance for this AO.
 * @param event Pointer to the incoming broker event frame.
 *
 * @note Runs in the AO’s thread context and must remain non-blocking.
 */
void snmp_operational_handler(fsm_t *fsm, const message_frame_t *event) {
	snmp_agent_ao_t *me = (snmp_agent_ao_t*) fsm->super;
	switch (event->signal) {
		case SNMP_GET_RX(0) ... SNMP_GET_RX(0xFFFF): {
			snmp_payload_t snmp_payload = {0};
			if(!snmp_payload_from_json((char*)event->payload,&snmp_payload,NULL,0))return;
			mib_entry_t *entry = find_mib_entry_by_name(snmp_payload.name);
			if(!entry)return;
			pending_get_t *pg = snmp_find_delegate_agent(me, entry->name);
			if (!pg)return;

			pg->asn_type = (int) entry->asn_type;
			uint8_t len = (uint8_t)strlen((char*)snmp_payload.buff);
			if (len) {
				pg->val = (char*) malloc(len);
				if (pg->val) {
					memcpy(pg->val, snmp_payload.buff, len);
					pg->val_len = len;
					schedule_get_completion(pg);
				}
			}
			break;
		}
	}
}

/**
 * @brief FSM handler for the SNMP agent’s error state.
 *
 * Invoked when the SNMP AO enters an error condition. This handler
 * may process recovery events, ignore most traffic, or trigger
 * a state transition back to initialization or operational states.
 *
 * @param fsm   Pointer to the FSM instance for this AO.
 * @param event Pointer to the incoming broker event frame.
 *
 * @note Typically logs or reports errors; business logic may
 *       decide whether to reset or halt the AO.
 */
void snmp_error_handler(fsm_t *fsm, const message_frame_t *event) {
	snmp_agent_ao_t *me = (snmp_agent_ao_t*) fsm->super;
	switch (event->signal) {
	case SNMP_CHANGE_STATE_ERRINT(0): {
		pthread_mutex_lock(&me->agent_mtx);
		me->pump_running = 1;
		pthread_mutex_unlock(&me->agent_mtx);
		topic_config_t config_op[] = { { .topic = SNMP_CHANGE_STATE_OP, .type =
				EXACT_MATCH }, { .topic = SNMP_CHANGE_STATE_ERR(0), .type =
				EXACT_MATCH } };

		broker_subscribe(((base_obj_t*) fsm->super)->broker, config_op, 2,
				((base_obj_t*) fsm->super));
		message_frame_t evt = { .signal = SNMP_CHANGE_STATE_OP };
		post((base_obj_t*) me, evt);
	}
		break;
	}
}

/**
 * @brief Broker dispatch entry point for the SNMP Active Object.
 *
 * Routes an incoming broker message to the object's finite-state machine (FSM)
 * for handling. Called by the framework when a message addressed to this AO arrives.
 *
 <<<<<<< Upstream, based on origin/main
 * @param me     Base active-object instance (cast to the concrete SNMP AO).
 =======
 * @param me    Base active-object instance (cast to the concrete SNMP AO).
 >>>>>>> 9503705 syncing after merge request
 * @param frame Immutable message frame received from the broker.
 *
 * @pre me != NULL
 * @pre frame != NULL
 * @note Must be non-blocking; heavy work is scheduled on the AO's thread
 *       (via the FSM) or delegated to the SNMP agent loop.
 * @see fsm_handler()
 */
void dispatch(base_obj_t *const me, const message_frame_t *frame) {
	//TODO add assert for pointer to me and frame
	fsm_handler(&me->fsm, frame);
}

/* -------------------------------------------------------------------------- */
/*                                 Constructor                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Construct the SNMP AO and bind it to the framework.
 *
 * Wires the file-local @c dispatch into the AO vtable via @c INIT_BASE (like ao_system.c),
 * initializes the FSM to IDLE, and copies @ref snmp_agent_cfg_t with defaults.
 *
 * @param me      Storage for the AO.
 * @param broker  Broker instance used by the AO framework.
 * @param name    Name of the AO.
 * @param cfg     Optional configuration; may be NULL for defaults.
 */
void snmp_agent_ctor(snmp_agent_ao_t *const me, broker_t *broker, char *name,
		const snmp_agent_cfg_t *cfg) {
	INIT_BASE(me, broker, name, system_id, NULL); /* subscribes and binds this TU's dispatch */
	MsgQueue_Init(&me->super.msgQueue);
	me->super.initialisation_state = &snmp_initialisation_state;
	/* Defaults */
	me->cfg.app_name = name; /* shows in Net-SNMP logs */
	me->cfg.subagent = true; /* AgentX subagent mode (recommended) */
	me->cfg.agentx_socket = "/var/agentx/master"; /* default agentx socket; adjust if needed */
	;
	me->cfg.stderr_log = true; /* log to stderr for easy debugging */
	if (cfg)
		me->cfg = *cfg;

	me->pump_tid = (pthread_t) 0;
	me->pump_running = 0;
	me->agent_inited = 0;

	pthread_mutex_init(&me->pending_mtx, NULL);
	pthread_mutex_init(&me->agent_mtx, NULL);
	me->pending_head = NULL;

}

#endif /* __linux__ */

