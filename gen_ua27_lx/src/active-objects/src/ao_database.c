/**
 * @file ao_database.c
 * @brief Active Object Database implementation.
 *
 * This file implements the finite state machine (FSM) for the
 * database active object. It integrates SQLite with the Active
 * Object + Broker framework, handling:
 * - Opening and validating the database during initialization.
 * - Subscribing to broker topics in operational state.
 * - Unsubscribing and signaling errors in failure conditions.
 *
 * The database expects a fixed schema with the following tables:
 * - "ADC_THRESHOLDS"
 * - "SYSTEM_INFOS"
 *
 * @date Aug 26, 2025
 * @author Nathan Ikolo
 */

#include <string.h>
#include <sys_defns.h>
#include <stdbool.h>
#include <stdlib.h>
#include <cjson/cJSON.h>
#include "ao_database.h"
#include "broker.h"

/// Path to the SQLite database file used by this active object.
#define DATABASE_PATH "test.db"

/// Maximum number of tables expected and validated at startup.
#define MAX_TABLES	1

/**
 * @struct db_tables_t
 * @brief Metadata describing required database tables.
 *
 * Each entry tracks a table name and whether it was validated
 * during the initialization sequence.
 */
typedef struct {
	char *tableName; /**< Name of the SQLite table. */
	bool valid; /**< True if the table exists. */
} db_tables_t;


/**
 * @enum db_error_t
 * @brief Error codes for database initialization.
 *
 * Used internally to categorize errors that occur during
 * database startup and validation.
 */
typedef enum {
	DB_OK, /**< No error occurred. */
	DB_OPEN_ERR, /**< Database could not be opened. */
	DB_SQL_ERR, /**< SQL query execution failed. */
	DB_TABLE_ERR /**< One or more required tables are missing. */
} db_error_t;

/// Internal static array listing required tables and validation state.
static db_tables_t db_tables[MAX_TABLES] = { { "ADC_THRESHOLDS", false }, { "SYSTEM_INFOS", false } };

/* --- Forward declarations --- */
static void on_enter_initialisation(fsm_t *fsm);
static void on_enter_operational(fsm_t *fsm);
static void on_exit_operational(fsm_t *fsm);

static void dispatch(base_obj_t *const me, const message_frame_t *frame);
static void db_initialisation_handler(fsm_t *fsm, const message_frame_t *event);
static void db_operational_handler(fsm_t *fsm, const message_frame_t *event);
static void db_error_handler(fsm_t *fsm, const message_frame_t *event);
static char* db_table_to_json(sqlite3 *db, const char *table);

/* --- Transition tables --- */
transition_t db_initialisation_transitions[] = { { DB_CHANGE_STATE_OP, &db_operational_state, NULL }, { DB_CHANGE_STATE_ERR, &db_error_state, NULL } };

transition_t db_operational_transitions[] = { { DB_CHANGE_STATE_ERR, &db_error_state, NULL }, { DB_CHANGE_STATE_INIT, &db_initialisation_state, NULL } };

transition_t db_error_transitions[] = { { DB_CHANGE_STATE_OP, &db_operational_state, NULL }, { DB_CHANGE_STATE_INIT, &db_initialisation_state, NULL } };

/**
 * @brief SQLite callback for validating tables.
 *
 * Executed by @c sqlite3_exec during initialization.
 * Compares discovered table names against the expected list
 * and marks entries as valid if found.
 *
 * @param NotUsed Unused user data pointer.
 * @param argc Number of columns in the result row.
 * @param argv Array of strings representing column values.
 * @param azColName Array of column names.
 * @return Always 0 to continue iteration.
 */
static int validate_tables(void *NotUsed, int argc, char **argv, char **azColName) {
	int i, j;
	for (i = 0; i < argc; i++) {
		for (j = 0; j < MAX_TABLES; j++) {
			if (strcmp(db_tables[j].tableName, argv[i]) == 0) {
				db_tables[j].valid = true;
			}
		}
	}
	return 0;
}

static int check_row_count(void *NotUsed, int argc, char **argv, char **azColName) {
	int *rows = (int*) NotUsed;
	if (argv != NULL) {
		*rows = atoi(argv[0]);
	}
	return 0;
}

