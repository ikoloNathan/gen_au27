/*
 * hal_gpio.c
 *
 *  Created on: Oct 8, 2025
 *      Author: nathan
 */

/* hal_gpio.c */
/**
 * @file hal_gpio.c
 * @brief Implementation of a minimal libgpiod v2 GPIO helper.
 *
 * See @ref HAL_GPIO for usage and design notes.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "hal_gpio.h"


/**
 * @brief Open a GPIO chip by numeric index (e.g., 0 → /dev/gpiochip0).
 *
 * Builds the device path using @c snprintf and opens it via
 * ::gpiod_chip_open(). If the formatted path would not fit in the
 * internal buffer, the function sets @c errno to @c ENAMETOOLONG and
 * returns @c NULL.
 *
 * @param chip_num Numeric chip index (0 ⇒ /dev/gpiochip0).
 * @return Pointer to an open ::gpiod_chip on success; @c NULL on failure.
 *
 * @retval NULL If @c snprintf fails or overflows the buffer, with
 *         @c errno set to @c ENAMETOOLONG. If ::gpiod_chip_open()
 *         fails, @c errno is set by the underlying call.
 *
 * @note This helper does not log; callers may use @c perror or
 *       @c strerror(errno) after a @c NULL return.
 * @warning The returned handle must be closed with ::gpiod_chip_close()
 *          exactly once when no longer needed.
 */
static struct gpiod_chip *gpiod_chip_open_by_number(uint8_t chip_num){

	char path[32] = {0};
	int n = snprintf(path,sizeof(path),"/dev/gpiochip%u", (unsigned)chip_num);
	if(n < 0 || (size_t)n >= sizeof(path)){
		errno = ENAMETOOLONG;
		return NULL;
	}
	return gpiod_chip_open(path);
}

/**
 * @brief Convert 0/1 direction to libgpiod direction enum.
 * @param d 0=input, 1=output.
 * @return GPIOD_LINE_DIRECTION_INPUT or GPIOD_LINE_DIRECTION_OUTPUT.
 */
static inline enum gpiod_line_direction to_dir(uint8_t d) {
    return d ? GPIOD_LINE_DIRECTION_OUTPUT : GPIOD_LINE_DIRECTION_INPUT;
}

/**
 * @brief Convert 0/1 level to libgpiod value enum.
 * @param v 0=inactive/low, 1=active/high.
 * @return GPIOD_LINE_VALUE_INACTIVE or GPIOD_LINE_VALUE_ACTIVE.
 */
static inline enum gpiod_line_value to_val(uint8_t v) {
    return v ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
}

int io_register(io_ctrl_t *ctrl, uint8_t chip_num) {
    if (!ctrl) return -1;

    ctrl->chip = gpiod_chip_open_by_number(chip_num);
    if (!ctrl->chip) {
        perror("gpiod_chip_open_by_number");
        return -2;
    }
    ctrl->chip_num = chip_num;

    int rc = -3;
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (!settings) { perror("gpiod_line_settings_new"); goto fail_chip; }

    gpiod_line_settings_set_direction(settings, to_dir(ctrl->dir));
    gpiod_line_settings_set_output_value(settings, to_val(ctrl->state));

    struct gpiod_line_config *config = gpiod_line_config_new();
    if (!config) { perror("gpiod_line_config_new"); goto fail_settings; }

    /* Correct width & alignment for offsets array (libgpiod expects unsigned int). */
    unsigned int off = ctrl->offset;
    if (gpiod_line_config_add_line_settings(config, &off, 1, settings) < 0) {
        perror("gpiod_line_config_add_line_settings");
        goto fail_config;
    }

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    if (!req_cfg) { perror("gpiod_request_config_new"); goto fail_config; }

    gpiod_request_config_set_consumer(req_cfg, ctrl->name ? ctrl->name : "hal_gpio");

    ctrl->request = gpiod_chip_request_lines(ctrl->chip, req_cfg, config);
    gpiod_request_config_free(req_cfg);

    if (!ctrl->request) {
        perror("gpiod_chip_request_lines");
        rc = -4;
        goto fail_config;
    }

    if (ctrl->has_fd) {
        ctrl->fd = gpiod_line_request_get_fd(ctrl->request);
        if (ctrl->fd < 0) {
            perror("gpiod_line_request_get_fd");
            /* Non-fatal: continue without fd. */
        }
    }

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(config);
    return 0;

fail_config:
    gpiod_line_config_free(config);
fail_settings:
    gpiod_line_settings_free(settings);
fail_chip:
    gpiod_chip_close(ctrl->chip);
    ctrl->chip = NULL;
    return rc;
}

int io_set_output(io_ctrl_t *ctrl, uint8_t value) {
    if (!ctrl || !ctrl->request) return -1;
    unsigned int off = ctrl->offset;
    int rc = gpiod_line_request_set_value(ctrl->request, off, to_val(value));
    if (rc < 0) perror("gpiod_line_request_set_value");
    return rc;
}

int io_read_pin(io_ctrl_t *ctrl, uint8_t *value_out) {
    if (!ctrl || !ctrl->request || !value_out) return -1;
    unsigned int off = ctrl->offset;
    int val = gpiod_line_request_get_value(ctrl->request, off);
    if (val < 0) {
        perror("gpiod_line_request_get_value");
        return -2;
    }
    *value_out = (val == GPIOD_LINE_VALUE_ACTIVE) ? 1 : 0;
    return 0;
}

void io_unregister(io_ctrl_t *ctrl) {
    if (!ctrl) return;
    if (ctrl->request) {
        gpiod_line_request_release(ctrl->request); /* libgpiod v2 */
        ctrl->request = NULL;
    }
    if (ctrl->chip) {
        gpiod_chip_close(ctrl->chip);
        ctrl->chip = NULL;
    }
}
