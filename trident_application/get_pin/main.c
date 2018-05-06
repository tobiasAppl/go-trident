/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include<stdlib.h>
#include<stdio.h>

#include <trident/layer0/layer0.h>

int main(int argc, char* argv[]) {
    LOG_LAYER(TRIDENT_APPLICATION_LOG);
    if(2 != argc) {
        printf("--usage: set_pin <pin_nr>\n");
        return -1;
    }
    int pin_nr = atoi(argv[1]);

    ERRNO l0_init_success = layer0_initialize();
    if(ERRNO_NO_ERROR != l0_init_success) {
        ERROR(l0_init_success, "int main(int, char*): error while initializing layer 0");
        return l0_init_success;
    }

    int pin_init_success = layer0_initialize_pin(pin_nr, COMM_IO_IN);
    if(0 != pin_init_success) {
        printf("Unable to initialize pin, function returned %d\n", pin_init_success);
        return pin_init_success;
    }

    BIT read_value;
    int set_success = layer0_read_from_pin(pin_nr, &read_value);
    if(0 != set_success) {
        printf("Unable to set pin value, function returned %d\n", set_success);
        return set_success;
    }

    printf("Read value: %d\n", read_value);

    int free_success = layer0_finalize();

    return free_success;
}
