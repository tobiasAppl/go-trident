/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef TRIDENT_H
#define TRIDENT_H

#include <trident_config.h>
#include "errno.h"
#include "types.h"

/**
 * @brief initialize sets up the trident communication stack for operation
 * @return 0 on success, an error specific code otherwise
 */
ERRNO trident_initialize();

/**
 * @brief trident_allocate_channels allocates multiple channels
 * @param channel_c size of the CHANNEL_DESCRIPTOR array channels
 * @param channels An array of channel descriptors \warning "The CHANNEL_DESCRIPTOR.channel_nr field will be overwritten by the allocation function"
 * @return 0 on success, an error specific code otherwise
 */
ERRNO trident_allocate_channels(int channel_c, CHANNEL_DESCRIPTOR* channels);

/**
 * @brief trident_execute_communication executes writes and reads on all allocated channels
 * Opens a new POSIX thread for each channel and executes a the write/read in these channels.
 * \note "Waits for all channel threads to terminate before returning"
 * @return 0 on success, an error specific code otherwise
 */
ERRNO trident_execute_communication();

/**
 * @brief trident_set_channel_write_buffer sets the write buffer for an output channel
 * During the next exeution of the trident_execute_communication() function, the content of the write buffer will be written
 * specified output channel.
 * \warning "data_word length must coincide with channel bit width"
 * @param channel_nr
 * @param data_word
 * @return 0 on success, an error code otherwise
 */
ERRNO trident_write_to_output_channel_buffer(unsigned int channel_nr, WORD data_word);

/**
 * @brief trident_get_last_read_result
 * Accesses the last read result from an input channel
 * @param channel_nr
 * @param result_buffer A pointer to a WORD object, new memory will be allocated for the word data and the result length will be set by this function \warning "word data must be deallocated manually"
 * @return 0 on success, an error specific code otherwise
 */
ERRNO trident_get_read_result(unsigned int channel_nr, WORD* result_buffer);

/**
 * @brief finalize finalizes trident layer 2
 * Free layer2 dynamically allocated memory, invoke layer1 finalization
 * @return 0 on success or a problem specific error code otherwise
 */
ERRNO trident_finalize();

/*** RESET BARRIER FUNCTIONS ***/

/**
 * @brief layer1_reset initiates the reset process
 * Waits for a 1 on the reset pin, then resets all channels, then sets the reset response pin to 1.
 * Initializes all required pins.
 * @param reset_pin The pin to receive the reset control
 * @param reset_response_pin Pin to send the receive response
 * @param barrier_operation_function pointer a function containing the operation which should be executed during the barrier valid phase
 * @return 0 on success, an error code otherwise
 */
ERRNO trident_reset_barrier(unsigned int reset_pin, unsigned int reset_response_pin, ERRNO (*barrier_operation_function)());

/**
 * @brief layer1_reset_barrier_controller
 * Waits for all reset_response pins to become 1, then sets the reset pin to 1, and after all reset response pins became 0 again, sets the reset pin to 0
 * Initializes all required pins.
 * @param reset_pin The output pin to control the reset
 * @param reset_response_pins Input pins to receive reset responses
 * @param c_response_pin  Size of the reset_response_pins array must be >= 1
 * @return ERRNO_NO_ERROR on success, an errorcode otherwise
 */
ERRNO trident_reset_barrier_controller(unsigned int reset_pin, unsigned int* reset_response_pins, unsigned int c_response_pins);


#endif // TRIDENT_H
