/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef LOG_H
#define LOG_H

#include <trident_config.h>
#include "errno.h"

typedef enum {
    TRIDENT_APPLICATION_LOG = -1,
    TRIDENT_LAYER0_LOG = 0,
    TRIDENT_LAYER1_LOG = 1,
    TRIDENT_LAYER2_LOG = 2
} TRIDENT_LOG_LAYER_NUMBER;


#if defined(TRIDENT_LOGGING_INFO) || defined(TRIDENT_LOGGING_ERROR)
    #define LOG_LAYER(layer_nr) TRIDENT_LOG_LAYER_NUMBER __INTERNAL_TRIDENT_LOG_LAYER_NUMBER = layer_nr
#else
    #define LOG_LAYER(layer_nr)
#endif

#ifdef TRIDENT_LOGGING_INFO
    #include "layer0/message.h"
    #define MESSAGE(...) message(__INTERNAL_TRIDENT_LOG_LAYER_NUMBER, __VA_ARGS__)
#else
    #define MESSAGE(...)
#endif

#ifdef TRIDENT_LOGGING_ERROR
    #include "layer0/error.h"
    #define ERROR(...) error(__INTERNAL_TRIDENT_LOG_LAYER_NUMBER, __VA_ARGS__)
#else
    #define ERROR(...)
#endif

#endif // LOG_H

