/**
 * @file active_object.c
 * @brief Implements the Active Object pattern for message-driven execution.
 *
 * This file provides functions for managing Active Objects, including
 * initialization, message queue handling, dispatching, logging, and
 * integration with different operating systems (Windows & FreeRTOS).
 *
 * @author Nathan Ikolo
 * @date February 10, 2025
 */

#include <broker.h>
#include <string.h>
#include "active_object.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <stdio.h>

/** @brief System ID structure for identifying the active object. */
sys_id_t system_id = {
    .value = 0x00,
    .is_set = 1
};
#else
/** @brief System ID defined in the linker script for embedded platforms. */
sys_id_t system_id __attribute__ ((section(".sys_id_section"))); /* Defined in linker script */
#endif

/** @brief List of all registered active objects. */
base_obj_t *active_objects[MAX_ACTIVE_OBJECTS] = { 0 };
/** @brief Count of registered active objects. */
int active_object_count = 0;

#ifdef _WIN32
    #include <windows.h>
    #include <process.h>
    #include <synchapi.h>

static unsigned __stdcall event_loop(void *vparam);


/**
 * @brief Initializes the message queue.
 * @param q Pointer to the message queue.
 */
void MsgQueue_Init(MsgQueue_t *q) {
    InitializeCriticalSection(&q->lock);
    InitializeConditionVariable(&q->cond);
    q->head = 0;
    q->tail = 0;
    q->count = 0;
}

/**
 * @brief Pushes a message frame into the queue.
 * @param q Pointer to the message queue.
 * @param frame Pointer to the message frame to be added.
 */
static void MsgQueue_Push(MsgQueue_t *q, const message_frame_t *frame) {
    EnterCriticalSection(&q->lock);
    if (q->count < AO_QUEUE_SIZE) {
        q->buffer[q->tail] = *frame;
        q->tail = (q->tail + 1) % AO_QUEUE_SIZE;
        q->count++;
        WakeConditionVariable(&q->cond);
    }
    LeaveCriticalSection(&q->lock);
}

/**
 * @brief Pops a message frame from the queue.
 * @param q Pointer to the message queue.
 * @param frame Pointer to the message frame structure to be filled.
 * @return 1 if successful, 0 otherwise.
 */
static int MsgQueue_Pop(MsgQueue_t *q, message_frame_t *frame) {
    int success = 0;
    EnterCriticalSection(&q->lock);
    while (q->count == 0) {
        SleepConditionVariableCS(&q->cond, &q->lock, INFINITE);
    }
    if (q->count > 0) {
        *frame = q->buffer[q->head];
        q->head = (q->head + 1) % AO_QUEUE_SIZE;
        q->count--;
        success = 1;
    }
    LeaveCriticalSection(&q->lock);
    return success;
}

#elif defined (__linux__)

void MsgQueue_Init(MsgQueue_t *q) {
	q->head = q->tail = q->count = 0;
	pthread_mutex_init(&q->lock, NULL);
	sem_init(&q->items, 0, 0);                 // no items initially
	sem_init(&q->slots, 0, AO_QUEUE_SIZE);     // all slots free
}

void MsgQueue_Push(MsgQueue_t *q, const message_frame_t *m) {
	sem_wait(&q->slots);
	pthread_mutex_lock(&q->lock);
	q->buf[q->tail] = *m;
	q->tail = (q->tail + 1) % AO_QUEUE_SIZE;
	q->count++;
	pthread_mutex_unlock(&q->lock);
	// Signal that one item is available
	sem_post(&q->items);
}

uint8_t MsgQueue_Pop(MsgQueue_t *q, message_frame_t *out) {

	int success = 0;
	sem_wait(&q->items);

	pthread_mutex_lock(&q->lock);

	if (q->count > 0) {
		*out = q->buf[q->head];
		q->head = (q->head + 1) % AO_QUEUE_SIZE;
		q->count--;
		success = 1;
	}
	pthread_mutex_unlock(&q->lock);
	sem_post(&q->slots);

	return success;
}

#endif

/**
 * @brief Gets the current system time in milliseconds.
 * @return The current time in milliseconds.
 */
uint32_t get_time_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (uint32_t)((double)counter.QuadPart * 1000.0 / freq.QuadPart);
#elif defined (__linux__)
	struct timespec ts;
#ifdef CLOCK_MONOTONIC_RAW
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);   // not slewed by NTP
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);       // monotonic; may be gently slewed
#endif
	return (uint32_t) ((uint64_t) ts.tv_sec * 1000ULL
			+ (uint64_t) ts.tv_nsec / 1000000ULL);
#else
	return (uint32_t) (xTaskGetTickCount() * portTICK_PERIOD_MS);
#endif
}

#ifdef _WIN32
unsigned __stdcall event_loop(void *vparam);
#elif defined (__linux__)
static void* event_loop(void *vparam);
#else
static void event_loop(void *vparam);
#endif

