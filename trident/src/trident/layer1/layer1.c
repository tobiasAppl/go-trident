/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include "layer1.h"
#include "sender_fsm.h"
#include "receiver_fsm.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <trident_config.h>
#include <trident/util.h>
#include <trident/layer0/layer0.h>
#include <trident/layer1/barrier/synchronization_barriers.h>

#define MAX_CHANNELS TRIDENT_PIN_COUNT / 3

/**
 * @brief layer1_channels Channel managament array
 * This array holds the machinestate descriptors of all opened channels.
 * The layer1_channels_top_idx integer always refers to the last opened channel in this array
 */
MACHINESTATE __layer1_channels[MAX_CHANNELS];

/**
 * @brief layer1_channels_top_idx Index of the last aggregated channel
 * This integer is the index of the last opened channel in the layer1_channels array
 * or, -1 if no channels have previously been openend.
 */
int __layer1_channels_top_idx = -1;

ERRNO layer1_initialize() {
    int i = 0;
    for(; MAX_CHANNELS > i; ++i) {
        __layer1_channels[i].channel.direction = COMM_IO_UNINITIALIZED;
    }
    layer1_reset_channel_iterator();
    return layer0_initialize();
}

ERRNO layer1_aggregate_channel(CHANNEL_DESCRIPTOR* channel_descriptor) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    if(MAX_CHANNELS <= ++__layer1_channels_top_idx) {
        ERROR(ERRNO_MAX_CHANNELS_REACHED, "layer1_aggregate_channel: Maximal number of channels reached");
        return ERRNO_MAX_CHANNELS_REACHED;
    }
    channel_descriptor->channel_nr = __layer1_channels_top_idx;

    __layer1_channels[__layer1_channels_top_idx].channel = *channel_descriptor;
    __layer1_channels[__layer1_channels_top_idx].max_wait_tries = 0;
    __layer1_channels[__layer1_channels_top_idx].wait_tries = 0;

    __layer1_channels[__layer1_channels_top_idx].current_word_idx = 0;

    MACHINESTATE* channelstate = &__layer1_channels[__layer1_channels_top_idx];

    if(COMM_IO_OUT == channel_descriptor->direction) {
        //initialize pins
        layer0_initialize_pin(channelstate->channel.tx, COMM_IO_OUT);
        layer0_initialize_pin(channelstate->channel.data, COMM_IO_OUT);
        layer0_initialize_pin(channelstate->channel.rx, COMM_IO_IN);

        //set sender initial state
        channelstate->next_state_function = &s_start;
    }
    else if(COMM_IO_IN == channel_descriptor->direction) {
        //initialize pins
        layer0_initialize_pin(channelstate->channel.tx, COMM_IO_IN);
        layer0_initialize_pin(channelstate->channel.data, COMM_IO_IN);
        layer0_initialize_pin(channelstate->channel.rx, COMM_IO_OUT);

        //set receiver initial state
        channelstate->next_state_function = &r_start;
    }
    else {
        ERROR(ERRNO_CHANNEL_IO_UNDEFINED, "LAYER1_CHANNEL_IO_RESULT layer1_aggregate_channel(CHANNEL_DESCRIPTOR, int*): channel direction invalid");
        return ERRNO_CHANNEL_IO_UNDEFINED;
    }

    return ERRNO_NO_ERROR;
}

