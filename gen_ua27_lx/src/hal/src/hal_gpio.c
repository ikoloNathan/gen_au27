/*
 * hal_gpio.c
 *
 *  Created on: Oct 8, 2025
 *      Author: nathan
 */

#include <stdio.h>
#include "hal_gpio.h"


void io_register(io_ctrl_t *ctrl, uint8_t chip_num) {

	struct gpiod_line_settings *settings;
	struct gpiod_line_config *config;
	struct gpiod_request_config *req_cfg;

	if (ctrl == NULL)
		return;

	ctrl->chip = gpiod_chip_open(GPIO_CHIP_PATH(chip_num));
	if (!ctrl->chip) {
		perror("open chip");
		return;
	}

	settings = gpiod_line_settings_new();
	gpiod_line_settings_set_direction(settings, ctrl->dir);
	gpiod_line_settings_set_output_value(settings, ctrl->state);

	config = gpiod_line_config_new();
	gpiod_line_config_add_line_settings(config,(const unsigned int *) &ctrl->offset, 1, settings);


	req_cfg = gpiod_request_config_new();
	gpiod_request_config_set_consumer(req_cfg,ctrl->name);
	ctrl->request = gpiod_chip_request_lines(ctrl->chip, req_cfg, config);
	if (!ctrl->request) {
		perror("request lines");
		return ;
	}
	if(ctrl->has_fd)
		ctrl->fd = gpiod_line_request_get_fd(ctrl->request);
	gpiod_line_settings_free(settings);
	gpiod_line_config_free(config);
	gpiod_request_config_free(req_cfg);
}


void io_set_output(io_ctrl_t *ctrl, uint8_t value){
	gpiod_line_request_set_value(ctrl->request,ctrl->offset,value);
}

uint8_t io_read_pin(io_ctrl_t *ctrl){
	return gpiod_line_request_get_value(ctrl->request,ctrl->offset);
}