/**
 * @brief Read all rows from a SQLite table and return them as JSON string.
 *
 * Each row is represented as a JSON object with column names as keys
 * and typed values (integer, double, string, null). Rows are collected
 * into a JSON array.
 *
 * @param db      Open SQLite database handle.
 * @param table   Table name to read (e.g. "ADC_THRESHOLDS").
 * @return char*  JSON string (must be freed by caller), or NULL on error.
 */
char* db_table_to_json(sqlite3 *db, const char *table) {
	sqlite3_stmt *stmt;
	char sql[512];
	snprintf(sql, sizeof(sql), "SELECT * FROM %s;", table);

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare SQL: %s\n", sqlite3_errmsg(db));
		return NULL;
	}

	// Create JSON array
	cJSON *array = cJSON_CreateArray();

	// Iterate over each row
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		cJSON *row = cJSON_CreateObject();

		int col_count = sqlite3_column_count(stmt);
		for (int i = 0; i < col_count; i++) {
			const char *col_name = sqlite3_column_name(stmt, i);
			int col_type = sqlite3_column_type(stmt, i);

			switch (col_type) {
				case SQLITE_INTEGER:
					cJSON_AddNumberToObject(row, col_name, sqlite3_column_int(stmt, i));
					break;
				case SQLITE_FLOAT:
					cJSON_AddNumberToObject(row, col_name, sqlite3_column_double(stmt, i));
					break;
				case SQLITE_TEXT:
					cJSON_AddStringToObject(row, col_name, (const char*) sqlite3_column_text(stmt, i));
					break;
				case SQLITE_NULL:
					cJSON_AddNullToObject(row, col_name);
					break;
				default:
					// Unknown type → fallback as string
					cJSON_AddStringToObject(row, col_name, (const char*) sqlite3_column_text(stmt, i));
					break;
			}
		}
		cJSON_AddItemToArray(array, row);
	}

	sqlite3_finalize(stmt);

	// Convert array to JSON string
	char *json_str = cJSON_PrintUnformatted(array);
	cJSON_Delete(array); // free JSON object

	return json_str;
}

/**
 * @brief Parse a JSON string of rows and extract fields.
 *
 * @param json_str The JSON string (array of row objects).
 */
void parse_json_rows(const char *json_str) {
	cJSON *root = cJSON_Parse(json_str);
	if (!root || !cJSON_IsArray(root)) {
		printf("Invalid JSON data\n");
		return;
	}

	int row_count = cJSON_GetArraySize(root);
	for (int i = 0; i < row_count; i++) {
		cJSON *row = cJSON_GetArrayItem(root, i);

		// Extract fields by name
		cJSON *id = cJSON_GetObjectItem(row, "ID");
		cJSON *num = cJSON_GetObjectItem(row, "NUM");
		cJSON *name = cJSON_GetObjectItem(row, "NAME");
		cJSON *multiplier = cJSON_GetObjectItem(row, "MULTIPLIER");

		// Defensive: check type before using
		if (cJSON_IsNumber(id) && cJSON_IsNumber(num) && cJSON_IsString(name) && cJSON_IsNumber(multiplier)) {
			printf("Row %d:\n", i + 1);
			printf("  ID          = %d\n", id->valueint);
			printf("  NUM         = %d\n", num->valueint);
			printf("  NAME        = %s\n", name->valuestring);
			printf("  MULTIPLIER  = %f\n", multiplier->valuedouble);
		}
	}

	cJSON_Delete(root);
}

/**
 * @brief Entry action for initialization state.
 *
 * - Attempts to open the SQLite database at @ref DATABASE_PATH.
 * - Executes a query to enumerate existing tables.
 * - Validates required tables against @ref db_tables.
 * - Posts success or error signals to the broker.
 *
 * Error cases:
 * - If the DB cannot be opened, transitions to @ref db_error_state.
 * - If SQL query fails, transitions to @ref db_error_state.
 * - If required tables are missing, transitions to @ref db_error_state.
 *
 * @param fsm Pointer to the finite state machine context.
 */
