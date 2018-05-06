/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include <trident/layer0/error.h>
#include <stdio.h>
#include <stdlib.h>

void error(int trident_log_layer, ERRNO errno, const char* msg_format, ...) {
    int dolog = 0;
    switch(trident_log_layer) {
    case TRIDENT_APPLICATION_LOG:
#ifdef TRIDENT_LOGGING_APPLICATION_ENABLED
        dolog = 1;
#endif
        break;
    case TRIDENT_LAYER0_LOG:
#ifdef TRIDENT_LOGGING_LAYER0_ENABLED
        dolog = 1;
#endif
        break;
    case TRIDENT_LAYER1_LOG:
#ifdef TRIDENT_LOGGING_LAYER1_ENABLED
        dolog = 1;
#endif
        break;
    case TRIDENT_LAYER2_LOG:
#ifdef TRIDENT_LOGGING_LAYER2_ENABLED
        dolog = 1;
#endif
        break;
    }

    if(dolog) {
        va_list args;
        va_start(args, msg_format);
        fprintf(stderr, "ERROR %d: ", errno);
        vfprintf(stderr, msg_format, args);
        fprintf(stderr, "\n");
        va_end(args);
    }
}
