/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <trident/trident.h>


ERRNO finalize(ERRNO errno);
void printUsage();
BIT* construct_random_word(int bit_width);

int main(int argc, char* argv[]) {
    LOG_LAYER(TRIDENT_APPLICATION_LOG);
    if(8 != argc) {
        printUsage();
        return ERRNO_FUNCTION_ARGUMENT_VALUES;
    }
    CHANNEL_DESCRIPTOR channels[2];
    channels[0].channel_nr = -1;
    channels[0].direction = COMM_IO_OUT;
    channels[0].tx = atoi(argv[1]);
    channels[0].rx = atoi(argv[2]);
    channels[0].data = atoi(argv[3]);

    channels[1].channel_nr = -1;
    channels[1].direction = COMM_IO_IN;
    channels[1].tx = atoi(argv[4]);
    channels[1].rx = atoi(argv[5]);
    channels[1].data = atoi(argv[6]);

    int word_length = atoi(argv[7]);

    channels[0].bit_width = word_length;
    channels[1].bit_width = word_length;

    ERRNO init_succ = trident_initialize();
    if(ERRNO_NO_ERROR != init_succ) {
        return finalize(init_succ);
    }
    MESSAGE("Initialized trident");

    ERRNO challoc_succ = trident_allocate_channels(2, channels);
    if(ERRNO_NO_ERROR != challoc_succ) {
        return finalize(challoc_succ);
    }
    MESSAGE("Allocated sender channel %d", channels[0].channel_nr);
    MESSAGE("Allocated sender channel %d", channels[1].channel_nr);

    WORD send, receive;
    send.word_length = word_length;
    send.word_data = construct_random_word(word_length);
    char* wstr = word_to_string(send);
    MESSAGE("Writing to output buffer word \'%s\'", wstr);
    free(wstr);
    ERRNO outbufferwritesucc = trident_write_to_output_channel_buffer(channels[0].channel_nr, send);
    if(ERRNO_NO_ERROR != outbufferwritesucc){
        return finalize(outbufferwritesucc);
    }

    MESSAGE("Executing Communication ...");
    ERRNO commsucc = trident_execute_communication();
    if(ERRNO_NO_ERROR != commsucc) {
        return finalize(commsucc);
    }

    ERRNO inbufferreadsucc = trident_get_read_result(channels[1].channel_nr, &receive);
    if(ERRNO_NO_ERROR != inbufferreadsucc) {
        return finalize(commsucc);
    }
    wstr = word_to_string(receive);
    MESSAGE("Received word on input channel: \'%s\'", wstr);
    free(wstr);
    MESSAGE("Result: %s", word_equals(send, receive) ? "Transmission valid": "Transmission invalid");

    return finalize(ERRNO_NO_ERROR);
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

ERRNO finalize(ERRNO errno) {
    LOG_LAYER(TRIDENT_APPLICATION_LOG);
    MESSAGE("Finalizing Application");
    ERRNO finerr = trident_finalize();
    if(ERRNO_NO_ERROR != finerr) {
        ERROR(finerr, "APPLICATION: error during trident finalization");
    }
    return ERRNO_NO_ERROR == errno ? finerr : errno;
}

void printUsage() {
    fprintf(stderr, "--usage: ./multichannel_loop <tx0 pin> <rx0 pin> <data0 pin> <tx1 pin> <rx1 pin> <data1 pin> <bit width>");
}