void on_enter_initialisation(fsm_t *fsm) {

	int rc = 0;
	char sql[256] = { };
	char *zErrMsg = 0;
	db_error_t options = DB_OK;
	message_frame_t msg;

	// Try to open the SQLite database file
	rc = sqlite3_open(DATABASE_PATH, &((db_obj_t*) fsm)->db);

	if (rc) {
		// If sqlite3_open failed, mark error
		options = DB_OPEN_ERR;
		goto post;
	} else {
		// Query to get all table names in the database
		strcpy(sql, "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");

		// Execute the query and call validate_tables() callback
		rc = sqlite3_exec(((db_obj_t*) fsm)->db, sql, validate_tables, 0, &zErrMsg);
		memset(sql, 0, sizeof(sql));

		if (rc != SQLITE_OK) {
			// SQL execution failed (e.g. bad schema)
			options = DB_SQL_ERR;
			goto post;
		}
		// Check if all required tables were found
		for (int i = 0; i < MAX_TABLES; i++) {
			if (!db_tables[i].valid) {
				options = DB_TABLE_ERR;	// missing required table
			} else {
				int rows = 0;
				snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM %s;", db_tables[i].tableName);
				rc = sqlite3_exec(((db_obj_t*) fsm)->db, sql, check_row_count, &rows, &zErrMsg);
				memset(sql, 0, sizeof(sql));
				options |= rows != 0 ? DB_OK : DB_TABLE_ERR;
			}
		}

		post: switch (options) {
			case DB_TABLE_ERR:
			case DB_OPEN_ERR:
				// Database missing required tables or failed to open
				// Send error signal back via broker
				msg.signal = AO_SIGNAL(SIG_SEVERITY_ERROR,SIG_STATE_ERROR, SIG_TYPE_DATABASE, 0);
				memcpy(msg.payload, "DB_ERROR", 8);
				msg.length = 8;

				// Subscribe to DB_CHANGE_STATE_ERR so we can transition to error state
				topic_config_t config_err[] = { { .topic = DB_CHANGE_STATE_ERR, .type = EXACT_MATCH }};
				broker_subscribe(((base_obj_t*) fsm->super)->broker, config_err, 1, ((base_obj_t*) fsm->super));

				// Close the DB connection since it's invalid
				sqlite3_close(((db_obj_t*) fsm)->db);
				break;
			default:
				// Database opened and tables validated successfully
				// Signal broker to transition to OPERATIONAL state
				msg.signal = DB_CHANGE_STATE_OP;
				topic_config_t config_op[] = { { .topic = DB_CHANGE_STATE_OP, .type = EXACT_MATCH }, { .topic = DB_CHANGE_STATE_ERR, .type = EXACT_MATCH } ,{ .topic = DB_READ_TABLE(0, 0), .start = DB_READ_TABLE(0, 0), .type = MASK }};
				broker_subscribe(((base_obj_t*) fsm->super)->broker, config_op, 3, ((base_obj_t*) fsm->super));
				break;
		}
		broker_post(((base_obj_t*) fsm->super)->broker, msg, PRIMARY_QUEUE);
	}
}

/**
 * @brief Entry action for operational state.
 *
 * Subscribes the database active object to broker topics
 * that correspond to database read requests.
 *
 * @param fsm Pointer to the finite state machine context.
 */
void on_enter_operational(fsm_t *fsm) {
	topic_config_t config[] = { { .topic = DB_READ_TABLE(0, 0), .start = DB_READ_TABLE(0, 0), .type = MASK }, };
	broker_unsubscribe(((base_obj_t*) fsm->super)->broker, config, 1, ((base_obj_t*) fsm->super));
	broker_subscribe(((base_obj_t*) fsm->super)->broker, config, 1, ((base_obj_t*) fsm->super));
}

/**
 * @brief Exit action for operational state.
 *
 * Unsubscribes the database active object from broker topics
 * when leaving the operational state.
 *
 * @param fsm Pointer to the finite state machine context.
 */
void on_exit_operational(fsm_t *fsm) {
	topic_config_t config[] = { { .topic = DB_READ_TABLE(0, 0), .start = DB_READ_TABLE(0, 0), .type = MASK }, };
	broker_unsubscribe(((base_obj_t*) fsm->super)->broker, config, 1, ((base_obj_t*) fsm->super));
}
/* --- STATE DEFINITIONS --- */
state_t db_initialisation_state = { .handler = db_initialisation_handler, .on_entry = on_enter_initialisation, .on_exit = NULL, .transitions = db_initialisation_transitions, .transition_count = 2 };

