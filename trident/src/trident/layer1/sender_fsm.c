/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include "sender_fsm.h"
#include <trident/layer0/layer0.h>
#include <trident/util.h>
#include <stdlib.h>
#include <stdio.h>

STATE_RESULT s0_write_data(MACHINESTATE* machine_state) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT s0_write_data(MACHINESTATE*): bwidth=%d; idx=%d",machine_state->channel.bit_width, machine_state->current_word_idx);
    BIT new_data_bit = machine_state->current_word[machine_state->current_word_idx++];
    ERRNO write_failed = layer0_write_to_pin(machine_state->channel.data, new_data_bit);
    if(ERRNO_NO_ERROR != write_failed) {
        MESSAGE("STATE_RESULT s0_write_data(MACHINESTATE*): writing data to pin was unsuccessful");
        return STATE_RESULT_ERROR;
    }
    machine_state->data_old = new_data_bit;
    machine_state->next_state_function = &s1_toggle_tx;
    return STATE_RESULT_CONTINUE;
}

STATE_RESULT s1_toggle_tx(MACHINESTATE* machine_state) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT s1_toggle_tx(MACHINESTATE*): Entered state");
    BIT tx_toggled = BIT0 == machine_state->tx_old ? BIT1 : BIT0;
    ERRNO write_failed = layer0_write_to_pin(machine_state->channel.tx, tx_toggled);
    if(ERRNO_NO_ERROR != write_failed) {
        MESSAGE("STATE_RESULT s1_toggle_tx(MACHINESTATE*): writing to tx pin was unsuccessful");
        return STATE_RESULT_ERROR;
    } else {
        machine_state->tx_old = tx_toggled;
    }
    machine_state->next_state_function = &s2_wait_for_rx_toggle;
    return STATE_RESULT_CONTINUE;
}

STATE_RESULT s2_wait_for_rx_toggle(MACHINESTATE* machine_state) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT s2_wait_for_rx_toggle(MACHINESTATE*): Entered state");
    machine_state->wait_tries = 0;
    while(0 < machine_state->max_wait_tries ? machine_state->wait_tries < machine_state->max_wait_tries : 1 ) {

        BIT rx_value = machine_state->rx_old;
        ERRNO read_success = layer0_read_from_pin(machine_state->channel.rx, &rx_value);
        if(ERRNO_NO_ERROR != read_success) {
            ERROR(read_success, "s2_wait_for_rx_toggle: layer0_read_from_pin failed");
            return STATE_RESULT_ERROR;
        }
        if(rx_value !=  machine_state->rx_old) {
            MESSAGE("STATE_RESULT s2_wait_for_rx_toggle(MACHINESTATE*): rx toggled");
            machine_state->rx_old = rx_value;
            machine_state->next_state_function = &s0_write_data;
            if(machine_state->current_word_idx >= machine_state->channel.bit_width) {
                machine_state->next_state_function = &s_stop;
            }
            return STATE_RESULT_CONTINUE;
        }
        ++(machine_state->wait_tries);
    }
    return STATE_RESULT_WAIT_TIMEOUT;
}


STATE_RESULT s_start(MACHINESTATE* machine_state) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT s_start(MACHINESTATE*): Entered state");
    //set up machine state for word transmission
    machine_state->current_word_idx = 0;
    machine_state->next_state_function = &s0_write_data;
    return STATE_RESULT_CONTINUE;
}


STATE_RESULT s_stop(MACHINESTATE* machine_state) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT s_stop(MACHINESTATE*): Entered state, tx = %d, rx = %d", machine_state->tx_old, machine_state->rx_old);
    //Check if trasmission was complete
    if(machine_state->current_word_idx != machine_state->channel.bit_width) {
        ERROR(ERRNO_TRANSMISSION_BIT_WIDTH, "STATE_RESULT s_stop(MACHINESTATE*): incorrect transmission bit width - ch width: %d - send word width ", machine_state->channel.bit_width, machine_state->current_word_idx);
        return STATE_RESULT_ERROR;
    }
    //Check transmission termination criteria
    if(BIT0 != machine_state->tx_old) {
        ERROR(ERRNO_TRANSMISSION_TERMINATION_TX, "STATE_RESULT s_stop(MACHINESTATE*): tranmission termination criteria tx");
        return STATE_RESULT_ERROR;
    }

    machine_state->next_state_function = &s_start;
    return STATE_RESULT_FINISHED;
}
