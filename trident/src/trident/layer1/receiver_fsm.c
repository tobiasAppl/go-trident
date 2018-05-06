/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include "receiver_fsm.h"
#include <trident/layer0/layer0.h>
#include <trident/util.h>
#include <stdlib.h>
#include <stdio.h>

STATE_RESULT r0_wait_for_tx_toggle(MACHINESTATE* machine_state){
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT r0_wait_for_tx_toggle(MACHINESTATE*): invoked");
    machine_state->wait_tries = 0;
    while(0 < machine_state->max_wait_tries ? (machine_state->wait_tries < machine_state->max_wait_tries) : 1 ) {
        BIT tx_val = machine_state->tx_old;
        int read_success = layer0_read_from_pin(machine_state->channel.tx, &tx_val);
        if(ERRNO_NO_ERROR != read_success) {
            ERROR(read_success, "STATE_RESULT r0_wait_for_tx_toggle(MACHINESTATE*): layer0_read_from_pin failed");
            return STATE_RESULT_ERROR;
        }
        if(tx_val != machine_state->tx_old) {
            MESSAGE("STATE_RESULT r0_wait_for_tx_toggle(MACHINESTATE*): tx toggled");
            machine_state->tx_old = tx_val;
            machine_state->next_state_function = &r1_read_data;
            return STATE_RESULT_CONTINUE;
        }
        ++(machine_state->wait_tries);
    }
    return STATE_RESULT_WAIT_TIMEOUT;
}

STATE_RESULT r1_read_data(MACHINESTATE* machine_state){
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT r1_read_data(MACHINESTATE*): bwidth=%d; idx=%d",machine_state->channel.bit_width, machine_state->current_word_idx);
    BIT data;
    int read_success = layer0_read_from_pin(machine_state->channel.data, &data);
    if(ERRNO_NO_ERROR != read_success) {
        ERROR(read_success, "STATE_RESULT r1_read_data(MACHINESTATE*): layer0_read_from_pin failed");
        return STATE_RESULT_ERROR;
    }
    machine_state->data_old = data;
    machine_state->current_word[machine_state->current_word_idx++] = data;
    machine_state->next_state_function = &r2_toggle_rx;
    return STATE_RESULT_CONTINUE;
}

STATE_RESULT r2_toggle_rx(MACHINESTATE* machine_state){
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT r2_toggle_rx(MACHINESTATE*): Entered state");
    BIT rx_toggled = BIT0 == machine_state->rx_old ? BIT1 : BIT0;
    int write_success = layer0_write_to_pin(machine_state->channel.rx, rx_toggled);
    if(write_success) {
        MESSAGE("STATE_RESULT r2_toggle_rx(MACHINESTATE*): toggling write to pin not successful");
        return STATE_RESULT_ERROR;
    }
    machine_state->rx_old = rx_toggled;
    machine_state->next_state_function = &r0_wait_for_tx_toggle;
    if(machine_state->current_word_idx >= machine_state->channel.bit_width) {
        machine_state->next_state_function = &r_stop;
    }
    return STATE_RESULT_CONTINUE;
}

STATE_RESULT r_start(MACHINESTATE* machine_state)
{
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT r_start(MACHINESTATE*): Entered state");
    //set up machine state for receiving a word
    machine_state->current_word_idx = 0;
    machine_state->next_state_function = &r0_wait_for_tx_toggle;

    return STATE_RESULT_CONTINUE;
}


STATE_RESULT r_stop(MACHINESTATE* machine_state)
{
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    MESSAGE("STATE_RESULT r_stop(MACHINESTATE*): Entered state, tx = %d, rx = %d", machine_state->tx_old, machine_state->rx_old);
    //check if transmission was correct
    if(machine_state->current_word_idx != machine_state->channel.bit_width) {
        ERROR(ERRNO_TRANSMISSION_BIT_WIDTH, "STATE_RESULT r_stop(MACHINESTATE*): received bits not equal to channel width - ch width : %d - received bits : %d", machine_state->channel.bit_width, machine_state->current_word_idx);
        return STATE_RESULT_ERROR;
    }
    //check if transmission ended with writing 0 to rx
    if(BIT0 != machine_state->rx_old) {
        ERROR(ERRNO_TRANSMISSION_TERMINATION_RX, "STATE_RESULT r_stop(MACHINESTATE*): transmission termination did not reset rx to 0");
        return STATE_RESULT_ERROR;
    }
    machine_state->current_word_idx = 0;
    machine_state->next_state_function = &r_start;
    return STATE_RESULT_FINISHED;
}

