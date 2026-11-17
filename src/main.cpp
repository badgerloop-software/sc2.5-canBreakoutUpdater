#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/flash_map.h>
#include "const.h"
#include "canbreakout.h"
#include "updatemanager_eth.h"
#include "updatemanager_can.h"
#include "coordinator.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if PARTITION_EXISTS(payload_partition)
void updateCoordinatorTask(void * p1, void * p2, void * p3) {
    while (1) {
        board_id_t boardId = waitForPayload();
        beginCanSend(boardId);
        waitCanSendDone();
    }
}
#endif

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

#if PARTITION_EXISTS(payload_partition)
K_THREAD_DEFINE(update_manager_eth, 4096, ethernetUpdateTask, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(update_manager_can, 4096, canUpdateSendTask, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(update_coordinator, 1024, updateCoordinatorTask, NULL, NULL, NULL, 7, 0, 0);
#endif
K_THREAD_DEFINE(update_manager_can2, 4096, canUpdateTask, NULL, NULL, NULL, 7, 0, 0);
