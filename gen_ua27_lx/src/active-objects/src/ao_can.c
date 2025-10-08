/**
 * @file ao_can.c
 * @brief Implements the Active Object for CAN communication with timeout tracking.
 *
 * This file defines the CAN active object, which integrates with the event-driven
 * architecture. It manages message transmission, reception, filtering, and timeouts.
 * The system tracks pending messages and ensures reliable communication via a
 * circular buffer for timeout management.
 *
 * @author Nathan Ikolo
 * @date February 12, 2025
 */

#ifdef __cplusplus
extern "C" {
#endif
#if !defined (_WIN32) || !defined (__linux__)
#else
#include <string.h>
#include <stdlib.h>
#include <broker.h>
#include "ao_can.h"
#include <sys_timer.h>

#define MAX_PENDING_FRAMES 128 /**< Maximum number of pending CAN frames tracked */

/**
 * @struct can_timeout_entry_t
 * @brief Represents a CAN frame with an associated timeout.
 */
typedef struct {
	message_frame_t frame; /**< The CAN frame */
	timer_callback_entry_t *timerEntry; /**< Timer entry associated with the frame */
} can_timeout_entry_t;

// Circular buffer for pending CAN frames
static can_timeout_entry_t pendingFrames[MAX_PENDING_FRAMES];
static volatile uint8_t head = 0; /**< Circular buffer head */
static volatile uint8_t tail = 0; /**< Circular buffer tail */
static volatile uint8_t count = 0; /**< Number of elements in the buffer */

static broker_t *broker_ptr = NULL;
static can_obj_t *can_ptr = NULL;

// Internal function prototypes
static void ao_can_timeout(void *context);
static void ao_can_send_with_timeout(can_obj_t *me, const message_frame_t *frame);
static void ao_can_clear_timeout(can_obj_t *me, message_frame_t frame);

/**
 * @brief Dispatch function for processing received CAN frames.
 *
 * @param me Pointer to the active object.
 * @param frame Pointer to the received message frame.
 */
static void dispatch(base_obj_t *const me, const message_frame_t *frame) {
	message_frame_t frame1 = { 0 };
	memcpy(&frame1, frame, sizeof(frame1));
	switch (frame->topicOrServiceId) {
		case INTERNAL_SYSTEM_TIME:
#ifdef _WIN32
		me->last_heartbeat_time = (uint32_t)get_time_ms();
#else
			me->last_heartbeat_time = xTaskGetTickCount();
#endif
		break;
		default:

			if (frame->type == MESSAGE_TYPE_REQUEST) {
				ao_can_send_with_timeout((can_obj_t*) me, &frame1);
			} else {
				llce_can_write(1, frame1);
			}

		break;
	}

}

/**
 * @brief Sets a CAN filter.
 *
 * @param msgId The CAN message ID.
 * @param mask The CAN filter mask.
 * @param filterType The type of filter (MASK, RANGE, EXACT_MATCH).
 */
static void set_filter(uint32_t msgId, uint32_t mask, uint8_t filterType) {
	message_frame_t frame = { 0 };
	message_decode_can_id(msgId, &frame);
	Can_SetFilterType filter = { 0 };

	switch (filterType) {
		case MASK:
			filter.Hrh = frame.srcAddress;
			filter.eFilterType = LLCE_CAN_ENTRY_CFG_MASKED;
			filter.uIdMask = (uint32) ((mask & CAN_43_LLCE_MAX_IDMASK ) | LLCE_CAN_MB_RTR_U32 | LLCE_CAN_MB_IDE_U32);
			filter.uMessageId = (uint32) (msgId & CAN_43_LLCE_MAX_IDMASK );
			filter.eIdType = LLCE_CAN_EXTENDED;
			filter.u16MbCount = (uint16) 0x10;
			filter.u8RWInterface = frame.srcAddress;
			filter.eFilterMbLength = USE_SHORT_MB;
			Can_43_LLCE_SetFilter(&filter);
		break;
		case RANGE:
		break;
		case EXACT_MATCH:
		break;
	}
}

/**
 * @brief Constructs and initializes a CAN active object.
 *
 * This function initializes a CAN active object (`can_obj_t`) and registers
 * it with the provided broker.
 *
 * @param broker Pointer to the message broker responsible for event routing.
 * @param name Name of the CAN active object.
 * @return Pointer to the singleton CAN active object.
 */
can_obj_t* can_ctor(broker_t *broker, char *name) {
	static can_obj_t *me;
	if (me == NULL) {
		me = (can_obj_t*) malloc(sizeof(can_obj_t));
		memset(me, 0, sizeof(can_obj_t));
		INIT_BASE(me, broker, name, system_id, NULL);
		me->set_filter = &set_filter;
		can_ptr = me;
		llce_can_init();
	}
	return me;
}

/**
 * @brief Handles received CAN messages.
 *
 * This function is invoked when a CAN message is received. It processes
 * the message and passes it to the appropriate handler.
 *
 * @param Mailbox Pointer to the hardware CAN mailbox structure.
 * @param PduInfoPtr Pointer to the CAN Protocol Data Unit (PDU) containing
 *                   the received message payload.
 */
void CanIf_RxIndication(const Can_HwType *Mailbox, const PduInfoType *PduInfoPtr) {
	message_frame_t frame;

	message_decode_can_id(Mailbox->CanId & 0x1FFFFFFF, &frame);
	frame.meta.payload_length = PduInfoPtr->SduLength;
	memcpy(&frame.payload, PduInfoPtr->SduDataPtr, PduInfoPtr->SduLength);
	broker_ptr = broker_ctor();
	if (broker_ptr != NULL) {
		if (frame.type == MESSAGE_TYPE_RESPONSE) {
			ao_can_clear_timeout(can_ptr, frame);
		}
		broker_post_ISR(broker_ptr, frame, frame.priority != 3);
	}
}

/**
 * @brief Sends a CAN frame with an associated timeout.
 *
 * Adds the frame to a circular buffer and starts a timeout timer.
 *
 * @param me Pointer to the CAN object.
 * @param frame Pointer to the message frame to be sent.
 */
void ao_can_send_with_timeout(can_obj_t *me, const message_frame_t *frame) {
	if (!me || !me->timerManager)
		return;

	if (count >= MAX_PENDING_FRAMES) {
		head = (head + 1) % MAX_PENDING_FRAMES; // Overwrite oldest
		count--;
	}

	uint8_t index = tail;
	memcpy(&pendingFrames[index].frame, frame, sizeof(message_frame_t));

	// Add timeout callback
	pendingFrames[index].timerEntry = me->timerManager->add_callback(TIMER_100ms, ao_can_timeout, &pendingFrames[index], 1);

	if (pendingFrames[index].timerEntry) {
		me->timerManager->arm(pendingFrames[index].timerEntry);
	}

	// Move tail forward (circular buffer logic)
	tail = (tail + 1) % MAX_PENDING_FRAMES;
	count++;

	// Send CAN frame
	llce_can_write(frame->type, *frame);
}

/**
 * @brief Handles CAN frame timeouts.
 *
 * Called when a frame timeout occurs, triggering a timeout message.
 *
 * @param context Pointer to the timeout entry.
 */
void ao_can_timeout(void *context) {
	can_timeout_entry_t *entry = (can_timeout_entry_t*) context;
	if (entry != NULL) {
#ifdef _WIN32
        printf("Timeout: CAN ID %X, Correlation ID %X\n", message_encode_can_id(&entry->frame), entry->frame.meta.correlationId);
#else
		entry->frame.mode = MESSAGE_MODE_INTERNAL;
		entry->frame.type = MESSAGE_TYPE_TIMEOUT;
		broker_ptr = broker_ctor();
		if (broker_ptr != NULL) {
			broker_post(broker_ptr, entry->frame, PRIMARY_QUEUE);
		}
#endif
	}
}

/**
 * @brief Clears a timeout entry when a response is received.
 *
 * Removes the corresponding entry from the circular buffer.
 *
 * @param me Pointer to the CAN object.
 * @param frame The received message frame that acknowledges a pending request.
 */
void ao_can_clear_timeout(can_obj_t *me, message_frame_t frame) {
	uint8_t i = head;
	uint8_t processed = 0;

	while (processed < count) {
		if ((message_encode_can_id(&pendingFrames[i].frame) == message_encode_can_id(&frame)) && (pendingFrames[i].frame.meta.correlationId == frame.meta.correlationId)) {

			if (pendingFrames[i].timerEntry) {
				me->timerManager->disarm(pendingFrames[i].timerEntry);
				me->timerManager->remove_callback(TIMER_100ms, ao_can_timeout, pendingFrames[i].timerEntry->context);
				pendingFrames[i].timerEntry = NULL;
			}

			// Shift remaining entries forward
			uint8_t j = i;
			while (j != tail) {
				uint8_t next = (j + 1) % MAX_PENDING_FRAMES;
				pendingFrames[j] = pendingFrames[next];
				j = next;
			}

			// Update tail and count
			tail = (tail == 0) ? MAX_PENDING_FRAMES - 1 : tail - 1;
			count--;
			return;
		}
		i = (i + 1) % MAX_PENDING_FRAMES;
		processed++;
	}
}
#endif
#ifdef __cplusplus
}
#endif

