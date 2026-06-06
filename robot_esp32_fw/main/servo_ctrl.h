#ifndef SERVO_CTRL_H
#define SERVO_CTRL_H

#include <stdint.h>

void servo_init(void);
void servo_update_pwm(int16_t left_x, int16_t left_y, int16_t right_y, int gripper_cmd);

#endif