ERRNO layer1_write_to_channel(unsigned int channel_no, BIT* data_word, unsigned int max_wait_tries) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    if(MAX_CHANNELS <= channel_no) {
        ERROR(ERRNO_CHANNEL_IDX, "LAYER1_CHANNEL_IO_RESULT layer1_write_to_channel(unsigned int, BIT*, unsigned int): Channel number invalid");
        return ERRNO_CHANNEL_IDX;
    }
    if(__layer1_channels_top_idx < (int)channel_no) {
        ERROR(ERRNO_CHANNEL_IDX, "LAYER1_CHANNEL_IO_RESULT layer1_write_to_channel(unsigned int, BIT*, unsigned int): channel not open");
        return ERRNO_CHANNEL_IDX;
    }

    MACHINESTATE* channel_state = &__layer1_channels[channel_no];
    if(COMM_IO_OUT != channel_state->channel.direction) {
        ERROR(ERRNO_CHANNEL_DIRECTION, "LAYER1_CHANNEL_IO_RESULT layer1_write_to_channel(unsigned int, BIT*, unsigned int): Channel is not an output channel");
        return ERRNO_CHANNEL_DIRECTION;
    }
    if(&s_start != channel_state->next_state_function) {
        ERROR(ERRNO_FSM_STATE, "LAYER1_CHANNEL_IO_RESULT layer1_write_to_channel(unsigned int, BIT*, unsigned int): sender fsm is in the wrong state");
        return ERRNO_FSM_STATE;
    }

    //enter synchronization barrier
    ERRNO send_bar_errlvl = sender_barrier_synchronize(channel_state);
    if(ERRNO_NO_ERROR != send_bar_errlvl) {
        return send_bar_errlvl;
    }

    channel_state->next_state_function = &s_start;

    channel_state->max_wait_tries = max_wait_tries;
    channel_state->wait_tries = 0;


    // extend data word to an odd number of bits so an rx = tx = 0 final state can be reached
    int width_extended = 0;
    BIT e_dw[channel_state->channel.bit_width + 1];
    if(0 == (channel_state->channel.bit_width % 2)) {
        width_extended = 1;
        channel_state->channel.bit_width++;
        int e_idx = channel_state->channel.bit_width - 1;
        for(int i = 0; i < e_idx; ++i) {
            e_dw[i] = data_word[i];
        }
        e_dw[e_idx] = 0;
        channel_state->current_word = e_dw;
    }
    else if(data_word != channel_state->current_word) {
        channel_state->current_word = data_word;
    }


    // Send message
    STATE_RESULT state_result = STATE_RESULT_CONTINUE;
    while(STATE_RESULT_CONTINUE == state_result) {
        state_result = channel_state->next_state_function(channel_state);
    }

    // reset channel extension
    if(1 == width_extended) {
        channel_state->channel.bit_width--;
        channel_state->current_word = data_word;
    }

    if(STATE_RESULT_ERROR == state_result) {
        ERROR(ERRNO_OUT_TRANSMISSION, "LAYER1_CHANNEL_IO_RESULT layer1_write_to_channel(unsigned int, BIT*, unsigned int): Sender state machine resulted in an error");
        return ERRNO_OUT_TRANSMISSION;
    } else if(STATE_RESULT_WAIT_TIMEOUT == state_result) {
        MESSAGE("LAYER1_CHANNEL_IO_RESULT layer1_write_to_channel(unsigned int, BIT*, unsigned int): Timeout while sending a word");
        return ERRNO_OUT_TRANSMISSION;
    } else if(STATE_RESULT_FINISHED == state_result) {
        channel_state->current_word_idx = 0;
        MESSAGE("LAYER1_CHANNEL_IO_RESULT layer1_write_to_channel(unsigned int, BIT*, unsigned int): Finished sending a word");
        return ERRNO_NO_ERROR;
    } else { //Error state since enums are simple integers unrestricted of defined enum values
        ERROR(ERRNO_SENDER_RETURN_VALUE, "LAYER1_CHANNEL_IO_RESULT layer1_write_to_channel(unsigned int, BIT*, unsigned int): Sender state machine returned an invalid result");
        return ERRNO_OUT_TRANSMISSION;
    }
}

