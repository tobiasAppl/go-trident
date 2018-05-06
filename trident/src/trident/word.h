/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#ifndef WORD_H
#define WORD_H

#include <stdbool.h>
#include "layer0/layer0_types.h"
#include "errno.h"

typedef struct {
    BIT* word_data;
    unsigned int word_length;
} WORD;

/**
 * @brief equal compares two words for equality
 * Compares content length, if equal, also compares data payload
 * @param w0
 * @param w1
 * @return true if w0 == w1, false otherwise
 */
bool word_equals(WORD w0, WORD w1);

/**
 * @brief to_string Creates a c string version on the word
 * @param word
 * @return A pointer to a heap allocated c string with elements word.word_length + 1
 */
char* word_to_string(WORD word);

/**
 * @brief word_clone
 * Clones the content from one word into another word
 * @param src Pointer to a word that should be copied
 * @param target Pointer to word to copy into \warning "This Word's content will be deleted"
 */
void word_clone(WORD* src, WORD* target);

/**
 * @brief word_construct
 * Initializes a word with default values
 * @param word A pointer to a word
 */
void word_construct(WORD* word);

/**
 * @brief word_destruct
 * Frees all heap allocated space in a word and resets it to initial values;
 * @param word A pointer to a word
 */
void word_destruct(WORD* word);

/**
 * @brief word_to_int
 * @param word
 * @return
 */
ERRNO word_to_int(WORD* word, int* int_buffer);

/**
 * @brief word_from_int
 * Constructs word bit vector content from an integer value
 * \warning "Word length must be defined"
 * @param word
 * @param intval
 */
void word_from_int(WORD* word, unsigned int intval);

#endif // WORD_H

