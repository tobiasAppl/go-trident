/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <trident/layer1/layer1.h>
#include <trident_config.h>
#include <trident/util.h>
#include <trident/trident.h>


int finalize(int rvalue) {
    int l1_finalize_success = layer1_finalize();
    if(0 != l1_finalize_success) {
#ifdef TRIDENT_LOGGING
        fprintf(stderr, "layer0_finalize failed with error %d\n", l1_finalize_success);
#endif
    }
    return 0 == rvalue && 0 != l1_finalize_success ? l1_finalize_success : rvalue ;
}

int main(int argc, char* argv[]) {
    LOG_LAYER(TRIDENT_APPLICATION_LOG);

    int resetpin_nr = -1;
    int resetresponsepin_nr = -1;
    if(7 > argc || (7 != argc && 9 != argc)) {
        fprintf(stderr, "--usage: bitwise_ch_alteration <ch0 tx> <ch0 rx> <ch0 data> <ch1 tx> <ch1 rx> <ch1 data> [<reset pin nr> <reset response pin nr>]\n");
        return finalize(-1);
    } else if(9 == argc) {
        resetpin_nr = atoi(argv[7]);
        resetresponsepin_nr = atoi(argv[8]);
    }

    CHANNEL_DESCRIPTOR chout, chin;
    chout.tx = atoi(argv[4]);
    chout.rx = atoi(argv[5]);
    chout.data = atoi(argv[6]);
    chout.direction = COMM_IO_OUT;
    chin.tx = atoi(argv[1]);
    chin.rx = atoi(argv[2]);
    chin.data = atoi(argv[3]);
    chin.direction = COMM_IO_IN;
    chout.bit_width = chin.bit_width = 0;


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
        MESSAGE("Entered reset barrier");
        ERRNO reset_proc_succ = trident_reset_barrier(resetpin_nr, resetresponsepin_nr, &layer1_reset_channel_pin_states);
        if(ERRNO_NO_ERROR != reset_proc_succ) {
            ERROR(reset_proc_succ, "int main(int, char**): error during reset procedure");
            finalize(reset_proc_succ);
        }
        MESSAGE("Passed reset barrier");
    }
    //### initial reset barrier END

    for(unsigned int bwidth = 1; 1000 >= bwidth; ++bwidth) {
        MESSAGE("Sending %d bit(s)", bwidth);
        layer1_set_channel_bit_width(chin.channel_nr, bwidth);
        layer1_set_channel_bit_width(chout.channel_nr, bwidth);
        int runs = 1;
        if(500 < bwidth) {
            runs = 3;
        }
        else if(200 < bwidth) {
            runs = 5;
        }
        else {
            runs = 20;
        }
        for(int i = 0; i < runs; ++i) {
            MESSAGE("   - run %i", i+1);
            BIT* word = (BIT*) calloc(bwidth, sizeof(BIT));
            ERRNO rRes = layer1_read_from_channel(chin.channel_nr, word, 0);
            if(ERRNO_NO_ERROR != rRes) {
                ERROR(rRes, "receiving failed");
                return finalize(rRes);
            }

#ifdef TRIDENT_LOGGING_INFO
            char word_string[bwidth + 1];

            for(unsigned int i = 0; i < bwidth; ++i) {
                word_string[i] = word[i] + '0';
            }
            word_string[bwidth] = '\0';
            fprintf(stdout, "out word %s\n", word_string);
#endif

            ERRNO sRes = layer1_write_to_channel(chout.channel_nr, word, 0);
            if(ERRNO_NO_ERROR != sRes) {
                ERROR(sRes, "Sending failed");
                return finalize(sRes);
            }

            free(word);
        }
    }
    return finalize(0);
}
