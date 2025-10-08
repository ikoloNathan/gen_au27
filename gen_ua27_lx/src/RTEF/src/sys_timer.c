/**
 * @file sys_timer.c
 * @brief Implements a system timer manager for scheduling timed callbacks.
 *
 * This file provides an implementation for system timers, supporting both Windows
 * (using waitable timers) and RTOS (using software timers). The system allows
 * registering callbacks with different timer periods and priority levels.
 *
 * It manages multiple timers and ensures proper execution of registered callback
 * functions while maintaining synchronization between tasks.
 *
 * @author Nathan Ikolo
 * @date February 24, 2025
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <sys_timer.h>


/**
 * @enum timer_state_t
 * @brief Defines the possible states of a timer callback.
 */
typedef enum {
    DISARM = 0x7A, /**< Timer is disarmed (inactive). */
    ARM = 0xA7     /**< Timer is armed (active). */
} timer_state_t;

#ifdef _WIN32
    #define TIMER_PERIOD_10MS  10
    #define TIMER_PERIOD_100MS 100
    #define TIMER_PERIOD_200MS 200

    typedef struct {
        HANDLE handle;
        timer_callback_entry_t *callbackList;
        CRITICAL_SECTION lock;
    } timer_manager_t;

#elif defined (__linux__)
	#define TIMER_PERIOD_10MS  10
    #define TIMER_PERIOD_100MS 100
    #define TIMER_PERIOD_200MS 200
    typedef struct {
    	pthread_t handle;
    	int tfd;
		timer_callback_entry_t *callbackList;
		pthread_mutex_t lock;
	} timer_manager_t;
#else
#define TIMER_PERIOD_10MS  pdMS_TO_TICKS(10)	/**< Timer period for 10ms. */
#define TIMER_PERIOD_100MS pdMS_TO_TICKS(100)	/**< Timer period for 100ms. */
#define TIMER_PERIOD_200MS pdMS_TO_TICKS(200)	/**< Timer period for 200ms. */

/**
 * @struct timer_manager_t
 * @brief Represents a timer manager for RTOS.
 */
typedef struct {
	TimerHandle_t handle; /**< RTOS software timer handle. */
	timer_callback_entry_t *callbackList; /**< List of registered callbacks. */
} timer_manager_t;
#endif

/** @brief Array of timer managers, one for each timer type. */
static timer_manager_t timer_manager[MAX_TIMERS];

static void arm(timer_callback_entry_t *entry);
static void disarm(timer_callback_entry_t *entry);
static void TimerManager_Init(void);
static timer_callback_entry_t* timer_manager_add_callback(uint8_t timer_id, timer_callback_t callback, void *context, uint8_t priority);
static void timer_manager_remove_callback(uint8_t timer_id, timer_callback_t callback, void *context);
#if defined (_WIN32) || defined (__linux__)
static void timer_callback_handler(uint8_t timer_id);
#else
static void timer_callback_handler(TimerHandle_t xTimer);
#endif

/**
 * @brief Constructs a system timer manager.
 *
 * This function initializes and returns a timer manager instance.
 * Depending on the `type` parameter, it can either be an RTOS timer
 * or a hardware-based timer (not yet supported).
 *
 * @param type The type of timer to initialize.
 * @return Pointer to the created `timers_t` instance, or `NULL` if failed.
 */
