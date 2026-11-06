#ifndef CONST_H
#define CONST_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#define USING_FDCAN     0

#define EXAMPLE_ID      0xFF

static const struct device * const canDev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static const uint32_t CAN_IDS[] = {EXAMPLE_ID};
static const uint32_t CAN_FREQUENCY = 250000;

#endif
