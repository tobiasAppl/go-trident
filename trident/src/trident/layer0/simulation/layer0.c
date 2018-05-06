/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include <trident/layer0/layer0.h>
#include <trident_config.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <ctype.h>
#include <trident/util.h>

#include "pin_descriptor.h"

#define FIFO_NAME_PREAMBLE "td_fifo_wire_"

PIN_DESCRIPTOR trident_layer0_pin_settings[TRIDENT_PIN_COUNT];

ERRNO layer0_initialize() {
    LOG_LAYER(TRIDENT_LAYER0_LOG);
    unsigned int fifo_dir_strlen = strlen(TRIDENT_SIMULATION_IPC_DUMMY_FILE_FOLDER);
    unsigned int fifo_preamble_strlen = strlen(FIFO_NAME_PREAMBLE);

    char* line = NULL;
    size_t len = 0;
    ssize_t read_len;

    FILE* cfg_fp = fopen(TRIDENT_SIMULATION_IPC_WIRE_CONFIG, "r");
    if(NULL == cfg_fp) {
        ERROR(ERRNO_SIMULATION_IPC_CONFIG_FILE,"ERRNO layer0_initialize(): unable to open file %s", TRIDENT_SIMULATION_IPC_WIRE_CONFIG );
        return ERRNO_SIMULATION_IPC_CONFIG_FILE;
    }
    int line_no = 0;
    while(-1 != (read_len = getline(&line, &len, cfg_fp))) {
        line_no++;
        bool is_empty = true;
        for(int i = 0; read_len > i; ++i) {
            if(!isspace(line[i])) {
                is_empty = false;
                break;
            }
        }
        if(!is_empty) { //skip empty lines
            int line_strlen = strlen(line);
            if(0 < line_strlen && '#' != line[0]) {
                char c_char = ' ';
                //read pin_nr
                int line_pos = 0;

                //ignore all except numbers at the start
                for(; line_pos < line_strlen; line_pos++) {
                    c_char = line[line_pos];
                    if('0' <= c_char && '9' >= c_char) {
                        break;
                    }
                }
                // read number digits
                int pin_nr = c_char - '0';
                while(true) {
                    line_pos++;
                    if(line_pos >= line_strlen) {
                        ERROR(ERRNO_SIMULATION_IPC_CONFIG_FILE_PARSING, "ERRNO layer0_initialize(): unexpected eol at line %d, col %d", line_no, line_pos);
                        return ERRNO_SIMULATION_IPC_CONFIG_FILE_PARSING;
                    }
                    c_char = line[line_pos];
                    if('0' <= c_char && '9' >= c_char) {
                        pin_nr *= 10;
                        pin_nr += c_char - '0';
                    }
                    else {
                        break;
                    }
                }
                if(0 > pin_nr || TRIDENT_PIN_COUNT <= pin_nr ) {
                    ERROR(ERRNO_SIMULATION_IPC_CONFIG_FILE_PARSING, "ERRNO layer0_initialize(): pin number %d not allowed, at line %d, col %d", pin_nr, line_no, line_pos);
                    return ERRNO_SIMULATION_IPC_CONFIG_FILE_PARSING;
                }
                if(NULL != trident_layer0_pin_settings[pin_nr].dummy_file_path) {
                    ERROR(ERRNO_SIMULATION_IPC_CONFIG_FILE_PARSING, "ERRNO layer0_initialize(): pin number %d wire connection already exists, at line %d, col %d", pin_nr, line_no, line_pos);
                    return ERRNO_SIMULATION_IPC_CONFIG_FILE_PARSING;
                }

                int wscount = 0;
                //ignore any number of whitespaces between pin number and wire name
                for(; line_pos < line_strlen; line_pos++) {
                    c_char = line[line_pos];
                    if( ('0' <= c_char && '9' >= c_char) ||
                        ('a' <= c_char && 'z' >= c_char) ||
                        ('A' <= c_char && 'Z' >= c_char) ) {
                        break;
                    }
                    else {
                        wscount++;
                    }
                }
                if(line_pos >= line_strlen) {
                    ERROR(ERRNO_SIMULATION_IPC_CONFIG_FILE_PARSING, "ERRNO layer0_initialize(): unexpected eol at line %d, col %d, expected wire name", line_no, line_pos);
                    return ERRNO_SIMULATION_IPC_CONFIG_FILE_PARSING;
                }
                if(0 >= wscount) {
                    ERROR(ERRNO_SIMULATION_IPC_CONFIG_FILE_PARSING, "ERRNO layer0_initialize(): expected a whitespace between pin_nr and wire name at line %d, col %d", line_no, line_pos);
                    return ERRNO_SIMULATION_IPC_CONFIG_FILE_PARSING;
                }
                //read wire name
                int wire_name_len = 0;
                char* wire_name = (char*) malloc(1 * sizeof(char));
                wire_name[0] = '\0';

                for(; line_pos < line_strlen; line_pos++) {
                    c_char = line[line_pos];
                    if( ('0' <= c_char && '9' >= c_char) ||
                        ('a' <= c_char && 'z' >= c_char) ||
                        ('A' <= c_char && 'Z' >= c_char) ) {
                        wire_name = (char*) realloc(wire_name, ++wire_name_len);
                        sprintf(wire_name, "%s%c", wire_name, c_char);
                    }
                    else {
                        break;
                    }
                }

                // assemble full path to fifo file
                unsigned int fifo_name_strlen = fifo_dir_strlen + fifo_preamble_strlen + wire_name_len;

                if('/' == TRIDENT_SIMULATION_IPC_DUMMY_FILE_FOLDER[fifo_dir_strlen - 1]) {
                    trident_layer0_pin_settings[pin_nr].dummy_file_path = (char*) malloc((fifo_name_strlen + 1) * sizeof(char));
                    sprintf(trident_layer0_pin_settings[pin_nr].dummy_file_path, "%s%s%s", TRIDENT_SIMULATION_IPC_DUMMY_FILE_FOLDER, FIFO_NAME_PREAMBLE, wire_name);
                }
                else {
                    fifo_name_strlen++;
                    trident_layer0_pin_settings[pin_nr].dummy_file_path = (char*) malloc((fifo_name_strlen + 1) * sizeof(char));
                    sprintf(trident_layer0_pin_settings[pin_nr].dummy_file_path, "%s/%s%s", TRIDENT_SIMULATION_IPC_DUMMY_FILE_FOLDER, FIFO_NAME_PREAMBLE, wire_name);
                }

                //create dummy file if it does not exist
                if(-1 == access(trident_layer0_pin_settings[pin_nr].dummy_file_path, F_OK)) {
                    FILE* fptr = fopen(trident_layer0_pin_settings[pin_nr].dummy_file_path, "wb");
                    if(NULL != fptr) {
                        fclose(fptr);
                    }
                }

                //open shared memory for this pin
                int d_shmid;
                key_t d_shm_key = ftok(trident_layer0_pin_settings[pin_nr].dummy_file_path, 0);

                //  try to open/create share memory
                if( -1 == (d_shmid = shmget(d_shm_key, 1, IPC_CREAT | IPC_EXCL | 0666)) ) {
                    //retry in client mode
                    if( - 1 == (d_shmid = shmget(d_shm_key, 1, 0)) ) {
                        ERROR(ERRNO_SIMULATION_IPC_SHARED_MEMORY_ACCESS, "ERRNO layer0_read_from_pin(unsigned int, BIT*): unable to open shared ready state memory");
                        return ERRNO_SIMULATION_IPC_SHARED_MEMORY_ACCESS;
                    }
                }
                //  attach shared memory to this process
                if((char*)-1 == (trident_layer0_pin_settings[pin_nr].shm_ptr = (char*) shmat(d_shmid, 0, 0)) ) {
                    ERROR(ERRNO_SIMULATION_IPC_SHARED_MEMORY_ACCESS, "ERRNO layer0_read_from_pin(unsigned int, BIT*): unable to attach shared ready state memory to this process");
                    return ERRNO_SIMULATION_IPC_SHARED_MEMORY_ACCESS;
                }
            }
        }
    }
    return ERRNO_NO_ERROR;
}

