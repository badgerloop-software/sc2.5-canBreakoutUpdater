#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include "const.h"
#include "otap.h"
#include "examplecan.h"
#include "updatemanager_eth.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void) {
    LOG_INF("Hello World DEVELOP----- %s\n", CONFIG_BOARD_TARGET);
    ExampleCAN canBus(canDev, CAN_IDS, sizeof(CAN_IDS)/sizeof(CAN_IDS[0]), CAN_FREQUENCY);
    canBus.begin();

    if (canBus.canStatus() != CAN_OK) {
        LOG_ERR("CAN init failed: %d", canBus.canStatus());
        return -1;
    }
    
    while(1) {
        if (canBus.canStatus() != CAN_OK)
        {
            break;
        }

        k_msleep(1000);
    }
    
    return 0;
}

K_THREAD_DEFINE(update_manager, 4096, ethernetUpdateTask, NULL, NULL, NULL, 7, 0, 0);
