/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include "receiver_barrier.h"
#include <trident/util.h>
#include <trident/layer0/layer0.h>
#include <stdio.h>

STATE_RESULT rb_start(MACHINESTATE* machinestate) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT rb_start(MACHINESTATE*): entered receiver barrier");
    if(BIT0 != machinestate->tx_old) {
        ERROR(ERRNO_BARRIER_FSM, "STATE_RESULT rb_start(MACHINESTATE*): saved machinestate's tx must be 0");
        return STATE_RESULT_ERROR;
    }
    if(BIT0 != machinestate->rx_old) {
        ERROR(ERRNO_BARRIER_FSM, "STATE_RESULT rb_start(MACHINESTATE*): saved machinestate's rx must be 0");
        return STATE_RESULT_ERROR;
    }
    machinestate->next_state_function = &rb0_wait_for_tx_one;
    machinestate->wait_tries = 0;
    return STATE_RESULT_CONTINUE;
}

STATE_RESULT rb_end(MACHINESTATE* machinestate) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT rb_end(MACHINESTATE*): tx = %d, rx = %d, data = %d", machinestate->tx_old, machinestate->rx_old, machinestate->data_old);
    MESSAGE("STATE_RESULT rb_end(MACHINESTATE*): exited receiver barrier");
    machinestate->wait_tries = 0;
    return STATE_RESULT_FINISHED;
}

STATE_RESULT rb0_wait_for_tx_one(MACHINESTATE* machinestate) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT rb0_wait_for_tx_one(MACHINESTATE*): entered state");
    BIT txval = BIT0;
    while(BIT1 != txval) {
        layer0_read_from_pin(machinestate->channel.tx, &txval);
        if(BIT1 != txval && BIT0 != txval ) {
            ERROR(ERRNO_BARRIER_FSM, "STATE_RESULT rb0_wait_for_tx_one(MACHINESTATE*): unexpected tx value");
            return STATE_RESULT_ERROR;
        }
    }
    machinestate->tx_old = BIT1;
    machinestate->next_state_function = &rb1_set_rx_one;

    return STATE_RESULT_CONTINUE;
}

STATE_RESULT rb1_set_rx_one(MACHINESTATE* machinestate) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT rb1_set_rx_one(MACHINESTATE*): entered state");
    BIT rxval = (BIT0 == machinestate->rx_old ? BIT1 : BIT0);
    layer0_write_to_pin(machinestate->channel.rx, rxval);

    machinestate->rx_old = rxval;
    machinestate->next_state_function = &rb_end;
    return STATE_RESULT_CONTINUE;
}

