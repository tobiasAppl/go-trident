/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include<stdarg.h>

/**
 * @brief message logger
 * Processes a log message
 * @warning Implemented in target package
 * @param message The message to be logged
 */
void message(int trident_log_layer, const char* msg_format, ...);

#endif // MESSAGE_H
