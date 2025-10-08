/**
 * @file ao_udp.h
 * @brief Defines the UDP Active Object.
 *
 * This file provides the structure and function declarations for the UDP
 * active object (`udp_obj_t`). It integrates with the Active Object framework
 * and facilitates event-driven communication over a UDP socket.
 *
 * The UDP active object is only available for Windows-based systems (`_WIN32`).
 *
 * @author Nathan Ikolo
 * @date February 18, 2025
 */

#ifndef AOUDP_H_
#define AOUDP_H_

#ifdef __cplusplus
extern "C" {
#endif



#include "active_object.h"

/**
 * @struct udp_obj_t
 * @brief Represents the UDP Active Object.
 *
 * This structure extends `Base_obj_t` and integrates a separate thread
 * for handling UDP communication asynchronously.
 */
typedef struct udp_obj {
    base_obj_t super; /**< Inherits from the base active object. */
#ifdef _WIN32
    HANDLE thread;    /**< Handle to the UDP processing thread. */
#elif defined (__linux__)
    pthread_t thread; /* POSIX worker thread */
#endif
} udp_obj_t;

/**
 * @brief Constructs and initializes the UDP Active Object.
 *
 * This function initializes the UDP active object, linking it to the message
 * broker for event-based communication.
 *
 * @param me Pointer to the UDP active object instance.
 * @param broker Pointer to the message broker managing communication.
 * @param name Name of the UDP active object instance.
 */
udp_obj_t * udp_ctor(broker_t *broker, char *name);


#ifdef __cplusplus
}
#endif

#endif /* AOUDP_H_ */
