/*
 * Broker.h
 *
 *  Created on: 10 Feb 2025
 *      Author: Nathan Ikolo
 */

/**
 * @file broker.h
 * @brief Message Broker for Pub-Sub Communication
 *
 * Provides functions for ActiveObjects to subscribe, publish, and unsubscribe from topics.
 * Supports multi-threaded operation using mutex protection.
 *
 * @author Nathan Ikolo
 * @date February 10, 2025
 */

#ifndef BROKER_H_
#define BROKER_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <synchapi.h>
#elif defined(__linux__)
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#else
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#endif

#include "message.h"
#include "active_object.h"

/** @brief Maximum number of messages in a queue. */
#define MAX_QUEUE_SIZE  128

/** @brief Maximum number of topics managed by the broker. */
#define MAX_TOPICS      32

/** @brief Maximum subscribers per topic. */
#define MAX_SUBSCRIBERS_PER_TOPIC 32

/** @brief Mask applied to topic filtering. */
#define TOPIC_MASK      0x3FFFEFF

/**
 * @enum QueueSelection
 * @brief Specifies whether to post a message to the primary or secondary queue.
 */
typedef enum {
    SECONDARY_QUEUE, /**< Secondary message queue */
    PRIMARY_QUEUE    /**< Primary message queue */
} QueueSelection;

/**
 * @enum entry_type_t
 * @brief Defines the type of filtering used for topic subscriptions.
 */
typedef enum {
    EXACT_MATCH, /**< Matches exact topic ID */
    MASK,        /**< Uses a bitmask for topic filtering */
} entry_type_t;

/**
 * @struct topic_config_t
 * @brief Configuration structure for topic-based messaging.
 */
typedef struct {
    uint32_t topic; /**< Exact topic ID */
    uint32_t start; /**< Start range for range-based topics or Mask value if entry_type_t is MASK */
    uint32_t end;   /**< End range for range-based topics */
    entry_type_t type; /**< Type of filtering (EXACT, MASK, RANGE) */
} topic_config_t;

/**
 * @struct topic_entry_t
 * @brief Represents a topic and its list of subscribers.
 */
typedef struct {
    topic_config_t ui; /**< Topic metadata */
    uint8_t valid; /**< Whether the topic is currently active */
    uint8_t count;
    struct {
        base_obj_t *subscriber; /**< Pointer to the subscribing ActiveObject */
        int active; /**< Whether the subscription is active */
    } idx[MAX_SUBSCRIBERS_PER_TOPIC]; /**< list of subscriber assigned to a particular topic */
} topic_entry_t;

/**
 * @struct Broker_Queue_t
 * @brief Message queue used by the broker.
 *
 * Supports both Windows and FreeRTOS environments.
 */
typedef struct {
#ifdef _WIN32
    message_frame_t buffer[MAX_QUEUE_SIZE]; /**< Buffer for storing messages */
    int head; /**< Index of the first element in the queue */
    int tail; /**< Index where new elements are inserted */
    int count; /**< Current number of messages in the queue */
    CRITICAL_SECTION lock; /**< Mutex lock for Windows */
    CONDITION_VARIABLE cond; /**< Condition variable for Windows */
#elif defined (__linux__)
    message_frame_t buf[AO_QUEUE_SIZE];
	size_t head, tail, count;
	pthread_mutex_t lock;
	sem_t items;   // counts available messages
	sem_t slots;   // counts available slots
#else
    QueueHandle_t queue_handle; /**< FreeRTOS queue handle */
#endif
} Broker_Queue_t;

/**
 * @struct broker
 * @brief The core broker structure managing message distribution.
 */
struct broker {
#ifdef _WIN32
    HANDLE primary_thread_id; /**< Thread handling primary queue processing */
    HANDLE secondary_thread_id; /**< Thread handling secondary queue processing */
    CRITICAL_SECTION sem_handle; /**< Mutex for synchronization */
#elif defined (__linux__)
    pthread_t primary_thread_id; /**< POSIX thread */
    pthread_t secondary_thread_id; /**< POSIX thread */
    ao_mutex_t sem_handle; /**< Mutex for logging */
#else
    TaskHandle_t primary_thread_id; /**< FreeRTOS task for primary queue */
    TaskHandle_t secondary_thread_id; /**< FreeRTOS task for secondary queue */
    SemaphoreHandle_t sem_handle; /**< Semaphore for synchronization */
#endif
    Broker_Queue_t primary_queue; /**< Primary message queue */
    Broker_Queue_t secondary_queue; /**< Secondary message queue */
    topic_entry_t topics[MAX_TOPICS]; /**< List of active topics */
    uint8_t broker1_ready;
    uint8_t broker2_ready;
};

/**
 * @brief Initializes a Broker instance.
 *
 * Allocates memory and initializes the message queues and subscription lists.
 *
 * @return Pointer to the initialized broker instance.
 */
broker_t *broker_ctor(void);

/**
 * @brief Subscribes an ActiveObject to one or more topics.
 *
 * @param broker Pointer to the broker instance.
 * @param configs Pointer to topic configurations.
 * @param length Number of topics in `configs`.
 * @param subscriber The ActiveObject subscribing to topics.
 * @return 1 if subscription was successful, 0 otherwise.
 */
int broker_subscribe(broker_t *broker, topic_config_t *configs, uint16_t length, base_obj_t *subscriber);

/**
 * @brief Publishes a message to a topic.
 *
 * The message is delivered to all subscribers of the specified topic.
 *
 * @param broker Pointer to the broker instance.
 * @param frame Message frame containing the data.
 * @return 1 if successful, 0 otherwise.
 */
int broker_publish(broker_t *broker, const message_frame_t frame);

/**
 * @brief Unsubscribes an ActiveObject from multiple topics.
 *
 * @param broker Pointer to the Broker instance.
 * @param configs Pointer to an array of topic configurations.
 * @param length Number of topics in `configs`.
 * @param subscriber The ActiveObject that wants to unsubscribe.
 * @return Number of successfully unsubscribed topics.
 */
int broker_unsubscribe(broker_t *broker, topic_config_t *configs, uint16_t length,const base_obj_t *subscriber);

/**
 * @brief Posts a message to the broker queue.
 *
 * The message is placed in either the **primary or secondary queue**, based on the `primary` parameter.
 *
 * @param broker Pointer to the broker instance.
 * @param frame Message frame to be queued.
 * @param primary Specifies the queue to use (PRIMARY_QUEUE or SECONDARY_QUEUE).
 */
void broker_post(broker_t *broker, message_frame_t frame, int primary);


#ifndef _WIN32
/**
 * @brief Posts a message from an ISR (Interrupt Service Routine).
 *
 * Ensures real-time event dispatching without blocking execution.
 *
 * @param broker Pointer to the broker instance.
 * @param frame Message frame to be queued.
 * @param primary Determines which queue to use (PRIMARY_QUEUE or SECONDARY_QUEUE).
 */
void broker_post_ISR(broker_t* broker, message_frame_t frame, int primary);

/**
 * @brief Configures a message filter for incoming CAN messages.
 *
 * Used to limit which messages are processed based on ID and mask.
 *
 * @param msgId The message ID to filter.
 * @param mask Bitmask for filtering.
 * @param filter_type Type of filtering (MASK, RANGE, etc.).
 */
void broker_set_filter(uint32_t msgId, uint32_t mask, uint8_t filter_type);
#endif

#ifdef __cplusplus
}
#endif
#endif /* BROKER_H_ */
