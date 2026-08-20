#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>

void Controller_Init(void);
void Controller_Task(void);
void Controller_SetLights(const uint8_t light_values[7]);

#endif
