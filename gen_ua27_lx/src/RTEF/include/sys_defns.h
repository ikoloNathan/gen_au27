/**
 * @file sys_defns.h
 * @brief Defines system-wide structures and enumerations.
 *
 * This file provides common definitions used across the system, including
 * system states, system information, hardware details, and software metadata.
 *
 * It encapsulates key system parameters required for monitoring and
 * operational state management.
 *
 * @author Nathan Ikolo
 * @date February 22, 2025
 */

#ifndef SYSDEFNS_H_
#define SYSDEFNS_H_

#include <stdint.h>



/**
 * @enum sys_state_t
 * @brief Defines possible system operational states.
 */
typedef enum {
    SYSTEM_STATE_INIT,        /**< System is initializing. */
    SYSTEM_STATE_OK, /**< System is in normal operational mode. */
    SYSTEM_STATE_ALARM,       /**< System has encountered an alaram. */
    SYSTEM_STATE_LOADER,      /**< System is in loader mode (firmware update). */
    SYSTEM_STATE_ERROR  /**< System is in maintenance mode. */
} sys_state_t;

/**
 * @struct hw_info_t
 * @brief Represents hardware version information.
 *
 * This structure provides details about the hardware version, including
 * unique identifiers, versioning, and checksum validation.
 */
typedef struct {
	char id[64];   /**< Unique hardware identifier. */
	char revision[6]; /**< Hardware major version number. */
    uint16_t crc;  /**< CRC checksum for integrity verification. */
} hw_info_t;

/**
 * @struct sw_info_t
 * @brief Represents software version and metadata.
 *
 * This structure encapsulates versioning details, build date, and
 * software size for tracking and validation.
 */
typedef struct {
    char id[64];   /**< Unique software identifier. */
    char version[6]; /**< Software version number. */
    char date[64];   /**< Software build date. */
    uint16_t crc;  /**< CRC checksum for integrity verification. */
} sw_info_t;

/**
 * @struct sys_info_t
 * @brief Encapsulates system state, status information, and error handling.
 *
 * This structure contains details about the system's current operational state,
 * uptime, and error codes.
 */
typedef struct {
    sys_state_t current_state;   /**< Current system state */
    uint8_t status;              /**< Status byte (e.g., error codes, flags) */
    double up_time;             /**< System uptime in seconds */
    sw_info_t *sw;
    hw_info_t *hw;
    uint32_t error_code;          /**< Error code (if any) */
    uint8_t reserved[4];         /**< Reserved for future use */
} sys_info_t;




#endif /* SYSDEFNS_H_ */
