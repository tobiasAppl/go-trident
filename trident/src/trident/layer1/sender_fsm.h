/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef SENDER_FSM_H
#define SENDER_FSM_H

#include "layer1_types.h"

STATE_RESULT s_start(MACHINESTATE* machine_state);

STATE_RESULT s_stop(MACHINESTATE* machine_state);

STATE_RESULT s0_write_data(MACHINESTATE* machine_state);

STATE_RESULT s1_toggle_tx(MACHINESTATE* machine_state);

STATE_RESULT s2_wait_for_rx_toggle(MACHINESTATE* machine_state);

#endif // SENDER_FSM_H
