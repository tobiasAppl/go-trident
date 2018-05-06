/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef LAYER0_H
#define LAYER0_H

#include <trident/util.h>

#include "layer0_types.h"

/**
 * @brief layer0_initialize
 * Initializes all required data for layer 0
 * @return ERRNO_NO_ERROR on success, an error code if something went wrong
 */
ERRNO layer0_initialize();

/**
 * @brief initialize_pin
 * @param pin
 * @param direction
 * @return 0 if everything went ok, -1 if not
 */
ERRNO layer0_initialize_pin(unsigned int pin_nr, COMM_IO direction);

/**
 * @brief layer0_release_pin
 * @param pin_nr
 * @return 0 if the release was successful
 */
ERRNO layer0_release_pin(unsigned int pin_nr);

/**
 * @brief write_to_pin
 * @param pin_nr
 * @param data
 * @return 0 if everything went ok, -1 if not
 */
ERRNO layer0_write_to_pin(unsigned int pin_nr, BIT data);

/**
 * @brief read_from_pin
 * @param pin_nr
 * @param read_buffer
 * @return The bit read from the pin
 */
ERRNO layer0_read_from_pin(unsigned int pin_nr, BIT* read_buffer);

/**
 * @brief finalize_layer0
 * Free dynamically allocated memory, etc.
 * @return -1 if any errors occured, 0 if successful
 */
ERRNO layer0_finalize();


#endif // ifndef LAYER0_H
