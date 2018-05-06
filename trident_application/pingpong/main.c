/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <trident/layer1/layer1.h>
#include <trident_config.h>
#include <trident/util.h>
#include <trident/trident.h>

#define WAIT_RETRIES 20

int finalize(int rvalue) {
    int l1_finalize_success = layer1_finalize();
    if(0 != l1_finalize_success) {
#ifdef TRIDENT_LOGGING
        fprintf(stderr, "layer0_finalize failed with error %d\n", l1_finalize_success);
#endif
    }
    return 0 == rvalue && 0 != l1_finalize_success ? l1_finalize_success : rvalue ;
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

    struct timespec tstart, tend;

    int resetpin_nr = -1;
    int resetresponsepin_nr = -1;
    if(8 > argc || (8 != argc && 10 != argc)) {
        fprintf(stderr, "--usage: pingpong_benchmark <ch0 tx> <ch0 rx> <ch0 data> <ch1 tx> <ch1 rx> <ch1 data> <bit width> [<reset pin nr> <reset response pin nr>]\n");
        return finalize(-1);
    } else if(10 == argc) {
        resetpin_nr = atoi(argv[8]);
        resetresponsepin_nr = atoi(argv[9]);
    }

    CHANNEL_DESCRIPTOR chout, chin;
    chout.tx = atoi(argv[1]);
    chout.rx = atoi(argv[2]);
    chout.data = atoi(argv[3]);
    chout.direction = COMM_IO_OUT;
    chin.tx = atoi(argv[4]);
    chin.rx = atoi(argv[5]);
    chin.data = atoi(argv[6]);
    chin.direction = COMM_IO_IN;
    chout.bit_width = chin.bit_width = atoi(argv[7]);

    BIT* word_out = construct_random_word(chout.bit_width);
    BIT* word_in = (BIT*) calloc(chout.bit_width, sizeof(BIT));

    ERRNO l1_init_success = layer1_initialize();
    if(ERRNO_NO_ERROR != l1_init_success) {
        ERROR(l1_init_success, "layer1_initialize failed");
        return finalize(l1_init_success);
    }

    ERRNO chout_init_success = layer1_aggregate_channel(&chout);
    ERRNO chin_init_success = layer1_aggregate_channel(&chin);
    if(ERRNO_NO_ERROR != chout_init_success) {
        ERROR(chout_init_success, "output channel initialization failed");
        return finalize(chout_init_success);
    }
    if(ERRNO_NO_ERROR != chin_init_success) {
        ERROR(chin_init_success, "input channel initialization failed");
        return finalize(chin_init_success);
    }

    //### initial reset barrier START
    if(-1 < resetpin_nr && -1 < resetresponsepin_nr) {
        ERRNO reset_proc_succ = trident_reset_barrier(resetpin_nr, resetresponsepin_nr, &layer1_reset_channel_pin_states);
        if(ERRNO_NO_ERROR != reset_proc_succ) {
            ERROR(reset_proc_succ, "int main(int, char**): error during reset procedure");
            finalize(reset_proc_succ);
        }
    }
    //### initial reset barrier END

    //### TIMING Area START
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tstart);
    ERRNO sRes = layer1_write_to_channel(chout.channel_nr, word_out, 0);
    if(ERRNO_NO_ERROR != sRes) {
        ERROR(sRes, "Sending failed");
        return finalize(sRes);
    }
    ERRNO rRes = layer1_read_from_channel(chout.channel_nr, word_in, 0);
    if(ERRNO_NO_ERROR != rRes) {
        ERROR(rRes, "receiving failed");
        return finalize(rRes);
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);
    //### TIMING Area END

    // calculate time spent
    double t_ns = (double)(tend.tv_sec - tstart.tv_sec) * 1.0e9 +  (double)(tend.tv_nsec - tstart.tv_nsec);

#ifdef TRIDENT_LOGGING_INFO
    char wordout_string[chout.bit_width + 1];

    for(unsigned int i = 0; i < chout.bit_width; ++i) {
        wordout_string[i] = word_out[i] + '0';
    }
    wordout_string[chout.bit_width] = '\0';
    fprintf(stdout, "out word %s\n", wordout_string);

    char wordin_string[chout.bit_width + 1];
    for(unsigned
        int i = 0; i < chout.bit_width; ++i) {
        wordin_string[i] = word_in[i] + '0';
    }
    wordin_string[chout.bit_width] = '\0';
    fprintf(stdout, " in word %s\n", wordin_string);
#endif

    // check if result == message
    int valid = 1;

    for(unsigned int j = 0; j < chout.bit_width; ++j) {
        valid &= (word_in[j] == word_out[j]) ? 1 : 0;
    }
    //
    printf(" %d %c %f\n", chout.bit_width, 0 == valid ? 'i' : 'v', t_ns);
    free(word_out);
    free(word_in);
    return finalize(0);
}
