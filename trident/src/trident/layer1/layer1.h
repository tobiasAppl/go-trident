/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef LAYER1_H
#define LAYER1_H

#include "layer1_types.h"

#include <trident_config.h>
#include <trident/layer0/layer0_types.h>
#include <stdbool.h>


/**
 * @brief initialize_layer1
 * Additionally initializes layer 0
 */
ERRNO layer1_initialize();

/**
 * @brief aggregate_channel create a new channel
 * @param c_descr A CHANNEL_DESCRIPTOR depicting the new channel's settings
 * @return 0 on success, an error code if any problems occured
 */
ERRNO layer1_aggregate_channel(CHANNEL_DESCRIPTOR* channel_descriptor);

/**
 * @brief layer1_set_channel_bit_width
 * \warning "Channel must be initialized"
 * @param channel_number
 * @return 0 on success, an error code if any problems occured
 */
ERRNO layer1_set_channel_bit_width(unsigned int channel_number, unsigned int bit_width);

/**
 * @brief layer1_get_channel_bit_width
 * @param channel_number
 * @param bit_with_buffer
 * @return 0 on success, an error code otherwise
 */
ERRNO layer1_get_channel_bit_width(unsigned int channel_number, unsigned int* bit_with_buffer);

/**
 * @brief write_to_channel Send a data word via an output channel
 * @param channel_no The channel's number as returned by aggregate_channel()
 * @param data_word Pointer to the beginning of the data word to send, treated as a BIT array of size TRIDENT_BIT_WIDTH
 * @param max_wait_tries Maximum number of tries for Wait for Rx toggled, infinite if 0
 * @return 0 on success, an error code if any problems occured
 */
ERRNO layer1_write_to_channel(unsigned int channel_no, BIT* data_word, unsigned int max_wait_tries);

/**
 * @brief read_from_channel read a data word from an input channel
 * @param channel_no The output channel's number as returned by aggregate_channel
 * @param read_buffer A pointer to the beginning of a BIT array of length TRIDENT_BIT_WIDTH, input buffer
 * @param max_wait_tries Maximum number of tries for Wait for Rx toggled, infinite if 0
 * @return 0 on success, an error code if any problems occured
 */
ERRNO layer1_read_from_channel(unsigned int channel_no, BIT* read_buffer, unsigned int max_wait_tries);

/**
 * @brief layer1_get_channel_descriptor Accesses a channel descriptor for an already allocated channel
 * @param channel_no
 * @param channel_descriptor_buffer
 * @return 0
 */
ERRNO layer1_get_channel_descriptor(unsigned int channel_no, CHANNEL_DESCRIPTOR *channel_descriptor_buffer);

/**
 * @brief layer1_get_channel_descriptor_reference Accesses a channel descriptor for an already allocated channel
 * @param channel_no
 * @param channel_descriptor_reference_buffer a pointer to a buffer for the pointer to the channel desriptor
 * @return 0
 */
ERRNO layer1_get_channel_descriptor_reference(unsigned int channel_no, CHANNEL_DESCRIPTOR** channel_descriptor_reference_buffer);


/**
 * @brief layer1_get_channel_count
 * @return The number of allocated channels
 */
unsigned int layer1_get_channel_count();

/**
 * @brief layer1_reset_channel_iterator
 * Resets the channel iterator that controls which channel descriptor will be returned by layer1_get_next_channel_descriptor
 * @return 0 on success, an error specific code otherwise
 */
void layer1_reset_channel_iterator();

/**
 * @brief layer1_get_next_channel_descriptor Iterator style access to the channel descriptors
 * @return A pointer to the channel descriptor \warning "Do not modify structure fields!"
 */
CHANNEL_DESCRIPTOR* layer1_get_next_channel_descriptor();


/**
 * @brief layer1_reset_channel resets channel pin states to zero
 * @param channel_no number of the channel to reset
 * @return 0 on success, an error specific number otherwise
 */
ERRNO layer1_reset_channel_pin_states();

/**
 *  @brief finalize_layer1 clean up layer1
 *  Frees all dynamically allocated memory of layer1, closes all channels, then invokes layer0's finalization
 *  @return LAYER1_CHANNEL_IO_VALID if successful, LAYER1_CHANNEL_IO_ERROR if any problems occured
 */
ERRNO layer1_finalize();

#endif // ifndef LAYER1_H
