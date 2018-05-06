/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include <trident/layer0/layer0.h>
#include <trident_config.h>
#include <trident/util.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <math.h>
#include <time.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sched.h>

typedef struct {
    COMM_IO direction;
    int fd;
} PIN_DESCRIPTOR;

PIN_DESCRIPTOR pin_descriptors[TRIDENT_PIN_COUNT];

ERRNO layer0_initialize() {
    //NoOp
}

int layer0_finalize() {
    unsigned int pd_it;
    for(pd_it = 0; TRIDENT_PIN_COUNT > pd_it; ++pd_it) {
        pin_descriptors[pd_it].direction = COMM_IO_UNINITIALIZED;
        pin_descriptors[pd_it].fd = 0;
    }
    return 0;
}

/**
 * @brief init_pin helper function to initialize a pin
 * @param pin_nr the pin to initialize
 * @return the file descriptor of the pin's direction
 */
int init_pin(unsigned int number) {
    LOG_LAYER(TRIDENT_LAYER0_LOG);
    char vdpath[40];
    char exppath[40];
    int fd_dir;

    //try to open the pin
    snprintf(vdpath, 40, "/sys/class/gpio/gpio%d/value", number);
    fd_dir = open(vdpath, O_RDWR);
    if (0 > fd_dir) {
        //when pin not available, try to export it
        snprintf(exppath, 40, "echo %d > /sys/class/gpio/export", number);
        if(0 > system(exppath))
        {
            ERROR(ERRNO_PIN_INIT, "int init_pin(unsigned int): setupPin export pin failed");
            return ERRNO_PIN_INIT;
        }
        //and retry opening
        snprintf(vdpath, 40, "/sys/class/gpio/gpio%d/value", number);
        fd_dir = open(vdpath, O_RDWR);
        if (0 > fd_dir)
        {
            ERROR(ERRNO_PIN_INIT, "int init_pin(unsigned int): setupPin open failed");
            return ERRNO_PIN_INIT;
        }
    }

    pin_descriptors[number].fd = fd_dir;

    snprintf(vdpath, 40, "/sys/class/gpio/gpio%d/direction", number);
    fd_dir = open(vdpath, O_RDWR);
    if (0 > fd_dir) {
        ERROR(ERRNO_PIN_INIT, "int init_pin(unsigned int): setupPin open failed");
        return ERRNO_PIN_INIT;
    }
    return fd_dir;
}

int layer0_initialize_pin(unsigned int pin_number, COMM_IO direction) {
    LOG_LAYER(TRIDENT_LAYER0_LOG);
    if(COMM_IO_UNINITIALIZED != pin_descriptors[pin_number].direction) {
        ERROR(ERRNO_PIN_RESERVED, "int layer0_initialize_pin(unsigned int, COMM_IO): pin is already reserved");
        return ERRNO_PIN_RESERVED;
    }

    int fd_dir;

    if(0 <= (fd_dir = init_pin(pin_number)))
    {
        if(COMM_IO_IN == direction) {
            if (2 != write(fd_dir, "in", 2))
            {
                ERROR(ERRNO_PIN_OUT_INIT, "int layer0_initialize_pin(unsigned int, COMM_IO): setupPin write to /sys/class/gpio/gpio<d>/direction failed");
                return ERRNO_PIN_INIT;
            }
            close(fd_dir);
            pin_descriptors[pin_number].direction = COMM_IO_IN;
        }
        else if(COMM_IO_OUT == direction) {
            if (3 != write(fd_dir, "out", 3))
            {
                ERROR(ERRNO_PIN_OUT_INIT, "int layer0_initialize_pin(unsigned int, COMM_IO): setupPin write to /sys/class/gpio/gpio<d>/direction failed");
                return ERRNO_PIN_OUT_INIT;
            }
            close(fd_dir);
            pin_descriptors[pin_number].direction = COMM_IO_OUT;
        }
        else {
            ERROR(ERRNO_PIN_INIT, "int layer0_initialize_pin(unsigned int, COMM_IO): invalid pin direction");
            return ERRNO_PIN_INIT;
        }
    }
    else
    {
        ERROR(ERRNO_PIN_INIT, "int layer0_initialize_pin(unsigned int, COMM_IO): Setup pin failed");
        return ERRNO_PIN_INIT;
    }
    return ERRNO_NO_ERROR;
}

