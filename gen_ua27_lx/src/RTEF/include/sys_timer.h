/**
 * @file sys_timer.h
 * @brief Provides an abstraction for system timers.
 *
 * This file defines an interface for managing system timers, allowing
 * registration of callback functions, enabling/disabling timers, and
 * handling priority-based scheduling of timer callbacks.
 *
 * It supports both Windows (using threads and waitable timers) and
 * FreeRTOS (using software timers).
 *
 * @author Nathan Ikolo
 * @date February 24, 2025
 */

#ifndef INCLUDE_SYS_TIMER_H_
#define INCLUDE_SYS_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <synchapi.h>
#elif defined (__linux__)
#include <pthread.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <errno.h>
#include <time.h>
#else
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"
#endif

/** @brief Maximum number of system timers that can be created. */
#define MAX_TIMERS 3

/**
 * @enum timer_type_t
 * @brief Defines the types of timers supported.
 */
typedef enum {
    RTOS,     /**< FreeRTOS-based timer */
    HARDWARE  /**< Hardware-based timer */
} timer_type_t;

/**
 * @brief Predefined timer intervals.
 */
enum {
    TIMER_200ms, /**< 200 milliseconds timer. */
    TIMER_10ms,  /**< 10 milliseconds timer. */
    TIMER_100ms  /**< 100 milliseconds timer. */
};

/**
 * @typedef timer_callback_t
 * @brief Function pointer type for timer callbacks.
 *
 * @param context User-defined data passed to the callback.
 */
typedef void (*timer_callback_t)(void *context);

/** @typedef timer_callback_entry_t
 *  @brief Typedef for the timer structure.
 */
typedef struct timer_callback_entry timer_callback_entry_t;

/**
 * @struct timer_callback_entry
 * @brief Represents an individual timer callback entry.
 *
 * This structure stores information about registered timer callbacks,
 * including the callback function, context, and priority.
 */
struct timer_callback_entry {
	uint8_t timer_id; /**< Identifier for the associated timer. */
    uint8_t state; /**< Priority of the callback (higher executes first). */
    timer_callback_t callback; /**< Function to be executed when the timer expires. */
#ifdef _WIN32
    CRITICAL_SECTION sem_entry; /**< Mutex for thread safety (Windows) */
#elif defined (__linux__)
    pthread_mutex_t sem_entry;
#else
    SemaphoreHandle_t sem_entry; /**< Mutex for thread safety (FreeRTOS) */
#endif
    void *context;  /**< User-defined context data for the callback. */
    uint8_t priority; /**< Execution priority of the callback */
    struct timer_callback_entry *next; /**< Pointer to the next callback entry */
};

/**
 * @struct timers_t
 * @brief Manages system timers and registered callback functions.
 *
 * This structure provides function pointers to manage timers,
 * including adding, removing, arming, and disarming callbacks.
 */
typedef struct {
    /**
     * @brief Arms a timer with a callback entry.
     *
     * @param entry Pointer to the timer callback entry to be armed.
     */
    void (*arm)(timer_callback_entry_t *entry);

    /**
     * @brief Disarms a timer, preventing it from triggering.
     *
     * @param entry Pointer to the timer callback entry to be disarmed.
     */
    void (*disarm)(timer_callback_entry_t *entry);

    /**
     * @brief Adds a new callback to a timer.
     *
     * @param timerId ID of the timer (TIMER_200ms, TIMER_10ms, etc.).
     * @param callback Function to be executed when the timer expires.
     * @param context Pointer to user-defined context.
     * @param priority Priority level for the callback execution.
     * @return Pointer to the newly created TimerCallbackEntry.
     */
    timer_callback_entry_t* (*add_callback)(uint8_t timer_id, timer_callback_t callback, void *context, uint8_t priority);

    /**
     * @brief Removes a callback from a timer.
     *
     * @param timerId ID of the timer.
     * @param callback Function pointer to remove.
     */
    void (*remove_callback)(uint8_t timer_id, timer_callback_t callback, void* context);
} timers_t;

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
timers_t* timer_ctor(timer_type_t type);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_SYS_TIMER_H_ */
