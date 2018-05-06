/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef RECEIVER_FSM_H
#define RECEIVER_FSM_H

#include "layer1_types.h"

/**
 * @brief r_start is an explicit start state for the receiver state machine
 * This state explicitly starts the sending of a word and does some checkups
 * @param machine_state
 * @return
 */
STATE_RESULT r_start(MACHINESTATE* machine_state);

/**
 * @brief r_stop is an explicit stop state for the receiver state machine
 * This state should be reached once a full word has been received
 * @param machine_state
 * @return
 */
STATE_RESULT r_stop(MACHINESTATE* machine_state);

STATE_RESULT r0_wait_for_tx_toggle(MACHINESTATE* machine_state);

STATE_RESULT r1_read_data(MACHINESTATE* machine_state);

STATE_RESULT r2_toggle_rx(MACHINESTATE* machine_state);


#endif // RECEIVER_FSM_H