ERRNO layer1_read_from_channel(unsigned int channel_no, BIT* read_buffer, unsigned int max_wait_tries) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    if(MAX_CHANNELS <= channel_no) {
        ERROR(ERRNO_CHANNEL_IDX, "LAYER1_CHANNEL_IO_RESULT layer1_read_from_channel(unsigned int, BIT*, unsigned int): Channel number invalid");
        return ERRNO_CHANNEL_IDX;
    }
    if(__layer1_channels_top_idx < (int)channel_no) {
        ERROR(ERRNO_CHANNEL_IDX, "LAYER1_CHANNEL_IO_RESULT layer1_read_from_channel(unsigned int, BIT*, unsigned int): channel not open");
        return ERRNO_CHANNEL_IDX;
    }

    MACHINESTATE* channel_state = &__layer1_channels[channel_no];
    if(COMM_IO_IN != channel_state->channel.direction) {
        ERROR(ERRNO_CHANNEL_DIRECTION, "LAYER1_CHANNEL_IO_RESULT layer1_read_from_channel(unsigned int, BIT*, unsigned int): Channel is not an input channel");
        return ERRNO_CHANNEL_DIRECTION;
    }
    if(&r_start != channel_state->next_state_function) {
        ERROR(ERRNO_FSM_STATE, "LAYER1_CHANNEL_IO_RESULT layer1_read_from_channel(unsigned int, BIT*, unsigned int): receiver fsm is in the wrong state");
        return ERRNO_FSM_STATE;
    }

    //allocate rx and tx values via synchronization barrier
    ERRNO receiver_bar_errlvl = receiver_barrier_synchronize(channel_state);
    if(ERRNO_NO_ERROR != receiver_bar_errlvl)  {
        return receiver_bar_errlvl;
    }
    channel_state->next_state_function = &r_start;

    channel_state->max_wait_tries = max_wait_tries;
    channel_state->wait_tries = 0;
    channel_state->current_word_idx = 0;

    // extend read buffer to read an odd number of bits
    int width_extended = 0;
    BIT* e_rb = (BIT*) malloc((channel_state->channel.bit_width + 1) * sizeof(BIT));
    if(0 == (channel_state->channel.bit_width % 2)) {
        width_extended = 1;
        channel_state->current_word = e_rb;
        channel_state->channel.bit_width++;
    }
    else if(read_buffer != channel_state->current_word) {
        channel_state->current_word = read_buffer;
    }

    // read word
    STATE_RESULT state_result = STATE_RESULT_CONTINUE;
    while(STATE_RESULT_CONTINUE == state_result) {
        state_result = channel_state->next_state_function(channel_state);
    }

    //copy from the extended buffer to the return buffer
    if(1 == width_extended) {
        channel_state->channel.bit_width--;
        for(unsigned int i = 0; i < channel_state->channel.bit_width;++i) {
            read_buffer[i] = e_rb[i];
        }
        channel_state->current_word = read_buffer;
    }
    free(e_rb);

    if(STATE_RESULT_ERROR == state_result) {
        ERROR(ERRNO_IN_TRANSMISSION, "LAYER1_CHANNEL_IO_RESULT layer1_read_from_channel(unsigned int, BIT*, unsigned int): Receiver state machine resulted in an error");
        return ERRNO_IN_TRANSMISSION;
    } else if (STATE_RESULT_FINISHED == state_result) {
        MESSAGE("LAYER1_CHANNEL_IO_RESULT layer1_read_from_channel(unsigned int, BIT*, unsigned int): Finished receiving a valid word");
        return ERRNO_NO_ERROR;
    } else if(STATE_RESULT_WAIT_TIMEOUT == state_result) {
        MESSAGE("LAYER1_CHANNEL_IO_RESULT layer1_read_from_channel(unsigned int, BIT*, unsigned int)Timeout while receiving a word");
        return ERRNO_NO_ERROR;
    } else { //Error state since enums are simple integers unrestricted to defined enum values
        ERROR(ERRNO_SENDER_RETURN_VALUE, "LAYER1_CHANNEL_IO_RESULT layer1_read_from_channel(unsigned int, BIT*, unsigned int): Receiver state machine returned an invalid result");
        return ERRNO_IN_TRANSMISSION;
    }
}

ERRNO layer1_set_channel_bit_width(unsigned int channel_number, unsigned int bit_width) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    if(__layer1_channels_top_idx < (int)channel_number || COMM_IO_UNINITIALIZED == __layer1_channels[channel_number].channel.direction) {
        ERROR(ERRNO_CHANNEL_NOT_INITIALIZED, "ERRNO layer1_set_channel_bit_width(unsigned int): requested channel %d has not yet been initialized", channel_number);
        return ERRNO_CHANNEL_NOT_INITIALIZED;
    }
    __layer1_channels[channel_number].channel.bit_width = bit_width;

    return ERRNO_NO_ERROR;
}


ERRNO layer1_get_channel_bit_width(unsigned int channel_number, unsigned int *bit_with_buffer) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    if(__layer1_channels_top_idx < (int)channel_number || COMM_IO_UNINITIALIZED == __layer1_channels[channel_number].channel.direction) {
        ERROR(ERRNO_CHANNEL_NOT_INITIALIZED, "layer1_get_channel_bit_width(unsigned int, unsigned int *): requested channel %d has not yet been initialized", channel_number);
        return ERRNO_CHANNEL_NOT_INITIALIZED;
    }

    *bit_with_buffer = __layer1_channels[channel_number].channel.bit_width;

    return ERRNO_NO_ERROR;
}


