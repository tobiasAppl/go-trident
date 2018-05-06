/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include "trident.h"
#include "layer1/layer1.h"
#include "layer0/layer0.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ctype.h>

typedef struct {
  CHANNEL_DESCRIPTOR* channel_ptr;
  WORD data_word;
} CHANNEL_IO_BUFFER;

CHANNEL_IO_BUFFER* __trident_write_buffer;
unsigned int __write_buffer_length;

CHANNEL_IO_BUFFER* __trident_result_buffer;
unsigned int __result_buffer_length;

ERRNO trident_initialize()
{
    __trident_write_buffer = NULL;
    __write_buffer_length = 0;

    __trident_result_buffer = NULL;
    __result_buffer_length = 0;
    return layer1_initialize();
}

ERRNO trident_allocate_channels(int channel_c, CHANNEL_DESCRIPTOR* channels)
{
    LOG_LAYER(TRIDENT_LAYER2_LOG);
    for(int i = 0; i < channel_c; ++i) {
        ERRNO chaggregate_success = layer1_aggregate_channel(&channels[i]);
        if(ERRNO_NO_ERROR != chaggregate_success) {
            ERROR(chaggregate_success, "ERRNO trident_allocate_channels(int, CHANNEL_DESCRIPTOR*): unable to allocated channels[%d]", i);
            return chaggregate_success;
        }
        else if(COMM_IO_IN == channels[i].direction) {
            __trident_result_buffer = (CHANNEL_IO_BUFFER*) realloc(__trident_result_buffer, ++__result_buffer_length * sizeof(CHANNEL_IO_BUFFER));
            ERRNO refaccesserr = layer1_get_channel_descriptor_reference(channels[i].channel_nr, &(__trident_result_buffer[__result_buffer_length-1].channel_ptr));
            if(ERRNO_NO_ERROR != refaccesserr) {
                ERROR(refaccesserr, "ERRNO trident_allocate_channels(int, CHANNEL_DESCRIPTOR): unable to get channel reference for channel %d", channels[i].channel_nr);
                return refaccesserr;
            }
            __trident_result_buffer[__result_buffer_length - 1].data_word.word_length = 0;
            __trident_result_buffer[__result_buffer_length - 1].data_word.word_data = NULL;
        }
        else if(COMM_IO_OUT == channels[i].direction) {
            __trident_write_buffer = (CHANNEL_IO_BUFFER*) realloc(__trident_write_buffer, ++__write_buffer_length * sizeof(CHANNEL_IO_BUFFER));
            ERRNO refaccesserr = layer1_get_channel_descriptor_reference(channels[i].channel_nr, &(__trident_write_buffer[__write_buffer_length - 1].channel_ptr));
            if(ERRNO_NO_ERROR != refaccesserr) {
                ERROR(refaccesserr, "ERRNO trident_allocate_channels(int, CHANNEL_DESCRIPTOR): unable to get channel reference for channel %d", channels[i].channel_nr);
                return refaccesserr;
            }
            __trident_write_buffer[__write_buffer_length - 1].data_word.word_length = 0;
            __trident_write_buffer[__write_buffer_length - 1].data_word.word_data = NULL;
        }
    }
    return ERRNO_NO_ERROR;
}


