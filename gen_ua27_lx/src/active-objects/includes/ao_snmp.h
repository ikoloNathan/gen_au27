/*
 * ao_snmp.h
 *
 *  Created on: Aug 26, 2025
 *      Author: Nathan Ikolo
 */

#ifndef ACTIVE_OBJECTS_INCLUDES_AO_SNMP_H_
#define ACTIVE_OBJECTS_INCLUDES_AO_SNMP_H_

/**
 * @file ao_snmp.h
 * @brief Net-SNMP Agent Active Object for Linux (FSM + broker-bridged GET/SET).
 *
 * This Active Object embeds a Net-SNMP agent and integrates it with the broker.
 * - The agent is pumped by a dedicated thread blocking in agent_check_and_process(1).
 * - SNMP GET/SET are routed asynchronously via the broker using Net-SNMP's
 *   delegated request pattern. See ao_snmp.c for payload formats and details.
 *
 * @note This unit requires Linux and Net-SNMP development headers/libs.
 *       Link with: `$(net-snmp-config --cflags --agent-libs)`.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/agent_handler.h>
#include <net-snmp/agent/util_funcs.h>
#include <net-snmp/mib_api.h>
#include <net-snmp/library/mib.h>
#include <stdatomic.h>
#include "active_object.h"
#include "broker.h"
#include "fsm.h"

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup AO_SNMP SNMP Agent Active Object
 * @brief AO that runs a Net-SNMP agent and bridges requests via the broker.
 * @{
 */


/**
 * @brief Pending GET request tracked between delegation and broker response.
 *
 * We keep the delegated cache from Net-SNMP to complete the request later on
 * the agent thread, plus the correlation ID used on the broker and the value
 * returned by the backend.
 */
typedef struct pending_get {
    uint32_t                  corr;   /**< Correlation ID for broker round-trip. */
    netsnmp_delegated_cache  *cache;  /**< Net-SNMP delegated cache (completion handle). */
    int                       asn_type; /**< ASN.1 type to set in varbind. */
    char                   	 *val;    /**< Value copied from broker response (malloc'd). */
    size_t                    val_len;/**< Length in bytes of @ref val. */
    struct pending_get       *next;   /**< Next in singly-linked list. */
} pending_get_t;


/**
 * @brief Configuration for the SNMP agent AO.
 */
typedef struct {
    const char *app_name;      /**< Application name for Net-SNMP (`init_agent/init_snmp`). */
    bool        subagent;      /**< `true` = AgentX subagent; `false` = master agent. */
    const char *agentx_socket; /**< Optional AgentX socket path; `NULL` for default. */
    bool        stderr_log;    /**< Log to stderr (useful during bring-up). */
} snmp_agent_cfg_t;

/**
 * @struct snmp_agent_ao
 * @brief Active Object encapsulating an SNMP agent instance.
 *
 * Wraps the Net-SNMP engine inside the Active Object framework,
 * providing lifecycle management, asynchronous request handling,
 * and integration with the broker/FSM system.
 */
typedef struct snmp_agent_ao {
    base_obj_t       super;        /**< Base Active Object (must be first). */

    fsm_t            fsm;          /**< FSM managing the agent lifecycle. */

    snmp_agent_cfg_t cfg;          /**< Configuration (copied at construction). */

    pthread_t        pump_tid;     /**< Pump thread (runs agent_check_and_process). */
    volatile int     pump_running; /**< Non-zero while the pump thread should run. */
    volatile int     agent_inited; /**< Flag indicating Net-SNMP engine is initialized. */

    /* Bookkeeping for asynchronous broker-bridged transactions is private to the .c. */
    pthread_mutex_t     pending_mtx;  	/**< Protects pending async GET/SET lists. */
    pending_get_t *pending_head; 		/**< Singly-linked list of pending GETs. */
    uint16_t pg_count;					/**< Track number of pending GETs within the list */
    uint32_t            next_corr;    	/**< Correlation ID generator for broker RPC. */
} snmp_agent_ao_t;


/**
 * @brief Construct an SNMP Agent Active Object.
 *
 * Binds the AO to @p broker and @p name, wires the local @c dispatch into the vtable
 * (like @c ao_system.c), initializes the FSM to IDLE, and stores @p cfg or defaults.
 *
 * @param[in,out] me      Storage for the AO instance (not NULL).
 * @param[in]     broker  Broker to use for pub/sub and posting messages.
 * @param[in]     name    Human-readable AO name.
 * @param[in]     cfg     Optional configuration; if NULL, sensible defaults are used.
 *
 * @post The AO is constructed in the IDLE state;
 */
void snmp_agent_ctor(snmp_agent_ao_t * const me,
                     broker_t *broker,
                     char *name,
                     const snmp_agent_cfg_t *cfg);

/**
 * @brief Send a v2 trap/notification identified by @p trap_oid_str.
 *
 * @param[in] me            The SNMP AO (must be running / initialized).
 * @param[in] trap_oid_str  OID string like "1.3.6.1.6.3.1.1.5.1" (coldStart).
 *
 * @note This helper builds standard varbinds (sysUpTime.0 and snmpTrapOID.0)
 *       and calls @c send_v2trap().
 */
void snmp_agent_send_trap(const snmp_agent_ao_t *me, const char *trap_oid_str);


/**
 * @brief FSM state for snmp initialization.
 *
 * This state attempts to open the database, validate required tables,
 * and transition to @ref db_operational_state if successful.
 */
extern state_t snmp_initialisation_state;

/**
 * @brief FSM state for operational behavior.
 *
 * In this state, the snmp object subscribes to signals
 * and services requests through the broker.
 */
extern state_t snmp_operational_state;

/**
 * @brief FSM state for snmp error handling.
 */
extern state_t snmp_error_state;

/** @} */ /* end of group AO_SNMP */

#endif /* __linux__ */

#ifdef __cplusplus
}
#endif


#endif /* ACTIVE_OBJECTS_INCLUDES_AO_SNMP_H_ */
