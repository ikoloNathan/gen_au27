/**
 * @file active_object.h
 * @brief Active Object framework for Windows and FreeRTOS.
 *
 * This file defines the Active Object system, supporting message queuing,
 * event dispatching, and logging functionality for both Windows and FreeRTOS platforms.
 *
 * @author Nathan Ikolo
 * @date February 10, 2025
 */

#ifndef INCLUDE_ACTIVE_OBJECT_H_
#define INCLUDE_ACTIVE_OBJECT_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#include <synchapi.h>
#include <stdio.h> /**< File logging for Windows */
#elif defined (__linux__)
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#else
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#endif

#include <string.h>
#include "message.h"
#include <fsm.h>

/** @def AO_QUEUE_SIZE
 *  @brief Defines the message queue size.
 */
#define AO_QUEUE_SIZE 16

/** @def MAX_ACTIVE_OBJECTS
 *  @brief Defines the maximum number of active objects in the system.
 */
#define MAX_ACTIVE_OBJECTS 32

#if defined (_WIN32) || defined (__linux__)
    /** @brief Initializes platform-specific message queue. */
    #define __PLATFORM_INIT__(me) MsgQueue_Init(&(me)->msgQueue);
#else
    /** @brief Initializes platform-specific message queue for FreeRTOS. */
    #define __PLATFORM_INIT__(me) (me)->msg_queue_id = xQueueCreate(AO_QUEUE_SIZE, sizeof(message_frame_t));
#endif

#ifdef __linux__
#define THREAD_INIT 0
#else
#define THREAD_INIT NULL
#endif
/**
 * @brief Initializes an Active Object instance.
 *
 * This macro assigns function pointers to the vtable and initializes the object.
 *
 * @param me Pointer to the Active Object instance.
 * @param broker Pointer to the event broker.
 * @param name Name of the Active Object.
 * @param start_fn Function pointer to a custom start function, or NULL to use the default.
 */
#define INIT_BASE(me, broker, name,system_id, start_fn)    \
    static const base_vtable_t __vtable__ = {               \
        (start_fn) != NULL ? (start_fn) : &start, &stop, &post, &dispatch, &logger};\
        (me)->super.vptr = &__vtable__;                        \
        (me)->super.broker = (broker);                     \
        (me)->super.ready = 0;								\
        (me)->super.last_heartbeat_time = (uint32_t)get_time_ms();						\
        strncpy((me)->super.name, (name), sizeof((me)->super.name) - 1); \
        (me)->super.name[sizeof((me)->super.name) - 1] = '\0'; \
        __PLATFORM_INIT__(&me->super);														\
        (me)->super.thread_id = THREAD_INIT;                       \
        (me)->super.fsm.super = (void*) &(me)->super;

/** @typedef base_vtable_t
 *  @brief Typedef for the Active Object virtual table structure.
 */
typedef struct base_vtable base_vtable_t;

/** @typedef broker_t
 *  @brief Typedef for the event broker structure.
 */
typedef struct broker broker_t;

/** @typedef base_obj_t
 *  @brief Typedef for the active object structure.
 */
typedef struct base_obj base_obj_t;

/**
 * @struct sys_id_t
 * @brief Represents the system ID.
 */
typedef struct {
    uint8_t value; /**< System ID value. */
    uint8_t is_set; /**< Flag indicating if system ID is set. */
} sys_id_t;

#ifdef _WIN32
/**
 * @struct MsgQueue_t
 * @brief Message queue structure for Windows.
 */
typedef struct {
    message_frame_t buffer[AO_QUEUE_SIZE]; /**< Circular buffer */
    int head; /**< Index of the queue head */
    int tail; /**< Index of the queue tail */
    int count; /**< Number of messages in the queue */
    CRITICAL_SECTION lock; /**< Mutex for thread safety */
    CONDITION_VARIABLE cond; /**< Condition variable for message waiting */
} MsgQueue_t;

void MsgQueue_Init(MsgQueue_t *q);
#elif defined(__linux__)

typedef pthread_mutex_t ao_mutex_t;
typedef pthread_cond_t ao_cond_t;

typedef struct {
    message_frame_t buf[AO_QUEUE_SIZE];
    size_t head, tail, count;
    pthread_mutex_t lock;
    sem_t items;   // counts available messages
	sem_t slots;   // counts available slots
} MsgQueue_t;

void MsgQueue_Init(MsgQueue_t *q);
void MsgQueue_Push(MsgQueue_t *q, const message_frame_t *m);
uint8_t MsgQueue_Pop(MsgQueue_t *q, message_frame_t *out);

#endif