ERRNO trident_execute_communication()
{
    LOG_LAYER(TRIDENT_LAYER2_LOG);

    unsigned int channel_c = __result_buffer_length + __write_buffer_length;
    if(0 == channel_c) {
        return ERRNO_NO_ERROR;
    }
    pid_t running_thread_ids[channel_c]; //TODO handle with linked list to increase performance
    int running_thread_c = channel_c;

    //allocate share memory to store thread results
    // Calculate needed space
    int shared_space_size = 0;

    layer1_reset_channel_iterator();
    for(unsigned int i = 0; __result_buffer_length > i; ++i) {
        shared_space_size += __trident_result_buffer[i].channel_ptr->bit_width * sizeof(BIT);
    }

    // Allocate needed space in shared memory
    int d_shmid;
    key_t d_shm_key = ftok(".", 10);

    // Try to open/create share memory
    if( -1 == (d_shmid = shmget(d_shm_key, shared_space_size * sizeof(BIT), IPC_CREAT | IPC_EXCL | 0666)) ) {
        // Retry in client mode
        if( - 1 == (d_shmid = shmget(d_shm_key, shared_space_size * sizeof(BIT), 0)) ) {
            ERROR(ERRNO_COMMUNICATION_SHARED_MEMORY_ACCESS, "ERRNO trident_execute_communication(): unable to open shared ready state memory");
            return ERRNO_COMMUNICATION_SHARED_MEMORY_ACCESS;
        }
    }

    //spawn a thread for each receive channel
    int shm_offset = 0;
    for(unsigned int i = 0; __result_buffer_length > i; ++i) {
        CHANNEL_DESCRIPTOR* chd = __trident_result_buffer[i].channel_ptr;

        if(COMM_IO_IN != chd->direction) {
            ERROR(ERRNO_CHANNEL_DIRECTION, "ERRNO trident_execute_communication(): Channel %d somehow landed in the result buffer array, even though it's not an input channel, comm direction = %d", chd->channel_nr, chd->direction);
            return ERRNO_CHANNEL_DIRECTION;
        }

        pid_t pid = fork();
        if(0 == pid) {
            //  attach shared memory to this process
            BIT* shm_ptr = NULL;
            if((BIT*)-1 == (shm_ptr = (BIT*) shmat(d_shmid, 0, 0)) ) {
                ERROR(ERRNO_COMMUNICATION_SHARED_MEMORY_ACCESS, "ERRNO trident_execute_communication(): unable to attach shared ready state memory to this process %d", getpid());
                exit(ERRNO_COMMUNICATION_SHARED_MEMORY_ACCESS);
            }
            // set off shm ptr to the location reserved for this process' channel data
            BIT* c_shm_ptr = shm_ptr + shm_offset;

            ERRNO err_comm_in = layer1_read_from_channel(chd->channel_nr, c_shm_ptr, 0);
            if(ERRNO_NO_ERROR != err_comm_in) {
                ERROR(err_comm_in, "ERRNO trident_execute_communication(): error during reading from channel %d, PID  %d", chd->channel_nr, getpid());
                exit(err_comm_in);
            }

            exit(ERRNO_NO_ERROR);//terminate child thread
        }
        else {
            running_thread_ids[i] = pid;
        }


        shm_offset += chd->bit_width * sizeof(BIT);
    }

    if(shared_space_size != shm_offset) {
        ERROR(ERRNO_UNDEFINED, "ERRNO trident_execute_communication(): something went wrong during thread spawn process:\n shared_space_size %d != shared_memory_offset %d", shared_space_size, shm_offset);
    }

    //span threads for output channels
    for(unsigned int i = 0; __write_buffer_length > i; ++i) {
        CHANNEL_DESCRIPTOR* chd = __trident_write_buffer[i].channel_ptr;

        if(COMM_IO_OUT != chd->direction) {
            ERROR(ERRNO_CHANNEL_DIRECTION, "ERRNO trident_execute_communication(): Channel %d somehow landed in the write buffer array, even though it's not an output channel, comm direction = %d", chd->channel_nr, chd->direction);
            return ERRNO_CHANNEL_DIRECTION;
        }
        pid_t pid = fork();
        if(0 == pid) { //child thread
            for(unsigned int i = 0; __write_buffer_length > i ; ++i) {
                if(__trident_write_buffer[i].channel_ptr->channel_nr == chd->channel_nr &&
                   0 < __trident_write_buffer[i].data_word.word_length) {

                    ERRNO err_comm_out = layer1_write_to_channel(__trident_write_buffer[i].channel_ptr->channel_nr, __trident_write_buffer[i].data_word.word_data, 0);
                    if(ERRNO_NO_ERROR != err_comm_out) {
                        ERROR(err_comm_out, "ERRNO trident_execute_communication(): error during writing a word of length %d to channel %d, PID %d", __trident_write_buffer[i].data_word.word_length, chd->channel_nr, getpid() );
                        exit(err_comm_out);
                    }
                }
            }

            exit(ERRNO_NO_ERROR);//terminate child thread
        }
        else {
            running_thread_ids[__result_buffer_length + i] = pid;
        }
    }
    //wait for all child processes to terminate
    for ever {
        int wstatus;
        int term_thread_idx = -1;
        pid_t wpid = wait(&wstatus);

        if(0 > wpid) { //error
            ERROR(ERRNO_COMMUNICATION_EXECUTION_THREAD_WAIT, "ERRNO trident_execute_communication(): negative return value from wait function");
        }
        else if( 0 < wpid ) { // a thread terminated successfully            
            //find thread index in the array of running communication threads
            for(int i = 0; i < running_thread_c; ++i) {
                if(running_thread_ids[i] == wpid) {
                    term_thread_idx = i;
                }
            }
            if(-1 != term_thread_idx) {
                //shift all elements from positions > i one position down to eliminate element i
                for(int i = term_thread_idx + 1; i < running_thread_c; i++) {
                    running_thread_ids[i-1] = running_thread_ids[i];
                }
                //the last element is now empty, so decrement size of the thread array
                --running_thread_c;
            }

            //TODO child thread exit status management
        }
        if(0 == running_thread_c) {
            break;
        }
    }


    // copy from shared memory to read buffer
    //  attach share memory
    BIT* shm_ptr = NULL;
    if((BIT*)-1 == (shm_ptr = (BIT*) shmat(d_shmid, 0, 0)) ) {
        ERROR(ERRNO_COMMUNICATION_SHARED_MEMORY_ACCESS, "ERRNO trident_execute_communication(): unable to attach shared ready state memory to the main process %d", getpid());
        exit(ERRNO_COMMUNICATION_SHARED_MEMORY_ACCESS);
    }

    shm_offset = 0;
    for(unsigned int i = 0; __result_buffer_length > i; ++i) {
        CHANNEL_IO_BUFFER* in_buffer_ptr = &__trident_result_buffer[i];

        BIT* c_shm_ptr = shm_ptr + shm_offset;

        // Allocate data_word.word_data
        if(NULL != in_buffer_ptr->data_word.word_data) {
            if(in_buffer_ptr->data_word.word_length != in_buffer_ptr->channel_ptr->bit_width) {
                free(in_buffer_ptr->data_word.word_data);
                in_buffer_ptr->data_word.word_length = in_buffer_ptr->channel_ptr->bit_width;
                in_buffer_ptr->data_word.word_data = (BIT*) malloc(in_buffer_ptr->channel_ptr->bit_width * sizeof(BIT));
            }
        }
        else {
            in_buffer_ptr->data_word.word_length = in_buffer_ptr->channel_ptr->bit_width;
            in_buffer_ptr->data_word.word_data = (BIT*) malloc(in_buffer_ptr->channel_ptr->bit_width * sizeof(BIT));
        }

        for(unsigned int b_offset = 0; b_offset < in_buffer_ptr->data_word.word_length; ++b_offset) {
            in_buffer_ptr->data_word.word_data[b_offset] = c_shm_ptr[b_offset];
        }

        shm_offset += in_buffer_ptr->channel_ptr->bit_width * sizeof(BIT);
    }

    if(shared_space_size != shm_offset) {
        ERROR(ERRNO_UNDEFINED, "ERRNO trident_execute_communication(): something went wrong when getting back data from the shared memory:\n shared_space_size %d != shared_memory_offset %d", shared_space_size, shm_offset);
    }
    // free shared memory
    shmctl(d_shmid, IPC_RMID, 0);

    return ERRNO_NO_ERROR;
}

