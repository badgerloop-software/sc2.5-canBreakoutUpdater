#include "canbreakout.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_rx_breakout, LOG_LEVEL_INF);

CANBreakout::CANBreakout(const struct device * canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency) : 
    ZephyrCAN(canDevice, targetIDList, targetIDListSize, frequency) {}

void CANBreakout::readHandler(struct can_frame * msg) {
    if ((msg->id) == EXAMPLE_ID) {
    }
}

int CANBreakout::sendBreakoutData() {
    return 0;
}
