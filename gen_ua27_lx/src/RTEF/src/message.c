/**
 * @file message.c
 * @brief Implements message encoding, decoding, serialization, and deserialization.
 *
 * This file provides the implementation for handling messages in a CAN-based
 * communication system. It includes functions for encoding and decoding
 * 29-bit CAN IDs, as well as serializing and deserializing message payloads.
 *
 * The message frame structure is designed to support various message types,
 * ensuring reliable communication within the system.
 *
 * @author Nathan Ikolo
 * @date February 6, 2025
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "message.h"





#ifdef __cplusplus
}
#endif
