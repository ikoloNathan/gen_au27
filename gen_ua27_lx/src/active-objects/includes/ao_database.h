/**
 * @file ao_database.h
 * @brief Active Object Database interface.
 *
 * This module defines the database active object, which integrates
 * a SQLite database into the Active Object framework. It exposes
 * constructor and state machine definitions that allow the database
 * to behave like any other active object within the system.
 *
 * The database active object:
 * - Maintains a persistent connection to a SQLite database.
 * - Validates required tables during initialization.
 * - Responds to broker messages to query or update tables.
 * - Transitions between initialization, operational, and error states.
 *
 * Typical usage:
 * @code
 * db_obj_t db;
 * db_ctor(&db, broker, "DatabaseAO");
 * AO_Start(&db.super); // start active object loop
 * @endcode
 *
 *
 * @author Nathan Ikolo
 * @date Aug 26, 2025
 */

#ifndef ACTIVE_OBJECTS_INCLUDES_AO_DATABASE_H_
#define ACTIVE_OBJECTS_INCLUDES_AO_DATABASE_H_

#include "active_object.h"
#include <fsm.h>
#include <sqlite3.h>

/**
 * @struct db_obj_t
 * @brief Database active object instance.
 *
 * Extends @ref base_obj_t with an embedded SQLite handle.
 * The object operates as an active object within the broker,
 * subscribing to database-related topics and handling events.
 */
typedef struct {
    base_obj_t super;   /**< Base active object instance. */
    sqlite3 *db;        /**< SQLite3 database handle, valid in operational state. */
} db_obj_t;

/**
 * @brief Construct a database active object.
 *
 * Initializes the base object fields, sets up its internal
 * message queue, and assigns the initial state to
 * @ref db_initialisation_state.
 *
 * @param me Pointer to the database active object instance.
 * @param broker Pointer to the broker used for publish/subscribe.
 * @param name Human-readable object name for logging/identification.
 */
void db_ctor(db_obj_t *const me, broker_t *broker, char *name);

/**
 * @brief FSM state for database initialization.
 *
 * This state attempts to open the database, validate required tables,
 * and transition to @ref db_operational_state if successful.
 */
extern state_t db_initialisation_state;

/**
 * @brief FSM state for operational behavior.
 *
 * In this state, the database object subscribes to read/write topics
 * and services requests through the broker.
 */
extern state_t db_operational_state;

/**
 * @brief FSM state for database error handling.
 *
 * Entered when the database cannot be opened or required tables
 * are missing. The system may retry initialization or escalate errors.
 */
extern state_t db_error_state;

#endif /* ACTIVE_OBJECTS_INCLUDES_AO_DATABASE_H_ */
