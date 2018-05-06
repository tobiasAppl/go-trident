/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "layer1_result_types.h"

#include <trident/layer0/layer0_types.h>

/**
 * @brief The CHANNEL_DESCRIPTOR struct
 * Structure representation of a channel, needed for channel initialization
 */
struct CHANNEL_DESCRIPTOR {
  int channel_nr;       // sequentialized channel number, set by layer1

  int tx;               // tx pin number
  int rx;               // rx pin number
  int data;	            // data pin number
  COMM_IO direction;    // in|out
  unsigned int bit_width; // channel bit width
};
typedef struct CHANNEL_DESCRIPTOR CHANNEL_DESCRIPTOR;


struct MACHINESTATE {
    CHANNEL_DESCRIPTOR channel;
    BIT rx_old;
    BIT tx_old;
    BIT data_old;
    STATE_RESULT (*next_state_function)(struct MACHINESTATE *);

    BIT* current_word;
    unsigned int current_word_idx;

    unsigned int wait_tries;
    unsigned int max_wait_tries;
};
typedef struct MACHINESTATE MACHINESTATE;

#endif // STATEMACHINE_H
