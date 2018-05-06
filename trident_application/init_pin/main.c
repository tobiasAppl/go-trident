/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include <trident/layer0/layer0.h>

int main(int argc, char* argv[]) {
    if(3 != argc) {
        printf("--usage: init_pin <pin_nr> <i|o>\n");
        return -1;
    }
    int pin_nr = atoi(argv[1]);
    char* value = strdup(argv[2]);

    int vallen = strlen(value);
    if(1 != vallen) {
        printf("input \"%s\" invalid, only \'i\' or \'o\' allowed\n", value);
        return -1;
    }
    char direction = value[0];
    if('i' != direction && 'o' != direction) {
        printf("input \"%s\" invalid, only \'i\' or \'o\' allowed\n", value);
        return -1;
    }
    int init_success = layer0_initialize_pin(pin_nr,( 'i' == direction ? COMM_IO_IN : COMM_IO_OUT));
    if(0 != init_success) {
        printf("Unable to initialize pin, function returned %d\n", init_success);
        return init_success;
    }
    printf("Initialized pin %d for %s\n", pin_nr, ('i' == direction ? "input" : "output" ));
    int free_success = layer0_finalize();

    return free_success;
}
