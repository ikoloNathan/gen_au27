/*
 * hal_gpio.h
 *
 *  Created on: Oct 8, 2025
 *      Author: nathan
 */

/* hal_gpio.h */
/**
 * @file hal_gpio.h
 * @brief Thin, single-line GPIO wrapper over libgpiod v2.
 *
 * This HAL provides a minimal request/IO interface around libgpiod v2 using a
 * single GPIO line per handle. It supports configuring direction and initial
 * output value, reading and writing the line, and orderly teardown.
 *
 * @ingroup HAL_GPIO
 */
#ifndef SRC_HAL_INCLUDES_HAL_GPIO_H_
#define SRC_HAL_INCLUDES_HAL_GPIO_H_

#include <gpiod.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @defgroup HAL_GPIO HAL: GPIO (libgpiod v2)
 * @brief Minimal GPIO helper built on libgpiod v2.
 *
 * Typical usage:
 * @code
 * io_ctrl_t io = {
 *     .name   = "my_consumer",
 *     .offset = 17,          // line offset within the chip
 *     .dir    = 1,           // 1 = output
 *     .state  = 0,           // initial level when output
 *     .has_fd = false,
 * };
 * int rc = io_register(&io, 0);     // open /dev/gpiochip0 and request line 17
 * if (rc == 0) {
 *     io_set_output(&io, 1);
 *     uint8_t v;
 *     io_read_pin(&io, &v);
 *     io_unregister(&io);
 * }
 * @endcode
 * @{
 */

/**
 * @brief Per-line GPIO control block.
 *
 * One instance manages a single requested GPIO line on a single chip.
 */
typedef struct {
    /**
     * @brief Open chip handle (owned).
     *
     * Set by ::io_register. Closed by ::io_unregister.
     */
    struct gpiod_chip *chip;

    /**
     * @brief Active line request (owned).
     *
     * Created by ::io_register. Released by ::io_unregister.
     */
    struct gpiod_line_request *request;

    /**
     * @brief Optional consumer name shown in sysfs/debug.
     *
     * If `NULL`, a default name is used.
     */
    const char *name;

    /**
     * @brief GPIO line offset within the chip (not BCM/board numbering).
     */
    uint8_t offset;

    /**
     * @brief Direction: 0 = input, 1 = output.
     *
     * Mapped internally to libgpiod enums.
     */
    uint8_t dir;

    /**
     * @brief Initial output state: 0 = inactive/low, 1 = active/high.
     *
     * Only meaningful when requesting as output.
     */
    uint8_t state;

    /**
     * @brief If true, fetch the request file descriptor after requesting.
     */
    bool has_fd;

    /**
     * @brief Line request file descriptor (valid only if @ref has_fd is true).
     */
    int fd;

    /**
     * @brief Chip number passed to ::io_register (e.g., 0 for /dev/gpiochip0).
     */
    uint8_t chip_num;
} io_ctrl_t;

/**
 * @brief Open the GPIO chip, configure the line, and create a request.
 *
 * @param[in,out] ctrl       Pre-filled control block (see field docs). On
 *                           success, @ref chip, @ref request, and optionally
 *                           @ref fd are populated.
 * @param[in]     chip_num   Target chip number (0 => /dev/gpiochip0).
 * @return 0 on success; negative errno-like value on failure:
 *         -1 null parameter, -2 chip open failed, -3 setup alloc failed,
 *         -4 line request failed.
 *
 * @note Uses libgpiod v2 API: gpiod_chip_open_by_number(), line settings, etc.
 */
int  io_register(io_ctrl_t *ctrl, uint8_t chip_num);

/**
 * @brief Set the output value of the requested line.
 *
 * @param[in] ctrl   Valid control block returned by ::io_register.
 * @param[in] value  0 = inactive/low, 1 = active/high.
 * @return 0 on success; negative on failure (-1 invalid state, libgpiod error otherwise).
 *
 * @warning Calling this on an input-configured line is undefined for hardware;
 *          libgpiod will error. Keep @ref dir consistent with usage.
 */
int  io_set_output(io_ctrl_t *ctrl, uint8_t value);

/**
 * @brief Read the current value of the requested line.
 *
 * @param[in]  ctrl       Valid control block returned by ::io_register.
 * @param[out] value_out  Receives 0 (inactive/low) or 1 (active/high).
 * @return 0 on success; negative on failure (-1 invalid args, -2 read error).
 */
int  io_read_pin(io_ctrl_t *ctrl, uint8_t *value_out);

/**
 * @brief Release the requested line and close the chip.
 *
 * Safe to call multiple times. After return, @ref request and @ref chip are NULL.
 *
 * @param[in,out] ctrl  Control block to clean up.
 */
void io_unregister(io_ctrl_t *ctrl);

/** @} */ /* end of HAL_GPIO group */

#endif /* SRC_HAL_INCLUDES_HAL_GPIO_H_ */

