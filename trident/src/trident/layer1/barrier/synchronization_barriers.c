/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include "synchronization_barriers.h"

#include "receiver_barrier.h"
#include "sender_barrier.h"

ERRNO sender_barrier_synchronize(MACHINESTATE *machinestate) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    machinestate->next_state_function = &sb_start;
    STATE_RESULT stateresult = STATE_RESULT_CONTINUE;

    while(STATE_RESULT_CONTINUE == stateresult) {
        stateresult = machinestate->next_state_function(machinestate);
    }

    if(STATE_RESULT_FINISHED != stateresult) {
        if(STATE_RESULT_ERROR == stateresult) {
            ERROR(STATE_RESULT_ERROR, "int sender_barrier_synchronize(MACHINESTATE): an error occured during the send barrier fsm");
            return ERRNO_BARRIER_FSM;
        }
        else {
            ERROR(stateresult, "int sender_barrier_synchronize(MACHINESTATE): Illegal return value from sender barrier fsm");
            return ERRNO_BARRIER_FSM;
        }
    }

    return ERRNO_NO_ERROR;
}

ERRNO receiver_barrier_synchronize(MACHINESTATE *machinestate) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    machinestate->next_state_function = &rb_start;
    STATE_RESULT stateresult = STATE_RESULT_CONTINUE;

    while(STATE_RESULT_CONTINUE == stateresult) {
        stateresult = machinestate->next_state_function(machinestate);
    }

    if(STATE_RESULT_FINISHED != stateresult) {
        if(STATE_RESULT_ERROR == stateresult) {
            ERROR(stateresult, "ERRNO receiver_barrier_synchronize(MACHINESTATE): an error occured during the receiver barrier fsm");
            return ERRNO_BARRIER_FSM;
        }
        else {
            ERROR(stateresult, "ERRNO receiver_barrier_synchronize(MACHINESTATE): Illegal return value from receiver barrier fsm");
            return ERRNO_BARRIER_FSM;
        }
    }
    return ERRNO_NO_ERROR;
}