ERRNO trident_write_to_output_channel_buffer(unsigned int channel_nr, WORD data_word) {
    LOG_LAYER(TRIDENT_LAYER2_LOG);
    for(unsigned int i = 0; i < __write_buffer_length; ++i) {
        if(channel_nr == (unsigned int)__trident_write_buffer[i].channel_ptr->channel_nr) {
            WORD* w_dword = &(__trident_write_buffer[i].data_word);

            if(__trident_write_buffer[i].channel_ptr->bit_width != data_word.word_length) {
                ERROR(ERRNO_COMMUNICATION_WRITE_BUFFER_LENGTH, "ERRNO trident_set_channel_write_buffer(unsigned int, WORD): specified new data word length %d is not equal to the internal channel buffer length %d for channel %d", data_word.word_length, w_dword->word_length, channel_nr);
                return ERRNO_COMMUNICATION_WRITE_BUFFER_LENGTH;
            }

            if(w_dword->word_length != data_word.word_length) {
                if(NULL != data_word.word_data) {
                    free(w_dword->word_data);
                }
                w_dword->word_length = data_word.word_length;
                w_dword->word_data = (BIT*) malloc(data_word.word_length * sizeof(BIT));
            }

            for(unsigned int l = 0; l < w_dword->word_length; ++l) {
                w_dword->word_data[l] = data_word.word_data[l];
            }
            return ERRNO_NO_ERROR;
        }
    }

    ERROR(ERRNO_CHANNEL_NOT_INITIALIZED, "ERRNO trident_set_channel_write_buffer(unsigned int, WORD): no write buffer exists for channel number %d", channel_nr);
    return ERRNO_CHANNEL_NOT_INITIALIZED;
}