timers_t* timer_ctor(timer_type_t type) {
	if (type == RTOS) {
		static timers_t timer = { .arm = arm,
					.disarm = disarm,
					.add_callback = timer_manager_add_callback,
					.remove_callback = timer_manager_remove_callback };
#ifdef __linux__
		if (timer_manager[0].handle == 0 && timer_manager[1].handle == 0 && timer_manager[2].handle == 0) {
#else
		if (timer_manager[0].handle == NULL && timer_manager[1].handle == NULL && timer_manager[2].handle == NULL) {
#endif
			TimerManager_Init();
		}
		return &timer;
	}
	if (type == HARDWARE) {
		return NULL; //Not yet supported
	}
	return NULL;
}

#ifdef _WIN32
// ---------------------------- Windows Implementation ----------------------------

static DWORD WINAPI TimerThread(LPVOID lpParam);


void TimerManager_Init() {
    for (uint8_t i = 0; i < MAX_TIMERS; i++) {
        timer_manager[i].handle = CreateWaitableTimer(NULL, FALSE, NULL);
        InitializeCriticalSection(&timer_manager[i].lock);
        timer_manager[i].callbackList = NULL;

        if (timer_manager[i].handle) {
            CreateThread(NULL, 0, TimerThread, (LPVOID)(uintptr_t)i, 0, NULL);
        }
    }
}

static DWORD WINAPI TimerThread(LPVOID lpParam) {
    uint8_t timer_id = (uint8_t)(uintptr_t)lpParam;
    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -(int64_t)(timer_id == TIMER_10ms ? TIMER_PERIOD_10MS :
                                  timer_id == TIMER_100ms ? TIMER_PERIOD_100MS :
                                                           TIMER_PERIOD_200MS) * 10000LL;

    while (1) {
        SetWaitableTimer(timer_manager[timer_id].handle, &dueTime,
                         timer_id == TIMER_10ms ? TIMER_PERIOD_10MS :
                         timer_id == TIMER_100ms ? TIMER_PERIOD_100MS :
                                                  TIMER_PERIOD_200MS,
                         NULL, NULL, 0);

        WaitForSingleObject(timer_manager[timer_id].handle, INFINITE);
        timer_callback_handler(timer_id);
    }
    return 0;
}

static void timer_callback_handler(uint8_t timer_id) {
    EnterCriticalSection(&timer_manager[timer_id].lock);

    timer_callback_entry_t *entry = timer_manager[timer_id].callbackList;
    while (entry) {
        if (entry->callback && entry->state == ARM) {
            entry->callback(entry->context);
        }
        entry = entry->next;
    }

    LeaveCriticalSection(&timer_manager[timer_id].lock);
}

#elif defined (__linux__)

static void* TimerThread(void *arg);

static inline uint32_t timer_period_ms(uint8_t timer_id)
{
    switch (timer_id) {
        case TIMER_10ms:   return TIMER_PERIOD_10MS;
        case TIMER_100ms:  return TIMER_PERIOD_100MS;
        case TIMER_200ms:  return TIMER_PERIOD_200MS;
        default:           return 0u;  /* or assert/handle error */
    }
}

static int spawn_timer_thread(timer_manager_t *slot, uint8_t index) {
    return pthread_create(&slot->handle, NULL, TimerThread, (void*)(uintptr_t)index);
}

void TimerManager_Init(void)
{
    for (uint8_t i = 0; i < MAX_TIMERS; ++i) {
        timer_manager[i].tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
        pthread_mutex_init(&timer_manager[i].lock, NULL);
        timer_manager[i].callbackList = NULL;

        if (timer_manager[i].tfd >= 0) {
            if (spawn_timer_thread(&timer_manager[i], i) != 0) {
                /* couldn't start thread; clean up this slot */
                close(timer_manager[i].tfd);
                timer_manager[i].tfd = -1;
            }
        }
    }
}

static void* TimerThread(void *arg) {
    uint8_t id = (uint8_t)(uintptr_t)arg;
    uint32_t period = timer_period_ms(id); // your helper

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while(1) {
        // next += period
        next.tv_nsec += (long)(period % 1000) * 1000000L;
        next.tv_sec  += (time_t)(period / 1000) + (next.tv_nsec / 1000000000L);
        next.tv_nsec %= 1000000000L;

        // sleep until absolute deadline (no drift)
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR) {}

        // check a stop flag if you have one, then:
        timer_callback_handler(id);
    }
    return NULL;
}

static void timer_callback_handler(uint8_t timer_id) {
    pthread_mutex_lock(&timer_manager[timer_id].lock);
    timer_callback_entry_t *entry = timer_manager[timer_id].callbackList;
    while (entry) {
        if (entry->callback && entry->state == ARM) {
            entry->callback(entry->context);
        }
        entry = entry->next;
    }

    pthread_mutex_unlock(&timer_manager[timer_id].lock);
}

#else

void TimerManager_Init() {
	timer_manager[0].handle = xTimerCreate("Timer_200ms", TIMER_PERIOD_200MS, pdTRUE, NULL, timer_callback_handler);
	timer_manager[1].handle = xTimerCreate("Timer_10ms", TIMER_PERIOD_10MS, pdTRUE, NULL, timer_callback_handler);
	timer_manager[2].handle = xTimerCreate("Timer_100ms", TIMER_PERIOD_100MS, pdTRUE, NULL, timer_callback_handler);

	for (uint8_t i = 0; i < MAX_TIMERS; i++) {
		timer_manager[i].callbackList = NULL;
		if (timer_manager[i].handle != NULL) {
			xTimerStart(timer_manager[i].handle, 0);
		}
	}
}

