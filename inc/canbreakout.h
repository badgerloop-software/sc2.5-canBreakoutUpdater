#ifndef CANBREAKOUT_H
#define CANBREAKOUT_H

#include "zephyrcan.h"
#include "const.h"

class CANBreakout : public ZephyrCAN {
    public:
        CANBreakout(const struct device *canDevice, const uint32_t targetIDList[], size_t targetIDListSize, uint32_t frequency = CAN_FREQUENCY);
        void readHandler(struct can_frame * msg);
        int sendBreakoutData();
};

#endif