ERRNO trident_get_read_result(unsigned int channel_nr, WORD* result_buffer)
{
    LOG_LAYER(TRIDENT_LAYER2_LOG);

    for(unsigned int i = 0; i < __result_buffer_length; ++i) {
        if(channel_nr == (unsigned int) __trident_result_buffer[i].channel_ptr->channel_nr) {
            WORD* rword = &__trident_result_buffer[i].data_word;
            result_buffer->word_data = (BIT*) calloc(rword->word_length, sizeof(BIT));
            result_buffer->word_length = rword->word_length;

            for(unsigned int l = 0; l < rword->word_length; ++l) {
                result_buffer->word_data[l] = rword->word_data[l];
            }
            return ERRNO_NO_ERROR;
        }
    }

    ERROR(ERRNO_CHANNEL_NOT_INITIALIZED, "ERRNO trident_get_read_result(unsigned int, WORD*): no read buffer existing for channel number %d", channel_nr);
    return ERRNO_CHANNEL_NOT_INITIALIZED;
}


ERRNO trident_finalize()
{
    for(unsigned int i = 0; i < __result_buffer_length; ++ i) {
        if(NULL != __trident_result_buffer[i].data_word.word_data) {
            free(__trident_result_buffer[i].data_word.word_data);
        }
    }
    if(NULL != __trident_result_buffer) {
        free(__trident_result_buffer);
    }
    __result_buffer_length = 0;

    for(unsigned int i = 0; i < __write_buffer_length; ++ i) {
        if(NULL != __trident_write_buffer[i].data_word.word_data) {
            free(__trident_write_buffer[i].data_word.word_data);
        }
    }
    if(NULL != __trident_write_buffer) {
        free(__trident_write_buffer);
    }
    __write_buffer_length = 0;

    return layer1_finalize();
}


/*** RESET BARRIER ***/

