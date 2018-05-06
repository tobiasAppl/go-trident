/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include<stdlib.h>
#include<stdio.h>

#include <trident/layer0/layer0.h>

int main(int argc, char* argv[]) {
    if(2 != argc) {
        printf("--usage: get_pin_cont <pin_nr>\n");
        return -1;
    }
    int pin_nr = atoi(argv[1]);
    printf("bla\n");

    int init_success = layer0_initialize_pin(pin_nr, COMM_IO_IN);
    if(0 != init_success) {
        printf("Unable to initialize pin, function returned %d\n", init_success);
        return init_success;
    }

    while(1) {
        BIT read_value = -1;
        printf("Reading value from pin %d\n", pin_nr);
        int read_success = layer0_read_from_pin(pin_nr, &read_value);
        if(0 != read_success) {
            printf("Unable to set pin value, function returned %d\n", read_success);
            return read_success;
        }
        if(-1 == read_value){
            printf("Something went wrong during pin reading, value = %d\n", read_value);
            return -1;
        }

        printf("Read value: %d\n", read_value);
        char input = ' ';
        scanf("%c", &input);
        if('x' == input) {
            break;
        }
        printf("cont..\n");
    }
    int free_success = layer0_finalize();

    return free_success;
}
