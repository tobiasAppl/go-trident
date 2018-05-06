/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <trident/trident.h>
#include <trident/layer1/layer1.h>
#include <time.h>

int finalize(int retval) {
    layer1_finalize();
    return retval;
}

BIT* construct_random_word(int bit_width) {
    BIT* word = (BIT*) calloc(bit_width, sizeof(BIT));
    int i;

    time_t t;
    srand((unsigned)time(&t));
    for(i = 0; i < bit_width; ++i) {
        int r = rand() % 100;
        BIT randv = 50 >= r ? BIT0 : BIT1;
        word[i] = randv;
    }
    return word;
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
    send_ch.direction = COMM_IO_OUT;

    int l1_init_success = layer1_initialize();
    if(0 != l1_init_success) {
        fprintf(stderr, "layer1_initialize failed with %d\n", l1_init_success);
        return finalize(l1_init_success);
    }

    ERRNO ch_alloc_success = layer1_aggregate_channel(&send_ch);
    if(ERRNO_NO_ERROR != ch_alloc_success) {
        fprintf(stderr, "layer1_aggregate_channel failed %d\n", ch_alloc_success);
        return finalize(ch_alloc_success);
    }

    BIT* word = construct_random_word(send_ch.bit_width);

    char word_string[send_ch.bit_width + 1];

    for(unsigned int i = 0; i < send_ch.bit_width; ++i) {
        word_string[i] = word[i] + '0';
    }
    word_string[send_ch.bit_width] = '\0';
    printf("int main(int, char**): generated message - %s\n", word_string);

    if(-1 < rpin_nr && -1 < rrpin_nr) {
        ERRNO reset_proc_succ = trident_reset_barrier(rpin_nr, rrpin_nr, &layer1_reset_channel_pin_states);
        if(ERRNO_NO_ERROR != reset_proc_succ) {
            ERROR(reset_proc_succ, "int main(int, char**): error during reset procedure, ret %d");
            return finalize(reset_proc_succ);
        }
    }

    printf("Sending word ...\n");

    char wordin_string[send_ch.bit_width + 1];
    for(unsigned int i = 0; i < send_ch.bit_width; ++i) {
        wordin_string[i] = word[i] + '0';
    }
    wordin_string[send_ch.bit_width] = '\0';
    fprintf(stdout, "Word: %s\n", wordin_string);

    struct timespec tstart, tend;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tstart);
    ERRNO write_success = layer1_write_to_channel(send_ch.channel_nr, word, 0);
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);

    double t_ns = (double)(tend.tv_sec - tstart.tv_sec) * 1.0e9 +  (double)(tend.tv_nsec - tstart.tv_nsec);

    printf("send time: %fns\n", t_ns);

    if(ERRNO_NO_ERROR != write_success) {
        ERROR(write_success, "int main(int, char**): layer1_write_to_channel failed %d\n");
        return finalize(write_success);
    }

    free(word);

    return finalize(ERRNO_NO_ERROR);
}