/**
 * @brief Starts the Active Object.
 *
 * This function initializes the message queue and creates a thread/task
 * to handle incoming events.
 *
 * @param me Pointer to the Active Object instance.
 */
void start(base_obj_t *const me) {
#ifdef _WIN32
	if(me->thread_id == NULL){
		me->thread_id = (HANDLE)_beginthreadex(NULL, 0, event_loop, me, 0, NULL);
		while(!me->ready);
		if (me->thread_id != NULL) {
			me->vptr->log(me, (const uint8_t*) "ActiveObject started (Windows).", sizeof("ActiveObject started (Windows)."));
			register_active_object(me);
			fsm_init(&me->fsm, me->initialisation_state);
		}else{
			me->vptr->log(me, (const uint8_t*) "ActiveObject not started (Windows).", sizeof("ActiveObject not started (Windows)."));
		}
	}
#elif defined (__linux__)
	if (me->thread_id == 0) {
		pthread_mutex_init(&me->sem_log, NULL);
		if (pthread_create(&me->thread_id, NULL, event_loop, me) == 0) {
			while (!me->ready)
				;
			me->vptr->log(me, (const uint8_t*) "ActiveObject started (Linux).",
					sizeof("ActiveObject started (Linux)."));
			fsm_init(&me->fsm, me->initialisation_state);
		} else {
			me->vptr->log(me,
					(const uint8_t*) "ActiveObject not started (Linux).",
					sizeof("ActiveObject not started (Linux)."));
		}
	}

#else
	if (me->msg_queue_id != NULL && me->thread_id == NULL) {
		xTaskCreate(event_loop, me->name, 512, me, tskIDLE_PRIORITY + 1, &me->thread_id);
		while(!me->ready);
		if (me->thread_id != NULL) {
			me->vptr->log(me, (const uint8_t*) "ActiveObject started (FreeRTOS).", sizeof("ActiveObject started (FreeRTOS)."));
			register_active_object(me);
			fsm_init(&me->fsm, me->initialisation_state);
		} else {
			me->vptr->log(me, (const uint8_t*) "ActiveObject not started (FreeRTOS).", sizeof("ActiveObject not started (FreeRTOS)."));
		}
	}
#endif
}

/**
 * @brief Stops the Active Object.
 *
 * Cleans up message queues, deletes threads/tasks, and releases resources.
 *
 * @param me Pointer to the Active Object instance.
 */
void stop(base_obj_t *const me) {
	unregister_active_object(me);
#ifdef _WIN32
    if (me->thread_id) {
        TerminateThread(me->thread_id, 0);
        CloseHandle(me->thread_id);
        me->thread_id = NULL;
    }
    DeleteCriticalSection(&me->msgQueue.lock);
    me->vptr->log(me, (const uint8_t*)"ActiveObject stopped.",sizeof("ActiveObject stopped."));
#elif defined(__linux__)
	if (me->thread_id) {
		pthread_cancel(me->thread_id);
		pthread_join(me->thread_id, NULL);
		me->thread_id = 0;
	}
	pthread_mutex_destroy(&me->msgQueue.lock);
	sem_destroy(&me->msgQueue.slots);
	sem_destroy(&me->msgQueue.items);
	pthread_mutex_destroy(&me->sem_log);
	me->vptr->log(me, (const uint8_t*) "ActiveObject stopped (Linux).",
			sizeof("ActiveObject stopped (Linux)."));
#else
	if (me->thread_id) {
		vTaskDelete(me->thread_id);
		me->thread_id = NULL;
	}
	if (me->msg_queue_id) {
		vQueueDelete(me->msg_queue_id);
	}
	me->vptr->log(me, (const uint8_t*) "ActiveObject stopped (FreeRTOS).", sizeof("ActiveObject stopped (FreeRTOS)."));
#endif
}

/**
 * @brief Posts a message to the Active Object.
 *
 * This function adds a message to the object's queue for later processing.
 *
 * @param me Pointer to the Active Object instance.
 * @param frame The message frame to be added to the queue.
 */
void post(base_obj_t *const me, const message_frame_t frame) {
#if defined (_WIN32) || defined (__linux__)
	MsgQueue_Push(&me->msgQueue, &frame);
#else
	if (me->msg_queue_id) {
		xQueueSend(me->msg_queue_id, &frame, portMAX_DELAY);
	}
#endif
}

/**
 * @brief Dispatches a received message frame.
 * @param me Pointer to the Active Object instance.
 * @param frame Pointer to the received message frame.
 */
static void dispatch(base_obj_t *const me, const message_frame_t *frame) {
	(void) me;
	(void) frame;
}

/**
 * @brief Logs a message.
 *
 * This function formats and logs messages in hex format.
 *
 * @param me Pointer to the Active Object instance.
 * @param data Pointer to the byte array message.
 * @param length Length of `data` in bytes.
 */