int layer0_release_pin(unsigned int pin_nr) {
    LOG_LAYER(TRIDENT_LAYER0_LOG);
    if(COMM_IO_IN != pin_descriptors[pin_nr].direction && COMM_IO_OUT != pin_descriptors[pin_nr].direction) {
        ERROR(ERRNO_PIN_NOT_SET, "int layer0_release_pin(unsigned int): pin not set");
        return ERRNO_PIN_NOT_SET;
    }

    pin_descriptors[pin_nr].direction = COMM_IO_UNINITIALIZED;
    pin_descriptors[pin_nr].fd = 0;

    return ERRNO_NO_ERROR;
}

int layer0_write_to_pin(unsigned int pin_nr, BIT data) {
    LOG_LAYER(TRIDENT_LAYER0_LOG);
    if(TRIDENT_PIN_COUNT <= pin_nr) {
        ERROR(ERRNO_PIN_NUMBER_INVALID, "layer0_write_to_pin: pin number exceeds maximum number of pins");
        return ERRNO_PIN_NUMBER_INVALID;
    }
    if(COMM_IO_UNINITIALIZED == pin_descriptors[pin_nr].direction) {
        ERROR(ERRNO_PIN_RESERVED, "layer0_write_to_pin: pin direction uninitialized");
        return ERRNO_PIN_RESERVED;
    }
    if(0 >= pin_descriptors[pin_nr].fd) {
        ERROR(ERRNO_PIN_FILE_DESCRIPTOR, "layer0_write_to_pin: invalid pin file descriptor");
        return ERRNO_PIN_FILE_DESCRIPTOR;
    }
    if(BIT0 != data && BIT1 != data) {
        ERROR(ERRNO_FUNCTION_ARGUMENT_VALUES, "layer0_write_to_pin: value is invalid");
        return ERRNO_FUNCTION_ARGUMENT_VALUES;
    }

    char value = (data & 0x1) + '0';
    if (0 > write(pin_descriptors[pin_nr].fd, &value, 1)) {
        ERROR(ERRNO_OUT_TRANSMISSION, "layer0_write_to_pin(): write to Pin %d failed\n");
        return ERRNO_OUT_TRANSMISSION;
    }

    MESSAGE("int layer0_write_to_pin(unsigned int pin_nr, BIT): writing bit %d to pin# %d", data, pin_nr);

    return ERRNO_NO_ERROR;
}

int layer0_read_from_pin(unsigned int pin_nr, BIT* read_buffer) {
    LOG_LAYER(TRIDENT_LAYER0_LOG);
    if(TRIDENT_PIN_COUNT <= pin_nr) {
        ERROR(ERRNO_PIN_NUMBER_INVALID, "layer0_read_from_pin: pin number exceeds maximum number of pins");
        return ERRNO_PIN_NUMBER_INVALID;
    }
    if(COMM_IO_UNINITIALIZED == pin_descriptors[pin_nr].direction ) {
        ERROR(ERRNO_PIN_RESERVED, "layer0_read_from_pin: pin direction uninitialized");
        return ERRNO_PIN_RESERVED;
    }
    if(0 >= pin_descriptors[pin_nr].fd) {
        ERROR(ERRNO_PIN_FILE_DESCRIPTOR, "layer0_read_from_pin(): invalid pin file descriptor");
        return ERRNO_PIN_FILE_DESCRIPTOR;
    }

    int rdval;
    lseek(pin_descriptors[pin_nr].fd, 0, SEEK_SET);
    if (0 > read(pin_descriptors[pin_nr].fd, &rdval, 1)) {
        ERROR(ERRNO_IN_TRANSMISSION, "layer0_read_from_pin: read from Pin failed\n");
        return ERRNO_IN_TRANSMISSION;
    }

    *read_buffer = (0 == ((rdval - '0') & 0x1) ? BIT0 : BIT1);

    MESSAGE("int layer0_read_from_pin(unsigned int pin_nr, BIT*): read bit %d from pin# %d", *read_buffer, pin_nr);

    return ERRNO_NO_ERROR;
}
