/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef LAYER1_RESULT_TYPES_H
#define LAYER1_RESULT_TYPES_H

#include <trident/util.h>

typedef enum {
    STATE_RESULT_FINISHED = -2,
    STATE_RESULT_ERROR = -1,
    STATE_RESULT_CONTINUE = 0,
    STATE_RESULT_WAIT_TIMEOUT = 1
} STATE_RESULT;

#endif //LAYER1_RESULT_TYPES_H