/**
 * @brief Transition table for Operational State.
 *
 * Defines possible transitions from Operational to other states.
 */
state_t db_operational_state = { .handler = db_operational_handler, .on_entry = on_enter_operational, .on_exit = on_exit_operational, .transitions = db_operational_transitions, .transition_count = 2 };

/**
 * @brief Transition table for Error State.
 *
 * Defines possible transitions from Operational to other states.
 */
state_t db_error_state = { .handler = db_error_handler, .on_entry = NULL, .on_exit = NULL, .transitions = db_error_transitions, .transition_count = 2 };

/**
 * @brief Initialization state handler.
 *
 * Processes events received while in initialization state.
 *
 * @param fsm Pointer to the FSM.
 * @param event Pointer to the incoming event.
 */
void db_initialisation_handler(fsm_t *fsm, const message_frame_t *event) {
	printf("Initialisations");
}

/**
 * @brief Operational state handler.
 *
 * Handles broker messages that request table reads.
 * Currently supports masked topic subscriptions.
 *
 * @param fsm Pointer to the FSM.
 * @param event Pointer to the incoming event.
 */
void db_operational_handler(fsm_t *fsm, const message_frame_t *event) {
	char sql[256];
	message_frame_t msg;
	uint8_t table_id = 0, row_id = 0;
	switch (event->signal) {
		// Handle all DB_READ_TABLE(x,y) requests
		case DB_READ_TABLE(0,0) ... DB_READ_TABLE(0xFF, 0xFF):
			// Extract which table and which row index from the encoded signal
			table_id = DB_GET_TABLE_ID(event->signal);
			row_id = DB_GET_ROW_IDX(event->signal);

			// Only process if table_id is within the valid range
			if (table_id < MAX_TABLES) {
				switch (row_id) {
					case 0:
					case 0xFF:
						// Special case: row_id = 0 or 0xFF means "return ALL rows"
						// SQL becomes: SELECT * FROM <table>;
						strcpy(sql, db_tables[table_id].tableName);
						break;
					default:
						// Otherwise: return a specific row identified by ID
						// SQL becomes: SELECT * FROM <table> WHERE ID = <row_id>;
						snprintf(sql, sizeof(sql), "%s WHERE ID = %d", db_tables[table_id].tableName, row_id);
						break;
				}
				const char *json_str = db_table_to_json(((db_obj_t*) fsm)->db, sql);
				msg.signal = DB_PUBLISH_TABLE(table_id, row_id);
				msg.ptr = (uint8_t*) json_str;
				msg.length = strlen(json_str);
				broker_post(((base_obj_t*) fsm->super)->broker, msg, PRIMARY_QUEUE);
			} else {
				msg.signal = DB_PUBLISH_TABLE(table_id,row_id) | (SIG_SEVERITY_ERROR << 24);
				sprintf((char*) msg.payload, "db:(table_id %d,row_id %d error)", table_id, row_id);
				msg.length = strlen((char*) msg.payload);
				broker_post(((base_obj_t*) fsm->super)->broker, msg, PRIMARY_QUEUE);
			}
			break;
	}
}

/**
 * @brief Error state handler.
 *
 * Logs that the database is in an error state.
 *
 * @param fsm Pointer to the FSM.
 * @param event Pointer to the incoming event.
 */
void db_error_handler(fsm_t *fsm, const message_frame_t *event) {
	printf("db_err\n");

}

/**
 * @brief Dispatch function for database active object.
 *
 * Forwards messages to the FSM handler of the base object.
 *
 * @param me Pointer to the base active object.
 * @param frame Pointer to the incoming message frame.
 */
void dispatch(base_obj_t *const me, const message_frame_t *frame) {
	fsm_handler(&me->fsm, frame);
}

/**
 * @brief Database active object constructor.
 *
 * Initializes the base object, sets up its message queue,
 * and assigns the initial state to @ref db_initialisation_state.
 *
 * @param me Pointer to the database object to construct.
 * @param broker Pointer to the broker for pub-sub messaging.
 * @param name Human-readable object name.
 */
void db_ctor(db_obj_t *const me, broker_t *broker, char *name) {
	INIT_BASE(me, broker, name, system_id, NULL);
	MsgQueue_Init(&me->super.msgQueue);
	me->super.initialisation_state = &db_initialisation_state;
}