ERRNO trident_reset_barrier(unsigned int reset_pin, unsigned int reset_response_pin, ERRNO (*barrier_operation_function)(void)) {
    LOG_LAYER(TRIDENT_LAYER2_LOG);
    MESSAGE("ERRNO layer1_reset_procedure(): Entered function");

    //1. initialize reset and reset response
    MESSAGE("ERRNO layer1_reset_procedure(): initializing reset pin %d and reset response pin %d", reset_pin, reset_response_pin);
    ERRNO rp_init_s = layer0_initialize_pin(reset_pin, COMM_IO_IN);
    if(ERRNO_NO_ERROR != rp_init_s) {
        ERROR(rp_init_s, "ERRNO layer1_reset_procedure(): unexpected reset pin init result");
        return rp_init_s;
    }

    ERRNO rrp_init_s = layer0_initialize_pin(reset_response_pin, COMM_IO_OUT);
    if(ERRNO_NO_ERROR != rrp_init_s) {
        ERROR(rrp_init_s, "ERRNO layer1_reset_procedure(): unexpected reset response pin init result");
        return rrp_init_s;
    }

    ERRNO rrp_reset_succ = layer0_write_to_pin(reset_response_pin, BIT0);
    if(ERRNO_NO_ERROR != rrp_reset_succ) {
        ERROR(rrp_reset_succ, "ERRNO layer1_reset_procedure(): error during reset response pin reset");
        return rrp_reset_succ;
    }


    //2. set response pin to 1
    MESSAGE("ERRNO layer1_reset_procedure(): setting response pin %d to 1", reset_response_pin);
    ERRNO rrp_set_succ = layer0_write_to_pin(reset_response_pin, BIT1);
    if(ERRNO_NO_ERROR != rrp_set_succ) {
        ERROR(rrp_set_succ, "ERRNO layer1_reset_procedure(): error while setting response pin to 1");
        return rrp_set_succ;
    }

    //3. wait for reset pin to become 1
    MESSAGE("ERRNO layer1_reset_procedure(): waiting for reset pin %d to become 1", reset_pin);
    BIT rpval = BIT0;
    while(BIT1 != rpval) {
        ERRNO rp_read_succ = layer0_read_from_pin(reset_pin, &rpval);
        if(ERRNO_NO_ERROR != rp_read_succ) {
            ERROR(rp_read_succ, "ERRNO layer1_reset_procedure(): error while waiting for rp to become 1");
            return rp_read_succ;
        }
    }

    //4. executing reset barrier operation function
    MESSAGE("ERRNO layer1_reset_procedure(): executing operation function");
    ERRNO op_res = (*barrier_operation_function)();
    if(ERRNO_NO_ERROR != op_res) {
        ERROR(op_res, "ERRNO layer1_reset_procedure(): error during operation function execution");
        return op_res;
    }

    //5. set response pin to 0
    MESSAGE("ERRNO layer1_reset_procedure(): setting reset response pin %d to 0", reset_response_pin);
    rrp_set_succ = layer0_write_to_pin(reset_response_pin, BIT0);
    if(ERRNO_NO_ERROR != rrp_set_succ) {
        ERROR(rrp_set_succ, "ERRNO layer1_reset_procedure(): error while setting response pin to 0");
        return rrp_set_succ;
    }

    //6. wait for reset pin to become 0
    MESSAGE("ERRNO layer1_reset_procedure(): waiting for reset pin %d to become 0", reset_pin);
    rpval = BIT1; //just to be sure
    while(BIT0 != rpval) {
        ERRNO rp_read_succ = layer0_read_from_pin(reset_pin, &rpval);
        if(ERRNO_NO_ERROR != rp_read_succ) {
            ERROR(rp_read_succ, "ERRNO layer1_reset_procedure(): error while waiting for rp to become 0");
            return rp_read_succ;
        }
    }

    return ERRNO_NO_ERROR;
}

