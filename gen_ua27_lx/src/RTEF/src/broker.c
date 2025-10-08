/**
 * @file broker.c
 * @brief Implements the message broker for the publish-subscribe architecture.
 *
 * This file provides the broker implementation, which enables message-based
 * communication between active objects. It supports topic-based subscriptions,
 * message filtering, and queuing for both primary and secondary tasks.
 *
 * The broker ensures reliable message delivery across multiple threads,
 * supporting both Windows (with threads and condition variables) and
 * FreeRTOS (with task queues and semaphores).
 *
 * @author Nathan Ikolo
 * @date February 6, 2025
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdbool.h>
#include "broker.h"

#define MAX_QUEUE_SIZE 	128  /**< Maximum size of the broker queue. */


#ifdef _WIN32
/**
 * @brief Primary message processing thread (Windows).
 * @param param Pointer to the broker instance.
 * @return Always returns 0.
 */
static unsigned __stdcall Broker_PrimaryTask(void *param);

/**
 * @brief Secondary message processing thread (Windows).
 * @param param Pointer to the broker instance.
 * @return Always returns 0.
 */
static unsigned __stdcall Broker_SecondaryTask(void *param);
#elif defined (__linux__)
/**
 * @brief Primary message processing task (Linux).
 * @param param Pointer to the broker instance.
 */
static void *Broker_PrimaryTask(void *param);

/**
 * @brief Secondary message processing task (Linux).
 * @param param Pointer to the broker instance.
 */
static void *Broker_SecondaryTask(void *param);
#else
/**
 * @brief Primary message processing task (FreeRTOS).
 * @param param Pointer to the broker instance.
 */
static void Broker_PrimaryTask(void *param);

/**
 * @brief Secondary message processing task (FreeRTOS).
 * @param param Pointer to the broker instance.
 */
static void Broker_SecondaryTask(void *param);
#endif

/**
 * @brief Initializes a broker queue.
 * @param q Pointer to the queue structure.
 */
static void Broker_Queue_Init(Broker_Queue_t *q) {
#ifdef _WIN32
	q->head = 0;
	q->tail = 0;
	q->count = 0;
	InitializeCriticalSection(&q->lock);
	InitializeConditionVariable(&q->cond);
#elif defined (__linux__)
	q->head = q->tail = q->count = 0;
	pthread_mutex_init(&q->lock, NULL);
    sem_init(&q->items, 0, 0);                 // no items initially
	sem_init(&q->slots, 0, MAX_QUEUE_SIZE);    // all slots free

#else
	q->queue_handle = xQueueCreate(MAX_QUEUE_SIZE, sizeof(message_frame_t));
#endif
}

/**
 * @brief Retrieves a message from the broker queue.
 * @param q Pointer to the queue structure.
 * @param frame Pointer to the message frame that will be populated.
 * @return 1 if successful, 0 otherwise.
 */
static int Broker_Queue_Pop(Broker_Queue_t *q, message_frame_t *frame) {
#ifdef _WIN32
	int success = 0;
	EnterCriticalSection(&q->lock);
	while (q->count == 0) {
		SleepConditionVariableCS(&q->cond, &q->lock, INFINITE);
	}
	if (q->count > 0) {
		*frame = q->buffer[q->head];
		q->head = (q->head + 1) % MAX_QUEUE_SIZE;
		q->count--;
		success = 1;
	}
	LeaveCriticalSection(&q->lock);
	return success;
#elif defined (__linux__)
	int success = 0;
	sem_wait(&q->items);
	pthread_mutex_lock(&q->lock);

	if(q->count > 0){
		*frame = q->buf[q->head];
		q->head = (q->head + 1) % AO_QUEUE_SIZE;
		q->count--;
		success = 1;
	}
	pthread_mutex_unlock(&q->lock);
	sem_post(&q->slots);
	return success;
#else
	return xQueueReceive(q->queue_handle, frame, portMAX_DELAY) == pdTRUE;
#endif
}

/**
 * @brief Adds a message to the broker queue.
 * @param q Pointer to the queue structure.
 * @param frame Pointer to the message frame to enqueue.
 */
