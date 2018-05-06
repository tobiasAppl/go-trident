/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef ERROR_H
#define ERROR_H

#include<stdarg.h>
#include <trident/util.h>

/**
 * @brief error function
 * Processes error message and exits program
 * @warning Implemented in target package
 * @param message Message string
 * @param errno Error related number, defined in this file
 */
void error(int trident_log_layer, ERRNO errno, const char* msg_format, ...);

#endif // ERROR_H