ERRNO layer0_initialize_pin(unsigned int pin_nr, COMM_IO direction) {
    LOG_LAYER(TRIDENT_LAYER0_LOG);
    if(COMM_IO_IN != direction && COMM_IO_OUT != direction) {
        ERROR(ERRNO_FUNCTION_ARGUMENT_VALUES, "ERRNO layer0_initialize_pin(unsigned int, COMM_IO): PINs can only be initialized to in- or output");\
        return ERRNO_FUNCTION_ARGUMENT_VALUES;
    }
    if(TRIDENT_PIN_COUNT <= pin_nr || 0 >= pin_nr) {
        ERROR(ERRNO_PIN_NUMBER_INVALID, "ERRNO layer0_initialize_pin(unsigned int, COMM_IO): PIN number out of range");
        return ERRNO_PIN_NUMBER_INVALID;
    }
    if(COMM_IO_UNINITIALIZED != trident_layer0_pin_settings[pin_nr].direction){
        ERROR(ERRNO_PIN_RESERVED, "PIN %d already reserved", pin_nr);
        return ERRNO_PIN_RESERVED;
    }
    trident_layer0_pin_settings[pin_nr].pin_nr = pin_nr;
    trident_layer0_pin_settings[pin_nr].direction = direction;

    MESSAGE("ERRNO layer0_initialize_pin(unsigned int, COMM_IO): pin_nr=%d, fifo file=%s", pin_nr, trident_layer0_pin_settings[pin_nr].dummy_file_path);

    return ERRNO_NO_ERROR;
}