void logger(base_obj_t *const me, const uint8_t *data, uint16_t length) {
#ifdef _WIN32
//	FILE *logFile = fopen("ActiveObject.log", "a");
//	    if (logFile) {
//	        fprintf(logFile, "%s : [", me->name);
//	        if (data && length > 0) {
//	            for (uint16_t i = 0; i < length; i++) {
//	                fprintf(logFile, "0x%02X", data[i]);
//	                if (i < length - 1) {
//	                    fprintf(logFile, ",");
//	                }
//	            }
//	        }
//	        fprintf(logFile, "]\n");
//	        fclose(logFile);
//	    }
#else
	/* Placeholder for eMMC logging */
	// eMMC_WriteLog(me->name, message);
#endif
}

/**
 * @brief Constructs an Active Object.
 *
 * Initializes the object, assigns the vtable, and sets the object name.
 *
 * @param me Pointer to Active Object instance.
 * @param broker Pointer to event broker.
 * @param name Name of the Active Object.
 */
void ao_ctor(base_obj_t *const me, broker_t *broker, const char *name) {
	static const base_vtable_t vtable = { &start, &stop, &post, &dispatch,
			&logger };
	me->vptr = &vtable;
	me->broker = broker;
	strncpy(me->name, name, sizeof(me->name) - 1);
	me->name[sizeof(me->name) - 1] = '\0';
#if defined (__linux__)
	me->thread_id = -1;
#else
	me->thread_id = NULL;
#endif
}

/**
 * @brief Registers an `ActiveObject` in the system registry.
 *
 * This function adds an active object instance to the global registry.
 * It ensures that the object is trackable by the watchdog system and
 * other monitoring components.
 *
 * @param me Pointer to the `ActiveObject` instance to be registered.
 * @note This function should be called within `start()` to ensure all objects
 *       are properly tracked.
 */
void register_active_object(base_obj_t *me) {
	if (active_object_count < MAX_ACTIVE_OBJECTS) {
		active_objects[active_object_count++] = me;
	} else {
#ifdef _WIN32
        printf("Error: Maximum number of ActiveObjects reached.\n");
#endif
	}
}

/**
 * @brief Unregisters an `ActiveObject` from the system registry.
 *
 * This function removes an `ActiveObject` from the global registry when
 * it is stopped. It ensures that the watchdog system no longer tracks
 * the object, preventing false alerts about inactive objects.
 *
 * @param me Pointer to the `ActiveObject` instance to be unregistered.
 * @note This function should be called within `stop()` to maintain
 *       an accurate object registry.
 */
void unregister_active_object(const base_obj_t *me) {
	for (int i = 0; i < active_object_count; i++) {
		if (active_objects[i] == me) {
			active_objects[i] = NULL;  // Replace with last entry
			active_object_count--;
			return;
		}
	}
}

#ifdef _WIN32
/**
 * @brief Event loop for processing messages in Windows.
 *
 * This function runs in a separate thread and continuously listens for messages
 * from the Active Object's queue. When a message is received, it is dispatched
 * to the appropriate handler.
 *
 * @param vparam Pointer to the Active Object instance.
 * @return Always returns 0 (unused).
 */
unsigned __stdcall event_loop(void *vparam) {
    base_obj_t *me = (base_obj_t*) vparam;
    message_frame_t event;

    while (1) {
        if (MsgQueue_Pop(&me->msgQueue, &event)) {
            if (me->vptr && me->vptr->dispatch) {
                me->vptr->dispatch(me, &event);
                memset(&event, 0, sizeof(message_frame_t));
            }
        }
    }
    return 0;
}

#elif defined(__linux__)
/**
 * @brief Event loop for processing messages in Linux (POSIX).
 */
static void* event_loop(void *vparam) {
	base_obj_t *me = (base_obj_t*) vparam;
	message_frame_t event;
	me->ready = 1;
	(void) me;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	while (1) {
		if (MsgQueue_Pop(&me->msgQueue, &event)) {
			if (me->vptr && me->vptr->dispatch) {
				me->vptr->dispatch(me, &event);
				memset(&event, 0, sizeof(message_frame_t));
			}
		}
		pthread_testcancel(); /* cancellation point */
	}
	return NULL;
}
#else
/**
 * @brief Event loop for processing messages in FreeRTOS.
 *
 * This function runs as a FreeRTOS task and continuously listens for messages
 * from the Active Object's queue. When a message is received, it is dispatched
 * to the appropriate handler.
 *
 * @param vparam Pointer to the Active Object instance.
 */
static void event_loop(void *vparam) {
	base_obj_t *me = (base_obj_t*) vparam;
	message_frame_t event;
	if (me != NULL) {
		while (1) {
			if (xQueueReceive(me->msg_queue_id, &event, portMAX_DELAY) == pdTRUE) {
				if (me->vptr && me->vptr->dispatch) {
					me->vptr->dispatch(me, &event);
					memset(&event, 0, sizeof(message_frame_t));
				}
			}
		}
	}
}
#endif

#ifdef __cplusplus
}
#endif
