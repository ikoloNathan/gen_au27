/**
 * @file sys_timer.c
 * @brief Implements a system timer manager for scheduling timed callbacks.
 *
 * This file provides an implementation for system timers. The system allows
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
#include <stdbool.h>
#include <sys_timer.h>

/**
 * @enum timer_state_t
 * @brief Defines the possible states of a timer callback.
 */
typedef enum {
	DISARM = 0x7A, /**< Timer is disarmed (inactive). */
	ARM = 0xA7 /**< Timer is armed (active). */
} timer_state_t;

#define TIMER_PERIOD_10MS  10
#define TIMER_PERIOD_100MS 100
#define TIMER_PERIOD_200MS 200

typedef struct {
	pthread_t handle;
	pthread_mutex_t lock;
	pthread_cond_t cv;
	uint32_t armed_count;
	bool stop;
	timer_callback_entry_t *callbackList;
} timer_manager_t;

static inline void add_ms(struct timespec *t, uint32_t ms);
/** @brief Array of timer managers, one for each timer type. */
static timer_manager_t timer_manager[MAX_TIMERS];

static void arm(timer_callback_entry_t *entry);
static void disarm(timer_callback_entry_t *entry);
static void timer_manager_init(void);
static timer_callback_entry_t* timer_manager_add_callback(uint8_t timer_id,
		timer_callback_t callback, void *context, uint8_t priority,
		bool one_shot);
static void timer_manager_remove_callback(uint8_t timer_id,
		timer_callback_t callback);
static void* timer_thread(void *arg);
static inline uint32_t timer_period_ms(uint8_t timer_id);

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
timers_t* timer_ctor() {
	static timers_t timer = { .arm = arm, .disarm = disarm, .add_callback =
			timer_manager_add_callback, .remove_callback =
			timer_manager_remove_callback };
	if (timer_manager[0].handle == 0 && timer_manager[1].handle == 0
			&& timer_manager[2].handle == 0) {

		timer_manager_init();
	}
	return &timer;
}

uint32_t timer_period_ms(uint8_t timer_id) {
	switch (timer_id) {
	case TIMER_10ms:
		return TIMER_PERIOD_10MS;
	case TIMER_100ms:
		return TIMER_PERIOD_100MS;
	case TIMER_200ms:
		return TIMER_PERIOD_200MS;
	default:
		return 0u; /* or assert/handle error */
	}
}

void timer_manager_init(void) {
	for (uint8_t i = 0; i < MAX_TIMERS; ++i) {
		timer_manager[i].callbackList = NULL;
		timer_manager[i].armed_count = 0;
		timer_manager[i].stop = 0;

		pthread_mutex_init(&timer_manager[i].lock, NULL);
		pthread_cond_init(&timer_manager[i].cv, NULL);

		if (pthread_create(&timer_manager[i].handle, NULL, timer_thread,
				(void*) (uintptr_t) i) != 0) {
			// If creation fails, mark as stopped so other APIs can guard
			timer_manager[i].stop = 1;
			timer_manager[i].handle = (pthread_t) 0;
		}
	}
}

void add_ms(struct timespec *t, uint32_t ms) {
	t->tv_nsec += (long) (ms % 1000) * 1000000L;
	t->tv_sec += (time_t) (ms / 1000) + (t->tv_nsec / 1000000000L);
	t->tv_nsec %= 1000000000L;
}

void* timer_thread(void *arg) {
	uint8_t id = (uint8_t) (uintptr_t) arg;
	timer_manager_t *m = &timer_manager[id];
	const uint32_t period = timer_period_ms(id);

	pthread_mutex_lock(&m->lock);
	// Park until at least one callback is armed
	while (!m->stop && m->armed_count == 0)
		pthread_cond_wait(&m->cv, &m->lock);

	struct timespec next;
	clock_gettime(CLOCK_MONOTONIC, &next);

	while (!m->stop) {
		// schedule next absolute deadline
		add_ms(&next, period);

		// release lock while sleeping and executing callbacks
		pthread_mutex_unlock(&m->lock);

		// absolute sleep (handles EINTR)
		while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL)
				== EINTR) {
		}

		// dispatch callbacks
		pthread_mutex_lock(&m->lock);
		if (m->armed_count > 0) {
			for (timer_callback_entry_t *e = m->callbackList; e;) {
				if (e->state == ARM && e->callback) {
					timer_callback_entry_t *next_e = e->next; // save before unlock
					pthread_mutex_unlock(&m->lock);
					e->callback(e->context);
					if (e->one_shot)
						e->state = DISARM;
					pthread_mutex_lock(&m->lock);
					e = next_e;                                // move on safely
				} else {
					e = e->next;
				}
			}
		}

		// If we overran, skip ahead to the next boundary (no drift)
		struct timespec now;
		clock_gettime(CLOCK_MONOTONIC, &now);
		while ((now.tv_sec > next.tv_sec)
				|| (now.tv_sec == next.tv_sec && now.tv_nsec >= next.tv_nsec)) {
			add_ms(&next, period);
		}

		// If no work now, park until someone arms again
		while (!m->stop && m->armed_count == 0)
			pthread_cond_wait(&m->cv, &m->lock);
	}

	pthread_mutex_unlock(&m->lock);
	return NULL;
}

// Add a callback to a timer with priority
timer_callback_entry_t* timer_manager_add_callback(uint8_t timer_id,
		timer_callback_t callback, void *context, uint8_t priority,
		bool one_shot) {
	if (timer_id >= MAX_TIMERS) {
		return NULL; // Invalid timerId
	}
	timer_manager_t *m = &timer_manager[timer_id];
	pthread_mutex_lock(&m->lock);
	timer_callback_entry_t *newEntry = (timer_callback_entry_t*) malloc(
			sizeof(timer_callback_entry_t));
	if (!newEntry) {
		return NULL; // Memory allocation failed
	}
	pthread_mutex_init(&newEntry->sem_entry, NULL);

	newEntry->timer_id = timer_id;
	newEntry->state = DISARM;
	newEntry->one_shot = one_shot;
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
	pthread_mutex_unlock(&m->lock);
	return newEntry; // Success
}

// Remove a callback from a timer
void timer_manager_remove_callback(uint8_t timer_id, timer_callback_t callback) {
	if (timer_id >= MAX_TIMERS)
		return;
	timer_manager_t *m = &timer_manager[timer_id];
	pthread_mutex_lock(&m->lock);
	timer_callback_entry_t **head = &m->callbackList;
	timer_callback_entry_t *current = *head, *prev = NULL;

	while (current != NULL) {
		if (current->callback == callback && current->timer_id == timer_id) {
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
	pthread_mutex_unlock(&m->lock);
}

void arm(timer_callback_entry_t *e) {
	timer_manager_t *m = &timer_manager[e->timer_id];
	pthread_mutex_lock(&m->lock);
	if (e->state != ARM) {
		e->state = ARM;
		m->armed_count++;
		pthread_cond_signal(&m->cv); // wake thread if parked
	}
	pthread_mutex_unlock(&m->lock);
}

void disarm(timer_callback_entry_t *e) {
	timer_manager_t *m = &timer_manager[e->timer_id];
	pthread_mutex_lock(&m->lock);
	if (e->state == ARM) {
		e->state = DISARM;
		if (m->armed_count > 0)
			m->armed_count--;
		// If it drops to 0, the thread will park on next loop
	}
	pthread_mutex_unlock(&m->lock);
}

#ifdef __cplusplus
}
#endif
