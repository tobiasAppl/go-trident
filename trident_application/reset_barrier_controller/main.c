/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <trident/trident.h>

unsigned int num_rrpins;
unsigned int* reset_response_pins;

ERRNO finalize(ERRNO error_code);
void print_usage(char* app_name);

int main(int argc, char* argv[]) {
    LOG_LAYER(TRIDENT_APPLICATION_LOG);
    if(3 > argc) {
        print_usage(argv[0]);
        return finalize(ERRNO_UNDEFINED);
    }
    unsigned int reset_pin_nr = atoi(argv[1]);

    num_rrpins = argc - 2;
    reset_response_pins = (unsigned int*) malloc(num_rrpins * sizeof(unsigned int));
    for(unsigned int i = 0; i < num_rrpins; ++i) {
        reset_response_pins[i] = atoi(argv[2 + i]);
    }

    ERRNO l1_init_succ = trident_initialize();
    if(ERRNO_NO_ERROR != l1_init_succ) {
        ERROR(l1_init_succ, "int main(int, char*): error during layer1 initialization");
        return finalize(l1_init_succ);
    }

    ERRNO reset_result = trident_reset_barrier_controller(reset_pin_nr, reset_response_pins, num_rrpins);
    if(ERRNO_NO_ERROR != reset_result) {
        ERROR(reset_result, "int main(int, char*): error during execution of reset barrier controller");
        return finalize(reset_result);
    }

    return finalize(ERRNO_NO_ERROR);
}

ERRNO finalize(ERRNO error_code) {
    if(NULL != reset_response_pins) {
        free(reset_response_pins);
    }
    ERRNO fin_ret = trident_finalize();
    if(ERRNO_NO_ERROR != error_code) {
        fin_ret = error_code;
    }
    return fin_ret;
}

void print_usage(char* app_name) {
    fprintf(stderr, "--usage: %s <reset pin nr> <reset response pin 0> [<reset response pin 1> ... <reset response pin n>]\n", app_name);
}
