/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <trident/trident.h>
#include <trident/types.h>
#include <trident/layer1/layer1.h>


int finalize(int retval) {
    layer1_finalize();
    return retval;
}

int main(int argc, char* argv[]) {
    LOG_LAYER(TRIDENT_APPLICATION_LOG);
    int rpin_nr = -1;
    int rrpin_nr = -1;
    if(5 > argc) {
        fprintf(stderr, "--usage: %s <tx pin nr> <rx pin nr> <data pin nr> <bit width> [<reset pin nr> <reset response pin nr>]\n", argv[0]);
        return -1;
    } else if (5 != argc && 7 != argc){
        fprintf(stderr, "--usage: %s <tx pin nr> <rx pin nr> <data pin nr> <bit width> [<reset pin nr> <reset response pin nr>]\n", argv[0]);
        return -1;
    } else if(7 == argc) {
        rpin_nr = atoi(argv[5]);
        rrpin_nr = atoi(argv[6]);
    }
    CHANNEL_DESCRIPTOR send_ch;
    send_ch.tx = atoi(argv[1]);
    send_ch.rx = atoi(argv[2]);
    send_ch.data = atoi(argv[3]);
    send_ch.bit_width = atoi(argv[4]);
    send_ch.direction = COMM_IO_IN;
    send_ch.channel_nr = -1;
    int l1_init_success = layer1_initialize();
    if(0 != l1_init_success) {
        fprintf(stderr, "layer1_initialize failed with %d\n", l1_init_success);
        return finalize(l1_init_success);
    }

    ERRNO ch_alloc_success = layer1_aggregate_channel(&send_ch);
    if(ERRNO_NO_ERROR != ch_alloc_success) {
        fprintf(stderr, "layer1_aggregate_channel failed %d\n", ch_alloc_success);
        return ch_alloc_success;
    }
    if(-1 == send_ch.channel_nr) {
        fprintf(stderr, "something unforseen happened, channel number was not modified by layer1_aggregate_channel");
        return finalize(-1);
    }

    BIT* word = (BIT*) malloc(send_ch.bit_width * sizeof(BIT));
    for(unsigned int i = 0; i < send_ch.bit_width; ++i) {
        word[i] = -1;
    }

    if(-1 < rpin_nr && -1 < rrpin_nr) {
        ERRNO reset_proc_succ = trident_reset_barrier(rpin_nr, rrpin_nr, &layer1_reset_channel_pin_states);
        if(ERRNO_NO_ERROR != reset_proc_succ) {
            ERROR(reset_proc_succ, "int main(int, char**): error during reset procedure, ret %d");
            return finalize(reset_proc_succ);
        }
    }

    ERRNO read_success = layer1_read_from_channel(send_ch.channel_nr, word, 0);
    if(ERRNO_NO_ERROR != read_success) {
        fprintf(stderr, "layer1_read_from_channel failed %d\n", read_success);
        return finalize(read_success);
    }

    char word_string[send_ch.bit_width + 1];
    for(unsigned int i = 0; i < send_ch.bit_width; ++i) {
        word_string[i] = word[i] + '0';
    }
    word_string[send_ch.bit_width] = '\0';

    printf("Received word %s\n", word_string);


    free(word);
    return finalize(0);
}