/**
 * @struct base_obj
 * @brief Base Active Object class.
 *
 * The Active Object pattern enables event-driven execution with stateful behavior.
 * This structure encapsulates an object that can process messages asynchronously.
 *
 * Each Active Object:
 * - Is linked to a **message broker** for event dispatching.
 * - Runs within a **separate task/thread** (depending on platform).
 * - Uses a **Finite State Machine (FSM)** to manage state transitions.
 * - Can define a **custom initialization state** (`initialisation_state`).
 */
struct base_obj {

	/**
	 * @brief Virtual table for Active Object operations.
	 *
	 * The virtual function table (`vptr`) enables polymorphic behavior,
	 * allowing derived Active Objects to override standard behavior.
	 */
	base_vtable_t const *vptr; /**< Virtual table pointer */

#ifdef _WIN32

	/**
	 * @brief Thread handle for Windows implementation.
	 *
	 * This handle represents the execution thread of the Active Object.
	 * - Created using `_beginthreadex()`.
	 * - Terminated when `stop()` is called.
	 */
    HANDLE thread_id; /**< Windows thread handle */

    /**
	 * @brief Message queue for Windows implementation.
	 *
	 * The queue holds incoming messages and is protected by a mutex.
	 * - Uses a circular buffer.
	 * - Synchronization via `CRITICAL_SECTION` and `CONDITION_VARIABLE`.
	 */
    MsgQueue_t msgQueue; /**< Windows message queue */

    /**
	 * @brief Mutex for logging synchronization (Windows only).
	 *
	 * Prevents concurrent log writes from multiple threads.
	 */
    CRITICAL_SECTION sem_log; /**< Mutex for logging */
#elif defined (__linux__)
    /**
	 * @brief Thread handle for Linux (POSIX) implementation.
	 */
	pthread_t thread_id; /**< POSIX thread */

	/**
	 * @brief Message queue for POSIX implementation.
	 */
	MsgQueue_t msgQueue; /**< POSIX message queue */

	/**
	 * @brief Mutex for logging synchronization (POSIX).
	 */
	pthread_mutex_t sem_log; /**< Mutex for logging */
#else

	/**
	 * @brief Task handle for FreeRTOS implementation.
	 *
	 * Represents the FreeRTOS task running this Active Object.
	 * - Created using `xTaskCreate()`.
	 * - Deleted using `vTaskDelete()` when `stop()` is called.
	 */
	TaskHandle_t thread_id; /**< FreeRTOS task handle */

	/**
	 * @brief FreeRTOS message queue handle.
	 *
	 * This queue stores messages sent to the Active Object.
	 * - Uses `xQueueCreate()`.
	 * - Messages are posted via `post()`.
	 */
	QueueHandle_t msg_queue_id; /**< FreeRTOS message queue */

	/**
	 * @brief Semaphore for logging synchronization (FreeRTOS only).
	 *
	 * Ensures thread-safe logging within the Active Object.
	 */
	SemaphoreHandle_t sem_log; /**< Semaphore for log protection */
#endif

	/**
	 * @brief Pointer to the event broker managing this Active Object.
	 *
	 * The broker facilitates message dispatching between different Active Objects.
	 */
	broker_t *broker; /**< Event broker */

	/**
	 * @brief Finite State Machine (FSM) associated with the Active Object.
	 *
	 * The FSM allows the Active Object to transition between states based on events.
	 */
	fsm_t fsm; /**< Finite state machine */

	/**
	 * @brief Pointer to the initial state of the Active Object.
	 *
	 * Each derived Active Object assigns a custom `initialisation_state`,
	 * which defines the default behavior when the object starts.
	 *
	 * - The pointer is assigned dynamically in the constructor (`AO_ctor()`).
	 * - Different Active Objects can override this with unique states.
	 * - The state defines entry/exit actions, event handlers, etc.
	 *
	 * Example Usage:
	 * @code
	 * struct State custom_state = {
	 *			.handler = custom_handler,
	 *			.on_entry = custom_init,
	 *			.on_exit = custom_cleanup,
	 * 			.transitions = next_jump,
	 * 			.transition_count = 2
	 * };
	 * my_active_object.initialisation_state = &custom_state;
	 * @endcode
	 */
	struct state *initialisation_state;

	uint8_t dispatch_id;

	uint8_t ready;

	/**
	 * @brief Name identifier for the Active Object.
	 *
	 * This name is used for logging and debugging purposes.
	 * - Assigned at initialization.
	 * - Limited to 31 characters (null-terminated).
	 */
	char name[32]; /**< Object name */

