#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/dfu/flash_img.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/dfu/mcuboot.h>
#include "const.h"
#include "otap.h"
#include "canbreakout.h"
#include "updatemanager_eth.h"
#include "updatemanager_can.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void) {
    LOG_INF("Hello World DEVELOP----- %s\n", CONFIG_BOARD_TARGET);
    CANBreakout canBus(canDev, CAN_IDS, sizeof(CAN_IDS)/sizeof(CAN_IDS[0]));
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

K_THREAD_DEFINE(update_manager_eth, 4096, ethernetUpdateTask, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(update_manager_can, 4096, canUpdateTask, NULL, NULL, NULL, 7, 0, 0);