static void Broker_Queue_Push(Broker_Queue_t *q, const message_frame_t *frame) {
#ifdef _WIN32
	EnterCriticalSection(&q->lock);
	if (q->count < MAX_QUEUE_SIZE) {
		q->buffer[q->tail] = *frame;
		q->tail = (q->tail + 1) % MAX_QUEUE_SIZE;
		q->count++;
		WakeConditionVariable(&q->cond);
	}
	LeaveCriticalSection(&q->lock);
#elif defined (__linux__)
	sem_wait(&q->slots);
	pthread_mutex_lock(&q->lock);
	q->buf[q->tail] = *frame;
	q->tail = (q->tail + 1) % AO_QUEUE_SIZE;
	q->count++;
	pthread_mutex_unlock(&q->lock);
	sem_post(&q->items);
#else
	xQueueSend(q->queue_handle, frame, portMAX_DELAY);
#endif
}

#ifndef _WIN32
static void Broker_Queue_Push_ISR(Broker_Queue_t *q, const message_frame_t *frame) {
#ifdef _WIN32
	EnterCriticalSection(&q->lock);
	if (q->count < MAX_QUEUE_SIZE) {
		q->buffer[q->tail] = *frame;
		q->tail = (q->tail + 1) % MAX_QUEUE_SIZE;
		q->count++;
		WakeConditionVariable(&q->cond);
	}
	LeaveCriticalSection(&q->lock);
#elif defined (__linux__)
#else
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR(q->queue_handle, frame, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(&xHigherPriorityTaskWoken);
#endif
}
#endif

static topic_entry_t* findOrCreateTopic(broker_t *broker, const topic_config_t *configs) {
	for (int i = 0; i < MAX_TOPICS; i++) {
		switch (configs->type) {
			case EXACT_MATCH:
				if (broker->topics[i].valid && (broker->topics[i].ui.topic == configs->topic)) {
					return &broker->topics[i];
				}
			break;
			case MASK:
				if (broker->topics[i].valid && (broker->topics[i].ui.start == configs->start)
							&& ((broker->topics[i].ui.topic & broker->topics[i].ui.start) == (configs->topic & configs->start))) {
					return &broker->topics[i];
				}
			break;
		}
	}
	for (int i = 0; i < MAX_TOPICS; i++) {
		if (!broker->topics[i].valid) {
			switch (configs->type) {
				case EXACT_MATCH:
					broker->topics[i].valid = 1;
					broker->topics[i].ui.topic = configs->topic;
					broker->topics[i].ui.start = configs->start;
					broker->topics[i].ui.end = configs->end;
					broker->topics[i].ui.type = configs->type;
					return &broker->topics[i];
				case MASK:
					broker->topics[i].valid = 1;
					broker->topics[i].ui.topic = (configs->topic) & configs->start;
					broker->topics[i].ui.start = configs->start;
					broker->topics[i].ui.end = configs->end;
					broker->topics[i].ui.type = configs->type;
					return &broker->topics[i];
			}
		}
	}
	return NULL;
}

/**
 * @brief Initializes a Broker instance.
 *
 * Allocates memory and initializes the message queues and subscription lists.
 *
 * @return Pointer to the initialized broker instance.
 */
broker_t* broker_ctor(void) {
	static broker_t broker = { 0 };

	Broker_Queue_Init(&broker.primary_queue);
	Broker_Queue_Init(&broker.secondary_queue);
	broker.broker1_ready = 0;
	broker.broker2_ready = 1;
#ifdef _WIN32
	if (broker.primary_thread_id == NULL && broker.secondary_thread_id == NULL) {
		InitializeCriticalSection(&broker.sem_handle);
		broker.primary_thread_id = (HANDLE) _beginthreadex(NULL, 0, Broker_PrimaryTask, &broker, 0, NULL);
		broker.secondary_thread_id = (HANDLE) _beginthreadex(NULL, 0, Broker_SecondaryTask, &broker, 0, NULL);
#elif defined (__linux__)

	if (broker.primary_thread_id == 0 && broker.secondary_thread_id == 0) {
		pthread_create(&broker.primary_thread_id, NULL, Broker_PrimaryTask, &broker);
		pthread_create(&broker.secondary_thread_id, NULL, Broker_SecondaryTask, &broker);
		while(!(broker.broker1_ready & broker.broker2_ready));
#else
	if (broker.primary_thread_id == NULL && broker.secondary_thread_id == NULL) {
		broker.sem_handle = xSemaphoreCreateMutex();
		xTaskCreate(Broker_PrimaryTask, "PrimaryMessagePump", 512, &broker, tskIDLE_PRIORITY + 1, &broker.primary_thread_id);
		xTaskCreate(Broker_SecondaryTask, "SecondaryMessagePump", 512, &broker, tskIDLE_PRIORITY + 1, &broker.secondary_thread_id);
#endif


	}

	return &broker;
}

int broker_subscribe(broker_t *broker, topic_config_t *configs, uint16_t length, base_obj_t *subscriber) {
#ifdef _WIN32
	EnterCriticalSection(&broker->sem_handle);
#elif defined (__linux__)
	pthread_mutex_lock(&broker->sem_handle);
#else
	if (xSemaphoreTake(broker->sem_handle, portMAX_DELAY) == pdTRUE) {
#endif
		for (int j = 0; j < length; j++) {
			topic_entry_t *topicEntry = findOrCreateTopic(broker, &configs[j]);
			if (topicEntry == NULL) {
#ifdef _WIN32
    		LeaveCriticalSection(&broker->sem_handle);
#elif defined (__linux__)
    		pthread_mutex_unlock(&broker->sem_handle);
#else
				xSemaphoreGive(broker->sem_handle);
#endif
				return 0;
			}
			for (int i = 0; i < MAX_SUBSCRIBERS_PER_TOPIC; i++) {
				if (!topicEntry->idx[i].active) {
					topicEntry->idx[i].subscriber = subscriber;
					topicEntry->idx[i].active = true;
					topicEntry->count++;
					goto exit_inner_loop;
#ifdef _WIN32
    			LeaveCriticalSection(&broker->sem_handle);
#elif defined (__linux__)
    			pthread_mutex_unlock(&broker->sem_handle);
#else
					xSemaphoreGive(broker->sem_handle);
#endif
				}
			}
			exit_inner_loop: ;
		}

#ifdef _WIN32
    LeaveCriticalSection(&broker->sem_handle);

#elif defined (__linux__)
	pthread_mutex_unlock(&broker->sem_handle);
#else
	}
	xSemaphoreGive(broker->sem_handle);

#endif
	return 0;
}

int broker_unsubscribe(broker_t *broker, topic_config_t *configs, uint16_t length, const base_obj_t *subscriber) {
	int unsubscribed_count = 0; /* Track successful unsubscriptions */

#ifdef _WIN32
    EnterCriticalSection(&broker->sem_handle);
#elif defined (__linux__)
	pthread_mutex_lock(&broker->sem_handle);
#else
	if (xSemaphoreTake(broker->sem_handle, portMAX_DELAY) != pdTRUE) {
		return 0; /* Failed to acquire mutex */
	}
#endif

	for (uint16_t i = 0; i < length; i++) { /* Loop through all requested topics */
		uint32_t topic = (configs[i].type == MASK ? (configs[i].topic & configs[i].start) : configs[i].topic); /* Extract topic ID */

		for (int j = 0; j < MAX_TOPICS; j++) {
			if (broker->topics[j].valid && broker->topics[j].ui.topic == topic) {
				for (int k = 0; k < MAX_SUBSCRIBERS_PER_TOPIC; k++) {
					if (broker->topics[j].idx[k].active && broker->topics[j].idx[k].subscriber == subscriber) {
						/* Remove the subscriber */
						broker->topics[j].idx[k].active = 0;
						broker->topics[j].idx[k].subscriber = NULL;
						unsubscribed_count++;
						break; /* Stop searching once removed */
					}
				}
			}
		}
	}

#ifdef _WIN32
    LeaveCriticalSection(&broker->sem_handle);
#elif defined (__linux__)
	pthread_mutex_unlock(&broker->sem_handle);
#else
	xSemaphoreGive(broker->sem_handle);
#endif

	return unsubscribed_count; /* Return number of successfully removed subscriptions */
}

int broker_publish(broker_t *broker, const message_frame_t frame) {
#ifdef _WIN32
	EnterCriticalSection(&broker->sem_handle);
#elif defined (__linux__)
	pthread_mutex_lock(&broker->sem_handle);
#else
	if (xSemaphoreTake(broker->sem_handle, portMAX_DELAY) == pdTRUE) {
#endif
		for (int i = 0; i < MAX_TOPICS; i++) {
			if (broker->topics[i].valid) {
				switch (broker->topics[i].ui.type) {
					case EXACT_MATCH:
						if (broker->topics[i].ui.topic == frame.signal) {
							for (int j = 0; j < MAX_SUBSCRIBERS_PER_TOPIC; j++) {
								if (broker->topics[i].idx[j].active && broker->topics[i].idx[j].subscriber != NULL) {
									broker->topics[i].idx[j].subscriber->vptr->post(broker->topics[i].idx[j].subscriber, frame);
								}
							}
						}
					break;
					case MASK:
						if ((broker->topics[i].ui.start != 0) && ((broker->topics[i].ui.topic & broker->topics[i].ui.start) == (frame.signal & broker->topics[i].ui.start))) {
							for (int j = 0; j < MAX_SUBSCRIBERS_PER_TOPIC; j++) {
								if (broker->topics[i].idx[j].active && broker->topics[i].idx[j].subscriber != NULL) {
									broker->topics[i].idx[j].subscriber->vptr->post(broker->topics[i].idx[j].subscriber, frame);
								}
							}
						}
					break;
				}
			}
		}
#ifdef _WIN32
    	LeaveCriticalSection(&broker->sem_handle);
#elif defined (__linux__)
	pthread_mutex_unlock(&broker->sem_handle);
#else
		xSemaphoreGive(broker->sem_handle);
#endif
		return 0;

#ifdef _WIN32
	LeaveCriticalSection(&broker->sem_handle);
#elif defined (__linux__)
	pthread_mutex_unlock(&broker->sem_handle);
#else
	}
	xSemaphoreGive(broker->sem_handle);
#endif
	return 0;
}

void broker_post(broker_t *broker, message_frame_t frame, int primary) {
	if (primary == PRIMARY_QUEUE) {
		Broker_Queue_Push(&broker->primary_queue, &frame);
	} else {
		Broker_Queue_Push(&broker->secondary_queue, &frame);
	}
}


#ifndef _WIN32
void broker_post_ISR(broker_t *broker, message_frame_t frame, int primary) {
	if (primary == PRIMARY_QUEUE) {
		Broker_Queue_Push_ISR(&broker->primary_queue, &frame);
	} else {
		Broker_Queue_Push_ISR(&broker->secondary_queue, &frame);
	}
}

void broker_set_filter(uint32_t msgId, uint32_t mask, uint8_t filterType) {
	const broker_t *broker = broker_ctor();
	if (broker != NULL) {
#if defined (__linux__)
#else
	broker->ext_port->set_filter(msgId, mask, filterType);
#endif
	}
}

#endif

#ifdef _WIN32
unsigned __stdcall Broker_PrimaryTask(void *param) {
#elif defined (__linux__)
	void *Broker_PrimaryTask(void *param) {
#else
static void Broker_PrimaryTask(void *param) {
#endif
	broker_t *broker = (broker_t*) param;
	message_frame_t frame;
	broker->broker1_ready = 1;

	while (1) {
		if (Broker_Queue_Pop(&broker->primary_queue, &frame)) {
			broker_publish(broker,frame);
			memset(&frame, 0, sizeof(message_frame_t));
		}
	}
#ifdef _WIN32
	return 0;
#elif defined (__linux__)
	return NULL;
#endif
}

#ifdef _WIN32
unsigned __stdcall Broker_SecondaryTask(void *param) {
#elif defined (__linux__)
	void *Broker_SecondaryTask(void *param){
#else
static void Broker_SecondaryTask(void *param) {
#endif
	broker_t *broker = (broker_t*) param;
	message_frame_t frame;
	broker->broker2_ready = 1;

	while (1) {
		if (Broker_Queue_Pop(&broker->secondary_queue, &frame)) {
			broker_publish(broker, frame);
			memset(&frame, 0, sizeof(message_frame_t));
		}
	}
#ifdef _WIN32
	return 0;
#elif defined (__linux__)
	return NULL;
#endif
}

#ifdef __cplusplus
}
#endif