ERRNO layer0_write_to_pin(unsigned int pin_nr, BIT data) {
    LOG_LAYER(TRIDENT_LAYER0_LOG);
    if(BIT0 != data && BIT1 != data) {
        ERROR(ERRNO_FUNCTION_ARGUMENT_VALUES, "ERRNO layer0_write_to_pin(unsigned int, BIT): Binary logic dictates a bit either 0 or 1");
        return ERRNO_FUNCTION_ARGUMENT_VALUES;
    }
    if(TRIDENT_PIN_COUNT <= pin_nr || 0 >= pin_nr) {
        ERROR(ERRNO_PIN_NUMBER_INVALID, "ERRNO layer0_write_to_pin(unsigned int, BIT): PIN number out of range");
        return ERRNO_PIN_NUMBER_INVALID;
    }
    if(COMM_IO_OUT != trident_layer0_pin_settings[pin_nr].direction) {
        ERROR(ERRNO_PIN_RESERVED, "ERRNO layer0_write_to_pin(unsigned int, BIT): PIN %d is not an output pin", pin_nr);
        return ERRNO_PIN_RESERVED;
    }

    if(NULL == trident_layer0_pin_settings[pin_nr].dummy_file_path) {
        ERROR(ERRNO_PIN_INIT, "ERRNO layer0_write_to_pin(unsigned int, BIT): pin_nr=%d, fifo_file_path=null", pin_nr);
        return ERRNO_PIN_INIT;
    }
    char c_data = (char) data;

    //write data to shared memory
    char* d_shmptr = trident_layer0_pin_settings[pin_nr].shm_ptr;
    *d_shmptr = c_data;

    MESSAGE("ERRNO layer0_write_to_pin(unsigned int, BIT): Writing bit %c to pin %d", c_data, pin_nr);
    return ERRNO_NO_ERROR;
}

ERRNO layer0_read_from_pin(unsigned int pin_nr, BIT* read_buffer) {
    LOG_LAYER(TRIDENT_LAYER0_LOG);
    if(TRIDENT_PIN_COUNT <= pin_nr || 0 >= pin_nr) {
        ERROR(ERRNO_PIN_NUMBER_INVALID, "ERRNO layer0_read_from_pin(unsigned int, BIT*): PIN number out of range");
        return ERRNO_PIN_NUMBER_INVALID;
    }
    if(COMM_IO_IN != trident_layer0_pin_settings[pin_nr].direction) {
        ERROR(ERRNO_PIN_RESERVED, "ERRNO layer0_read_from_pin(unsigned int, BIT*): PIN %d is not an input pin pin", pin_nr);
        return ERRNO_PIN_RESERVED;
    }

    //get value from shared memory
    char* d_shmptr = trident_layer0_pin_settings[pin_nr].shm_ptr;
    switch(*d_shmptr) {
    case '0':   *read_buffer = BIT0;
                break;
    case '1':   *read_buffer = BIT1;
                break;
    case 0:     *read_buffer = BIT0;
                break;
    case 1:     *read_buffer = BIT1;
                break;
    default:    ERROR(ERRNO_SIMULATION_IPC_SHARED_MEMORY_VALUE, "ERRNO layer0_read_from_pin(unsigned int, BIT*): invalid value read from shared memory %c",  d_shmptr[pin_nr]);
                return ERRNO_SIMULATION_IPC_SHARED_MEMORY_VALUE;
                break;
    }

    MESSAGE("ERRNO layer0_read_from_pin(unsigned int, BIT*): Read %d from PIN %d", *read_buffer, pin_nr);

    return ERRNO_NO_ERROR;
}


ERRNO layer0_release_pin(unsigned int pin_nr) {
    LOG_LAYER(TRIDENT_LAYER0_LOG);
    if(COMM_IO_IN != trident_layer0_pin_settings[pin_nr].direction && COMM_IO_OUT != trident_layer0_pin_settings[pin_nr].direction) {
        ERROR(ERRNO_PIN_NOT_SET, "int layer0_release_pin(unsigned int): pin not set");
        return ERRNO_PIN_NOT_SET;
    }

    trident_layer0_pin_settings[pin_nr].direction = COMM_IO_UNINITIALIZED;

    return ERRNO_NO_ERROR;
}

ERRNO layer0_finalize() {
    for(int i = 0;i < TRIDENT_PIN_COUNT; ++i) {
        if(COMM_IO_IN == trident_layer0_pin_settings[i].direction || COMM_IO_OUT == trident_layer0_pin_settings[i].direction) {
            layer0_release_pin(i);
        }
        if(NULL != trident_layer0_pin_settings[i].dummy_file_path) {
            free(trident_layer0_pin_settings[i].dummy_file_path);
        }
    }

    return ERRNO_NO_ERROR;
}
