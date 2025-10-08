/**
 * @file ao_can.h
 * @brief Defines the CAN Active Object structure and its related functions.
 *
 * This file provides the structure definition and function declarations
 * for the CAN active object, which integrates with a message broker.
 * It supports message reception, filtering, and processing within an
 * event-driven system.
 *
 * The CAN active object follows the Active Object pattern and is responsible
 * for handling CAN bus communication asynchronously.
 *
 * @author Nathan Ikolo
 * @date February 12, 2025
 */

#ifndef INCLUDE_AO_CAN_H_
#define INCLUDE_AO_CAN_H_

#ifdef __cplusplus
extern "C" {
#endif
#if !defined (_WIN32) || !defined (__linux__)
#else
#include "active_object.h"
#include <can_bus.h>
#include <sys_timer.h>

/**
 * @struct can_obj_t
 * @brief Represents a CAN active object.
 *
 * This structure extends the base active object (`base_obj_t`) and includes
 * CAN-specific functionalities such as filter configuration and timer management.
 */
typedef struct {
    base_obj_t super; /**< Inherits from base active object. */
    timers_t *timerManager; /**< Pointer to the system timer manager. */

    /**
     * @brief Sets a CAN message filter.
     *
     * This function pointer allows filtering of CAN messages based on the
     * message ID and mask.
     *
     * @param msgId The message ID to filter.
     * @param mask The bitmask applied to the message ID.
     * @param filterType Type of filter (e.g., exact match, mask-based).
     */
    void (*set_filter)(uint32_t msgId, uint32_t mask, uint8_t filterType);
} can_obj_t;

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
can_obj_t *can_ctor(broker_t *broker, char *name);

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
void CanIf_RxIndication(const Can_HwType* Mailbox, const PduInfoType* PduInfoPtr);

#ifdef __cplusplus
}
#endif
#endif
#endif /* INCLUDE_AO_CAN_H_ */