// Timer ISR Callback Handler
void timer_callback_handler(TimerHandle_t xTimer) {
	for (uint8_t i = 0; i < MAX_TIMERS; i++) {
		if (timer_manager[i].handle == xTimer) {
			timer_callback_entry_t *entry = timer_manager[i].callbackList;
			while (entry != NULL && entry->state == ARM) {
				if (entry->callback) {
					entry->callback(entry->context);
				}
				entry = entry->next;
			}
			break;
		}
	}
}

#endif
// Add a callback to a timer with priority
timer_callback_entry_t* timer_manager_add_callback(uint8_t timer_id, timer_callback_t callback, void *context, uint8_t priority) {
	if (timer_id >= MAX_TIMERS) {
		return 0; // Invalid timerId
	}

	timer_callback_entry_t *newEntry = (timer_callback_entry_t*) malloc(sizeof(timer_callback_entry_t));
	if (!newEntry) {
		return NULL; // Memory allocation failed
	}
#ifdef _WIN32
	InitializeCriticalSection(&newEntry->sem_entry);
#elif defined (__linux__)
	pthread_mutex_init(&newEntry->sem_entry,NULL);
#else
	newEntry->sem_entry = xSemaphoreCreateMutex();
#endif
	newEntry->timer_id = timer_id;
	newEntry->state = DISARM;
	newEntry->callback = callback;
	newEntry->context = context;
	newEntry->priority = priority;
	newEntry->next = NULL;

	// Insert the new callback in sorted order based on priority
	timer_callback_entry_t **head = &timer_manager[timer_id].callbackList;
	if (*head == NULL || (*head)->priority < priority) {
		// Insert at the head if higher priority
		newEntry->next = *head;
		*head = newEntry;
	} else {
		// Insert in sorted order
		timer_callback_entry_t *current = *head;
		while (current->next != NULL && current->next->priority >= priority) {
			current = current->next;
		}
		newEntry->next = current->next;
		current->next = newEntry;
	}

	return newEntry; // Success
}

// Remove a callback from a timer
void timer_manager_remove_callback(uint8_t timer_id, timer_callback_t callback, void *context) {
	if (timer_id >= MAX_TIMERS)
		return;

	timer_callback_entry_t **head = &timer_manager[timer_id].callbackList;
	timer_callback_entry_t *current = *head, *prev = NULL;

	while (current != NULL) {
		if (current->callback == callback && current->timer_id == timer_id && current->context == context) {
			if (prev == NULL) {
				*head = current->next; // Remove first element
			} else {
				prev->next = current->next;
			}
			free(current);
			return;
		}
		prev = current;
		current = current->next;
	}
}

void arm(timer_callback_entry_t *entry) {
#ifdef _WIN32
	EnterCriticalSection(&entry->sem_entry);
#elif defined (__linux__)
	pthread_mutex_lock(&entry->sem_entry);
#else
	if (xSemaphoreTake(entry->sem_entry, portMAX_DELAY) == pdTRUE) {
#endif
		if (entry) {
			entry->state = ARM;
		}
#ifdef _WIN32
    LeaveCriticalSection(&entry->sem_entry);
#elif defined (__linux__)
    pthread_mutex_unlock(&entry->sem_entry);
#else
		xSemaphoreGive(entry->sem_entry);
	}
#endif
}

void disarm(timer_callback_entry_t *entry) {
#ifdef _WIN32
	EnterCriticalSection(&entry->sem_entry);
#elif defined (__linux__)
	pthread_mutex_lock(&entry->sem_entry);
#else
	if (xSemaphoreTake(entry->sem_entry, portMAX_DELAY) == pdTRUE) {
#endif
		if (entry) {
			entry->state = DISARM;
		}
#ifdef _WIN32
    LeaveCriticalSection(&entry->sem_entry);
#elif defined (__linux__)
    pthread_mutex_unlock(&entry->sem_entry);
#else
		xSemaphoreGive(entry->sem_entry);
	}
#endif
}

#ifdef __cplusplus
}
#endif
