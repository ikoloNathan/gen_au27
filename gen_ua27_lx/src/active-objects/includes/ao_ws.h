#ifndef AO_WS_H
#define AO_WS_H

#include <stdint.h>
#include <pthread.h>
#include "active_object.h"
#include "fsm.h"
#include "broker.h"
#include "message.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifndef WS_MAX_CLIENTS
#define WS_MAX_CLIENTS  64
#endif
#ifndef WS_RX_BUFSZ
#define WS_RX_BUFSZ     4096
#endif
#ifndef WS_TX_BUFSZ
#define WS_TX_BUFSZ     1024
#endif
#ifndef WS_OUTQ_LEN
#define WS_OUTQ_LEN     16
#endif
#ifndef WS_DOCROOT_MAX
#define WS_DOCROOT_MAX  256
#endif

typedef enum {
	WS_CL_FREE = 0, WS_CL_HTTP, WS_CL_WS
} ws_cl_state_t;

typedef struct {
	int fd;
	ws_cl_state_t st;
	unsigned long long conn_id;

	/* HTTP request buffer */
	char rx[WS_RX_BUFSZ];

	/* HTTP non-blocking transmit (header+file bytes) */
	char *http_tx; /* malloc'd buffer with full response */
	size_t http_len; /* total bytes to send */
	size_t http_off; /* bytes sent already */

	/* WS per-client TX ring (text frames) */
	int out_head, out_tail;
	char out_q[WS_OUTQ_LEN][WS_TX_BUFSZ];
} ws_client_t;

typedef struct ao_ws {
	base_obj_t super; /* MUST be first */

	/* Config */
	uint16_t port;
	char docroot[WS_DOCROOT_MAX]; /* e.g., "./www" */

	/* Reactor state */
	int epfd;
	int listenfd;
	int notifyfd; /* eventfd for AO->pump wake */

	/* Thread */
	pthread_t pump_tid;
	volatile int pump_running;

	/* Clients */
	ws_client_t clients[WS_MAX_CLIENTS];
	unsigned long long id_seq;

	/* Cross-thread command queue (AO -> pump) */
	struct {
		int head, tail;
		struct {
			int target_idx;
			char msg[WS_TX_BUFSZ];
		} q[64];
		pthread_mutex_t mx;
	} cmd;
} ao_ws_t;

/* FSM states */
extern state_t ws_initialisation_state;
extern state_t ws_operational_state;
extern state_t ws_error_state;

/* Constructor */
void ws_ctor(ao_ws_t *me, broker_t *broker, char *name, uint16_t port);

/* Optional: set/override document root (default: "./www") */
void ws_set_docroot(ao_ws_t *me, const char *path);

/* AO API */
void ws_send_to(ao_ws_t *me, int client_idx, const char *text);
void ws_broadcast(ao_ws_t *me, const char *text);

#ifdef __cplusplus
}
#endif
#endif /* AO_WS_H */
