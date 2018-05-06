/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include "sender_barrier.h"

#include <trident/util.h>
#include <trident/layer0/layer0.h>
#include <stdio.h>

STATE_RESULT sb_start(MACHINESTATE* machinestate) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT sb_start(MACHINESTATE*): entered sender barrier");
    if(BIT0 != machinestate->tx_old) {
        ERROR(ERRNO_BARRIER_FSM, "STATE_RESULT sb_start(MACHINESTATE*): saved machinestate's tx must be 0");
        return STATE_RESULT_ERROR;
    }
    if(BIT0 != machinestate->rx_old) {
        ERROR(ERRNO_BARRIER_FSM, "STATE_RESULT sb_start(MACHINESTATE*): saved machinestate's rx must be 0");
        return STATE_RESULT_ERROR;
    }

    machinestate->next_state_function = &sb0_set_tx_one;
    machinestate->max_wait_tries = 30;
    machinestate->wait_tries = 0;
    return STATE_RESULT_CONTINUE;
}

STATE_RESULT sb_end(MACHINESTATE* machinestate) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT sb_end(MACHINESTATE*): tx = %d, rx = %d, data = %d", machinestate->tx_old, machinestate->rx_old, machinestate->data_old);

    MESSAGE("STATE_RESULT sb_end(MACHINESTATE*): exited sender barrier");
    machinestate->wait_tries = 0;
    return STATE_RESULT_FINISHED;
}


STATE_RESULT sb0_set_tx_one(MACHINESTATE* machinestate) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT sb0_set_tx_one(MACHINESTATE*): entered state");
    BIT txval = BIT1;
    layer0_write_to_pin(machinestate->channel.tx, txval);

    machinestate->tx_old = txval;
    machinestate->next_state_function = &sb1_wait_for_rx_one;
    return STATE_RESULT_CONTINUE;
}

STATE_RESULT sb1_wait_for_rx_one(MACHINESTATE* machinestate) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT sb2_wait_for_rx_one(MACHINESTATE*): entered state");

    BIT rxval = BIT0;
    while(BIT1 != rxval) {
        layer0_read_from_pin(machinestate->channel.rx, &rxval);

        if(BIT0 == rxval) {
            machinestate->rx_old = BIT0;
            if(machinestate->max_wait_tries <= machinestate->wait_tries) { //reset and retry barrier
                MESSAGE("STATE_RESULT sb1_wait_for_rx_one(MACHINESTATE*): detected timeout");
                machinestate->wait_tries = 0;
                machinestate->next_state_function = &sb0_set_tx_one;
                return STATE_RESULT_CONTINUE;
            }
            else {
                (machinestate->wait_tries)++;
            }
        }
        else if(BIT1 != rxval) {
            ERROR(ERRNO_BARRIER_FSM, "STATE_RESULT sb1_wait_for_rx_one(MACHINESTATE*): unexpected rx value");
            return STATE_RESULT_ERROR;
        }
    }

    machinestate->rx_old = BIT1;
    machinestate->wait_tries = 0;
    machinestate->next_state_function = &sb_end;

    return STATE_RESULT_CONTINUE;
}
