/*
 * hal_gpio.h
 *
 *  Created on: Oct 8, 2025
 *      Author: nathan
 */

#ifndef SRC_HAL_INCLUDES_HAL_GPIO_H_
#define SRC_HAL_INCLUDES_HAL_GPIO_H_

#include <gpiod.h>
#include <stdint.h>
#include <stdbool.h>

#define GPIO_CHIP_PATH(x) "/dev/gpiochip" #x

typedef struct {
    struct gpiod_chip *chip;
    struct gpiod_line_request *request;
    const char *name;
    uint8_t offset;
    uint8_t dir;
    uint8_t state;
    bool has_fd;
    int fd;
} io_ctrl_t;

void io_register(io_ctrl_t *ctrl, uint8_t chip_num) ;

void io_set_output(io_ctrl_t *ctrl, uint8_t value);

uint8_t io_read_pin(io_ctrl_t *ctrl);

#endif /* SRC_HAL_INCLUDES_HAL_GPIO_H_ */
