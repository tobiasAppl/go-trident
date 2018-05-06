/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef RECEIVER_BARRIER_H
#define RECEIVER_BARRIER_H

#include <trident/layer1/layer1_types.h>

STATE_RESULT rb_start(MACHINESTATE* machinestate);

STATE_RESULT rb_end(MACHINESTATE* machinestate);

STATE_RESULT rb0_wait_for_tx_one(MACHINESTATE* machinestate);

STATE_RESULT rb1_set_rx_one(MACHINESTATE* machinestate);

#endif //RECEIVER_BARRIER_H
