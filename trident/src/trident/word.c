/**
 * Author: Tobias Appl <tobias.appl@gmail.com>
 */

#include "word.h"
#include <stdlib.h>
#include <math.h>

bool word_equals(WORD w0, WORD w1) {
    if(w0.word_length != w1.word_length) {
        return false;
    }

    bool content_equal = true;
    for(unsigned int i = 0; w0.word_length > i; ++i) {
        content_equal &= (w0.word_data[i] == w1.word_data[i]);
    }
    return content_equal;
}

char* word_to_string(WORD word) {
    char* w_str = (char*) calloc(word.word_length + 1, sizeof(char));
    for(unsigned int i = 0; word.word_length > i; ++i) {
        w_str[i] = word.word_data[i] + '0';
    }
    w_str[word.word_length] = '\0';
    return w_str;
}


void word_clone(WORD* src, WORD* target) {
    if(NULL == src || NULL == target ||
       NULL == src->word_data || 0 == src->word_length) {
        return;
    }

    if(NULL != target->word_data) {
        free(target->word_data);
    }

    target->word_length = src->word_length;
    target->word_data = (BIT*) calloc(src->word_length, sizeof(BIT));

    for(unsigned int i = 0; i < src->word_length; ++i) {
        target->word_data[i] = src->word_data[i];
    }
}

void word_construct(WORD *word) {
    if(NULL == word) {
        return;
    }
    word->word_data = NULL;
    word->word_length = 0;
}

void word_destruct(WORD *word) {
    if(NULL == word) {
        return;
    }
    if(NULL != word->word_data) {
        free(word->word_data);
        word->word_data = NULL;
    }
    word->word_length = 0;
}

void word_from_int(WORD *word, unsigned int intval) {
    if(NULL == word) {
        return;
    }
    if(NULL != word->word_data) {
        free(word->word_data);
    }
    word->word_data = (BIT*) malloc(word->word_length * sizeof(BIT));

    unsigned int shiftval = intval;
    for(unsigned int i = 0; i < word->word_length; ++i) {
        word->word_data[i] = shiftval & 1;
        shiftval >>= 1;
    }
}

ERRNO word_to_int(WORD *word, int* int_buffer) {
    if(NULL == word) {
        return ERRNO_OBJECT_NOT_ALLOCATED;
    }

    if(NULL == word->word_data || 0 == word->word_length) {
        *int_buffer = 0;
        return ERRNO_NO_ERROR;
    }

    for(unsigned int i = 0; i < word->word_length; ++i) {
        *int_buffer += (int) (word->word_data[i] * pow((double) 2, (double) i));
    }

    return ERRNO_NO_ERROR;
}
