/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <trident/layer1/layer1.h>
#include <trident_config.h>
#include <trident/util.h>
#include <trident/trident.h>

#define WAIT_RETRIES 20

char* logfile_str = NULL;
FILE* logfile = NULL;

int finalize(int rvalue) {
    LOG_LAYER(TRIDENT_APPLICATION_LOG);
    int l1_finalize_success = layer1_finalize();
    if(NULL != logfile_str) {
        free(logfile_str);
    }
    if(NULL != logfile) {
        fclose(logfile);
    }
    if(0 != l1_finalize_success) {
        ERROR(l1_finalize_success, "layer0_finalize failed with error %d\n", l1_finalize_success);
    }
    return (0 == rvalue && 0 != l1_finalize_success) ? l1_finalize_success : rvalue ;
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
        fprintf(stderr, "--usage: pingpong_benchmark <log file> <ch0 tx> <ch0 rx> <ch0 data> <ch1 tx> <ch1 rx> <ch1 data> [<reset pin nr> <reset response pin nr>]\n");
        return finalize(-1);
    } else if(10 == argc) {
        resetpin_nr = atoi(argv[8]);
        resetresponsepin_nr = atoi(argv[9]);
    }

    logfile_str = strdup(argv[1]);
    logfile = fopen(logfile_str, "w");
    if(NULL == logfile) {
        ERROR(-1, "Unable to open benchmark log file %s", logfile_str);
        return finalize(-1);
    }

    CHANNEL_DESCRIPTOR chout, chin;
    chout.tx = atoi(argv[2]);
    chout.rx = atoi(argv[3]);
    chout.data = atoi(argv[4]);
    chout.direction = COMM_IO_OUT;
    chin.tx = atoi(argv[5]);
    chin.rx = atoi(argv[6]);
    chin.data = atoi(argv[7]);
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
        ERRNO reset_proc_succ = trident_reset_barrier(resetpin_nr, resetresponsepin_nr, &layer1_reset_channel_pin_states);
        if(ERRNO_NO_ERROR != reset_proc_succ) {
            ERROR(reset_proc_succ, "int main(int, char**): error during reset procedure");
            finalize(reset_proc_succ);
        }
        MESSAGE("Passed reset barrier");
    }
    //### initial reset barrier END

    for(unsigned int bwidth = 1; 1000 >= bwidth; ++bwidth) {
        MESSAGE("Sending %d bits", bwidth);
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
            BIT* word_out = construct_random_word(bwidth);
            BIT* word_in = (BIT*) calloc(bwidth, sizeof(BIT));
            //### TIMING Area START
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tstart);
            ERRNO sRes = layer1_write_to_channel(chout.channel_nr, word_out, 0);
            if(ERRNO_NO_ERROR != sRes) {
                ERROR(sRes, "Sending failed");
                return finalize(sRes);
            }
            ERRNO rRes = layer1_read_from_channel(chin.channel_nr, word_in, 0);
            if(ERRNO_NO_ERROR != rRes) {
                ERROR(rRes, "receiving failed");
                return finalize(rRes);
            }
            clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tend);
            //### TIMING Area END

            // calculate time spent
            double t_ns = (double)(tend.tv_sec - tstart.tv_sec) * 1.0e9 +  (double)(tend.tv_nsec - tstart.tv_nsec);

#ifdef TRIDENT_LOGGING_INFO
            char wordout_string[bwidth + 1];

            for(unsigned int i = 0; i < bwidth; ++i) {
                wordout_string[i] = word_out[i] + '0';
            }
            wordout_string[bwidth] = '\0';
            fprintf(stdout, "out word %s\n", wordout_string);

            char wordin_string[bwidth + 1];
            for(unsigned
                int i = 0; i < bwidth; ++i) {
                wordin_string[i] = word_in[i] + '0';
            }
            wordin_string[bwidth] = '\0';
            fprintf(stdout, " in word %s\n", wordin_string);
#endif

            // check if result == message
            int valid = 1;

            for(unsigned int j = 0; j < bwidth; ++j) {
                valid &= (word_in[j] == word_out[j]) ? 1 : 0;
            }

            //echo status
            MESSAGE(" %d %c %f\n", bwidth, 0 == valid ? 'i' : 'v', t_ns);
            fprintf(logfile, " %d %c %f\n", bwidth, 0 == valid ? 'i' : 'v', t_ns);
            fflush(logfile);
            free(word_out);
            free(word_in);
        }
    }
    return finalize(0);
}