	/**
	 * @brief Last recorded heartbeat timestamp (used for monitoring).
	 *
	 * This tracks the last time the Active Object was active.
	 * - Updated regularly to indicate the system is alive.
	 * - Used for health monitoring and timeout detection.
	 */
	uint32_t last_heartbeat_time; /**< Last time this AO updated its heartbeat */
};

/**
 * @brief Global registry for tracking active objects.
 *
 * This array holds pointers to all currently active objects in the system.
 * Each `ActiveObject` instance is added to this registry upon creation.
 * The `AoWatchDog` and other system monitoring components use this registry
 * to track and ensure proper execution of active objects.
 *
 * @note The array has a fixed maximum size (`MAX_ACTIVE_OBJECTS`).
 */
extern base_obj_t *active_objects[MAX_ACTIVE_OBJECTS];

/**
 * @brief The number of active objects currently in the system.
 *
 * This counter keeps track of the number of `ActiveObject` instances
 * that are currently registered in the system. It is incremented when a new
 * object is created and decremented when an object is stopped and unregistered.
 */
extern int active_object_count;  // Counter

/**
 * @struct base_vtable
 * @brief Virtual table structure for Active Object.
 *
 * Provides function pointers for polymorphic behavior.
 */
struct base_vtable {
	/**
	 * @brief Starts the Active Object.
	 *
	 * Initializes resources and starts the event loop.
	 *
	 * @param me Pointer to the Active Object instance.
	 */
	void (*start)(base_obj_t *const me);

	/**
	 * @brief Stops the Active Object.
	 *
	 * Cleans up resources and stops the event loop.
	 *
	 * @param me Pointer to the Active Object instance.
	 */
	void (*stop)(base_obj_t *const me);

	/**
	 * @brief Posts a message to the queue.
	 *
	 * Queues a message for processing by the Active Object.
	 *
	 * @param me Pointer to the Active Object instance.
	 * @param frame The message frame to enqueue.
	 */
	void (*post)(base_obj_t *const me, const message_frame_t frame);

	/**
	 * @brief Dispatches an event.
	 *
	 * Handles messages dequeued from the message queue.
	 *
	 * @param me Pointer to the Active Object instance.
	 * @param frame Pointer to the message frame containing event data.
	 */
	void (*dispatch)(base_obj_t *const me, const message_frame_t *frame);

	/**
	 * @brief Logs a message.
	 *
	 * Logs messages in a hex format. On Windows, logs to a file. On FreeRTOS,
	 * a placeholder exists for logging via eMMC.
	 *
	 * Example log:
	 * `"SystemAO : [0x7,0x3,0x32,0xAD]"`
	 *
	 * @param me Pointer to the Active Object instance.
	 * @param data Pointer to byte array message.
	 * @param length Number of bytes in `data`.
	 */
	void (*log)(base_obj_t *const me, const uint8_t *data, uint16_t length);
};

/** @brief System ID symbol, defined in linker script */
extern sys_id_t system_id;

/**
 * @brief Constructs an Active Object.
 *
 * Initializes the object, assigns the vtable, and sets the object name.
 *
 * @param me Pointer to Active Object instance.
 * @param broker Pointer to event broker.
 * @param name Name of the Active Object.
 */
void ao_ctor(base_obj_t *const me, broker_t *broker,const char *name);

/**
 * @brief Starts the Active Object.
 *
 * This function initializes the message queue and creates a thread/task
 * to handle incoming events.
 *
 * @param me Pointer to the Active Object instance.
 */
void start(base_obj_t *const me);

/**
 * @brief Stops the Active Object.
 *
 * Cleans up message queues, deletes threads/tasks, and releases resources.
 *
 * @param me Pointer to the Active Object instance.
 */
void stop(base_obj_t *const me);

/**
 * @brief Posts a message to the Active Object.
 *
 * This function adds a message to the object's queue for later processing.
 *
 * @param me Pointer to the Active Object instance.
 * @param frame The message frame to be added to the queue.
 */
void post(base_obj_t *const me, const message_frame_t frame);

/**
 * @brief Logs a message.
 *
 * This function formats and logs messages in hex format.
 *
 * @param me Pointer to the Active Object instance.
 * @param data Pointer to the byte array message.
 * @param length Length of `data` in bytes.
 */
void logger(base_obj_t *const me, const uint8_t *data, uint16_t length);

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
void register_active_object(base_obj_t *me);

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
void unregister_active_object(const base_obj_t * me);

/**
 * @brief Retrieves the current system time in milliseconds.
 * @return The current system time in milliseconds.
 */
uint32_t get_time_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ACTIVE_OBJECT_H_ */