ERRNO layer1_reset_channel_pin_states() {
    LOG_LAYER(TRIDENT_LAYER1_LOG);

    for(int ch_idx = 0; __layer1_channels_top_idx >= ch_idx; ++ch_idx) {
        MACHINESTATE* ch_state = &(__layer1_channels[ch_idx]);
        //set channel output pins to 0
        switch(ch_state->channel.direction) {
        case COMM_IO_IN: {
                ERRNO rx_reset_succ = layer0_write_to_pin(ch_state->channel.rx, BIT0);
                if(ERRNO_NO_ERROR != rx_reset_succ) {
                    ERROR(rx_reset_succ, "LAYER1_CHANNEL_IO_RESULT layer1_reset_channel(unsigned int): error during rx reset ch#%d", ch_idx);
                    return rx_reset_succ;
                }
                ch_state->rx_old = BIT0;
                break;
            }
        case COMM_IO_OUT: {
                ERRNO tx_reset_succ = layer0_write_to_pin(ch_state->channel.tx, BIT0);
                if(ERRNO_NO_ERROR != tx_reset_succ) {
                    ERROR(tx_reset_succ, "LAYER1_CHANNEL_IO_RESULT layer1_reset_channel(unsigned int): error during tx reset ch#%d", ch_idx);
                    return tx_reset_succ;
                }
                ch_state->tx_old = BIT0;
                break;
            }
        default: {
                ERROR(ERRNO_CHANNEL_DIRECTION, "LAYER1_CHANNEL_IO_RESULT layer1_reset_channel(unsigned int): channel direction corrupt ch#%d", ch_idx);
                return ERRNO_CHANNEL_DIRECTION;
            }
        }
    }

    return ERRNO_NO_ERROR;
}

ERRNO layer1_get_channel_descriptor(unsigned int channel_no, CHANNEL_DESCRIPTOR* channel_descriptor_buffer) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    if(__layer1_channels_top_idx < (int)channel_no) {
        ERROR(ERRNO_CHANNEL_IDX, "ERRNO layer1_get_channel_descriptor(unsigned int, CHANNEL_DESCRIPTOR*): channel not open");
        return ERRNO_CHANNEL_IDX;
    }

    *channel_descriptor_buffer = __layer1_channels[channel_no].channel;

    return ERRNO_NO_ERROR;
}

ERRNO layer1_get_channel_descriptor_reference(unsigned int channel_no, CHANNEL_DESCRIPTOR** channel_descriptor_reference_buffer) {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    if(__layer1_channels_top_idx < (int)channel_no) {
        ERROR(ERRNO_CHANNEL_IDX, "ERRNO layer1_get_channel_descriptor(unsigned int, CHANNEL_DESCRIPTOR*): channel %d not open", channel_no);
        return ERRNO_CHANNEL_IDX;
    }

    *channel_descriptor_reference_buffer = &(__layer1_channels[channel_no].channel);
    return ERRNO_NO_ERROR;
}


unsigned int layer1_get_channel_count()
{
    return 0 > __layer1_channels_top_idx ? 0 : __layer1_channels_top_idx+1;
}

/*** Iterator style CHANNEL_DESCRIPTOR access ***/

int  __layer1_last_iterator_returned_channel_descriptor = -1;

void layer1_reset_channel_iterator() {
    __layer1_last_iterator_returned_channel_descriptor = -1;
}

CHANNEL_DESCRIPTOR* layer1_get_next_channel_descriptor() {
    __layer1_last_iterator_returned_channel_descriptor += 1;
    for(;MAX_CHANNELS > __layer1_last_iterator_returned_channel_descriptor; ++__layer1_last_iterator_returned_channel_descriptor) {
        if(COMM_IO_IN == __layer1_channels[__layer1_last_iterator_returned_channel_descriptor].channel.direction ||
           COMM_IO_OUT == __layer1_channels[__layer1_last_iterator_returned_channel_descriptor].channel.direction) {
            return &(__layer1_channels[__layer1_last_iterator_returned_channel_descriptor].channel);
        }
    }
    return NULL;
}

/*** FINALIZATION FUNCTIONS ***/

void finalize_in_channel(int channel_idx) {
    (void) channel_idx;
    //NoOp
}

void finalize_out_channel(int channel_idx) {
    (void) channel_idx;
    //NoOp
}

ERRNO layer1_finalize() {
    LOG_LAYER(TRIDENT_LAYER1_LOG);
    int ch_idx;
    for(ch_idx = 0; __layer1_channels_top_idx >= ch_idx; ++ch_idx) {
        MACHINESTATE* chstate = &__layer1_channels[ch_idx];
        if(COMM_IO_OUT == chstate->channel.direction) {
            finalize_out_channel(ch_idx);
        }
        else if (COMM_IO_IN == chstate->channel.direction) {
            finalize_in_channel(ch_idx);
        }
        else {
            ERROR(ERRNO_CHANNEL_DIRECTION, "LAYER1_CHANNEL_IO_RESULT layer1_finalize(): expected in or output channel %d", ch_idx);
        }
        chstate->channel.direction = COMM_IO_UNINITIALIZED;
    }
    __layer1_channels_top_idx = -1;
    layer1_reset_channel_iterator();

    return layer0_finalize();
}
