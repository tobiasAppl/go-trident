/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef SENDER_BARRIER_H
#define SENDER_BARRIER_H

#include <trident/layer1/layer1_types.h>

STATE_RESULT sb_start(MACHINESTATE* machinestate);

STATE_RESULT sb_end(MACHINESTATE* machinestate);

STATE_RESULT sb0_set_tx_one(MACHINESTATE* machinestate);

STATE_RESULT sb1_wait_for_rx_one(MACHINESTATE* machinestate);

#endif