ERRNO trident_reset_barrier_controller(unsigned int reset_pin, unsigned int* reset_response_pins, unsigned int c_response_pins) {
    LOG_LAYER(TRIDENT_LAYER2_LOG);
    if(0 >= c_response_pins) {
        ERROR(ERRNO_RESET_RESPONSE_PIN_INVALID, "ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): at least 1 reset response pin is required for the reset barrier controller");
        return ERRNO_RESET_RESPONSE_PIN_INVALID;
    }
    if(NULL == reset_response_pins) {
        ERROR(ERRNO_RESET_RESPONSE_PIN_INVALID, "ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): reset response pin vector = NULL");
        return ERRNO_RESET_RESPONSE_PIN_INVALID;
    }

    ERRNO rp_init = layer0_initialize_pin(reset_pin, COMM_IO_OUT);
    if(ERRNO_NO_ERROR != rp_init) {
        ERROR(rp_init, "ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): error while initilizing reset pin COMM_IO_OUT %d", reset_pin);
        return rp_init;
    }
    bool successful_pin_init = true;
    for(unsigned int i = 0; c_response_pins > i; ++i) {
        unsigned int rrpin = reset_response_pins[i];
        ERRNO rrp_init = layer0_initialize_pin(rrpin, COMM_IO_IN);
        successful_pin_init = successful_pin_init && (ERRNO_NO_ERROR == rrp_init);
    }
    if(!successful_pin_init) {
        ERROR(ERRNO_PIN_INIT, "ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): unable to initialize all response pins");
        return ERRNO_PIN_INIT;
    }
    //0. set reset pin to 0
    MESSAGE("ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): set reset pin to 0");
    ERRNO set_reset_succ = layer0_write_to_pin(reset_pin, BIT0);
    if(ERRNO_NO_ERROR != set_reset_succ) {
        ERROR(set_reset_succ, "ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): unable to set reponse pin to 0");
        return set_reset_succ;
    }
    //1. wait for all response pins to become 1
    MESSAGE("ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): wait for all reset response pins to become 1");
    bool successful_read = true;
    bool all_responses_one;
    do {
        all_responses_one = true;
        for(unsigned int i = 0; c_response_pins > i; ++i) {
            BIT readbuffer = BIT0;
            ERRNO response_read_succ = layer0_read_from_pin(reset_response_pins[i], &readbuffer);
            successful_read = successful_read && (ERRNO_NO_ERROR == response_read_succ);
            all_responses_one = all_responses_one && (BIT1 == readbuffer);
        }
        if(!successful_read) {
            break;
        }
    } while(!all_responses_one);
    if(!successful_read) {
        ERROR(ERRNO_PIN_READ, "ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): an error occurred while trying to read from a response pin");
        return ERRNO_PIN_READ;
    }

    //2. set reset pin to one
    MESSAGE("ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): set reset pin to 1");
    set_reset_succ = layer0_write_to_pin(reset_pin, BIT1);
    if(ERRNO_NO_ERROR != set_reset_succ) {
        ERROR(set_reset_succ, "ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): unable to set reponse pin to 1");
        return set_reset_succ;
    }

    //3. wait for all the response pins to become 0
    MESSAGE("ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): wait for all response pins to become zero");
    successful_read = true;
    bool all_responses_zero;
    do {
        all_responses_zero = true;
        for(unsigned int i = 0; c_response_pins > i; ++i) {
            BIT readbuffer = BIT1;
            ERRNO response_read_succ = layer0_read_from_pin(reset_response_pins[i], &readbuffer);
            successful_read = successful_read && (ERRNO_NO_ERROR == response_read_succ);
            all_responses_zero = all_responses_zero && (BIT0 == readbuffer);
        }
        if(!successful_read) {
            break;
        }
    } while(!all_responses_zero);
    if(!successful_read) {
        ERROR(ERRNO_PIN_READ, "ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): an error occurred while trying to read from a response pin");
        return ERRNO_PIN_READ;
    }

    //4. set reset pin to 0
    MESSAGE("ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): set reset pin to zero");
    set_reset_succ = layer0_write_to_pin(reset_pin, BIT0);
    if(ERRNO_NO_ERROR != set_reset_succ) {
        ERROR(set_reset_succ, "ERRNO layer1_reset_barrier_controller(unsigned int, unsigned int*, unsigned int): unable to set reponse pin to 0");
        return set_reset_succ;
    }

    return ERRNO_NO_ERROR;
}
