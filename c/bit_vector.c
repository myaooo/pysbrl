//
// Created by Ming Yao on 7/5/18.
//

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "bit_vector.h"

/* One-counting tools */
//int bit_ones[] = {0, 1, 3, 7, 15, 31, 63, 127};

static const uint8_t byte_ones[256] = {
/*   0 */ 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
/*  16 */ 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/*  32 */ 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/*  48 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/*  64 */ 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/*  80 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/*  96 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/* 112 */ 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/* 128 */ 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/* 144 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/* 160 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/* 176 */ 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/* 192 */ 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/* 208 */ 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/* 224 */ 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/* 240 */ 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

#define BYTE_MASK    0xFF

#define nbits2nwords(x) (((x) + BITS_PER_WORD - 1)/BITS_PER_WORD)

#define MIN(a, b)  (((a) <= (b)) ? (a) : (b))
#define MAX(a, b)  (((a) >= (b)) ? (a) : (b))
#define bits_in_top_word(nbits) ((nbits) ? bitvec_idx((nbits) - 1) + 1 : 0)
//#define bitmask64(nbits) ((nbits) ? (~(uint64_t)0 >> (64 - (nbits))) : ((uint64_t)0))
#define bitmask(nbits) ((nbits) ? (~(word_t)0 >> (BITS_PER_WORD - (nbits))) : ((word_t)0))

//
// Helper Functions
//

static inline void _mask_top_word(bit_vector_t *bitvec) {
    // Mask top word
    bit_size_t num_of_words = MAX(1, bitvec->n_words);
    bit_size_t bits_active = bits_in_top_word(bitvec->n_bits);
    bitvec->words[num_of_words - 1] &= bitmask(bits_active);
}

//
// Creator & Destroyer
//

// all bits set to 0
bit_vector_t *bit_vector_init(bit_size_t nbits) {
    bit_vector_t *ret = malloc(sizeof(bit_vector_t));

    ret->n_bits = nbits;
    ret->n_words = nbits2nwords(nbits);
    ret->words = (word_t *) calloc(ret->n_words, sizeof(word_t));
    if (ret->words == NULL) {
        errno = ENOMEM;
        free(ret);
        return NULL;
    }
    ret->n_ones = -1;
    return ret;
}

void bit_vector_free(bit_vector_t *vec) {
    if (vec == NULL) return;
    if (vec->words != NULL)
        free(vec->words);
    free(vec);
}

void bit_vector_dealloc(bit_vector_t *vec) {
    free(vec->words);
    memset(vec, 0, sizeof(bit_vector_t));
}

//
// Resize
//

char bit_vector_resize(bit_vector_t *bitvec, bit_size_t new_n_bits) {
    bit_size_t old_n_words = bitvec->n_words;
    bit_size_t new_n_words = nbits2nwords(new_n_bits);

    bitvec->n_bits = new_n_bits;
    bitvec->n_words = new_n_words;
    bitvec->n_ones = -1;
    bitvec->words = realloc(bitvec->words, new_n_words * sizeof(word_t));

    if (new_n_words > bitvec->n_words) {
        // Need to change the amount of memory used

        if (bitvec->words == NULL) {
            // error - could not allocate enough memory
            perror("resize realloc");
            errno = ENOMEM;
            return 0;
        }

        // Need to zero new memory
        memset(bitvec->words + old_n_words, 0, (new_n_words - old_n_words) * sizeof(word_t));

    }

    // Mask top word
    _mask_top_word(bitvec);
    return 1;
}

void bit_vector_resize_critical(bit_vector_t* bitvec, bit_size_t n_bits)
{
    bit_size_t old_n_bits = bitvec->n_bits;

    if(!bit_vector_resize(bitvec, n_bits))
    {
        fprintf(stderr, "Ran out of memory resizing [%lu -> %lu]",
                (unsigned long)old_n_bits, (unsigned long)n_bits);
        abort();
    }
}

void bit_vector_ensure_size_critical(bit_vector_t* bitvec, bit_size_t n_bits) {
    if (bitvec->n_bits < n_bits) {
        bit_vector_resize_critical(bitvec, n_bits);
    }
}

//
// Copy
//
bit_vector_t* bit_vector_clone(const bit_vector_t* src) {
    bit_vector_t* ret = bit_vector_init(src->n_bits);
    bit_vector_copy(ret, src);
    return ret;
}

void bit_vector_copy(bit_vector_t *dest, const bit_vector_t* src) {
    bit_vector_resize_critical(dest, src->n_bits);
    memmove(dest->words, src->words, src->n_words * sizeof(word_t));
}

//
// Logic operations
//

#define bit_vector_func_def_2ops(name, op) \
void name(bit_vector_t *dest, const bit_vector_t *src) { \
    assert(src->n_words <= dest->n_words);\
    for (bit_size_t i = 0; i < src->n_words; i++) { \
        op((dest->words[i]), (src->words[i])); \
    } \
    dest->n_ones = -1; \
}

#define bit_vector_func_def_3ops(name, op) \
void name(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2) { \
    assert(src1->n_words <= dest->n_words);\
    assert(src1->n_words <= src2->n_words);\
    for (bit_size_t i = 0; i < src1->n_words; i++) { \
        op((dest->words[i]), (src1->words[i]), (src2->words[i])); \
    } \
    dest->n_ones = -1; \
}

#define op_and_eq(dest, src) ((dest) &= (src))
#define op_or_eq(dest, src) ((dest) |= (src))
#define op_xor_eq(dest, src) ((dest) ^= (src))
#define op_and_eq_not(dest, src) ((dest) &= (~(src)))
#define op_or_eq_and(dest, src1, src2) ((dest) |= (src1) & (src2))
#define op_and(dest, src1, src2) ((dest) = (src1) & (src2))
#define op_or(dest, src1, src2) ((dest) = (src1) | (src2))

bit_vector_func_def_2ops(bit_vector_and_eq, op_and_eq)
bit_vector_func_def_2ops(bit_vector_or_eq, op_or_eq)
bit_vector_func_def_2ops(bit_vector_xor_eq, op_xor_eq)
bit_vector_func_def_2ops(bit_vector_and_eq_not, op_and_eq_not)
bit_vector_func_def_3ops(bit_vector_and, op_and)
bit_vector_func_def_3ops(bit_vector_or, op_or)
bit_vector_func_def_3ops(bit_vector_or_eq_and, op_or_eq_and)

void bit_vector_and_safe(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2) {
    // Ensure dst array is big enough
    bit_vector_ensure_size_critical(dest, MAX(src1->n_bits, src2->n_bits));
    bit_size_t min_words = MIN(src1->n_words, src2->n_words);

    bit_size_t i;

    for (i = 0; i < min_words; i++) {
        dest->words[i] = src1->words[i] & src2->words[i];
    }

    // Set remaining bits to zero
    if (dest->n_words > min_words)
        memset(dest->words + min_words, 0, sizeof(word_t) * (dest->n_words - min_words));
    dest->n_ones = -1;

}

void bit_vector_or_safe(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2) {
    // Ensure dst array is big enough
    bit_vector_ensure_size_critical(dest, MAX(src1->n_bits, src2->n_bits));
    bit_size_t min_words = MIN(src1->n_words, src2->n_words);
    bit_size_t max_words = MAX(src1->n_words, src2->n_words);

    bit_size_t i;

    for (i = 0; i < min_words; i++) {
        dest->words[i] = (src1->words[i]) | (src2->words[i]);
    }
    if (min_words != max_words) {
        const bit_vector_t * longer = src1->n_words > src2->n_words ? src1 : src2;
        memmove(dest->words + min_words, longer->words + min_words, (max_words - min_words) * sizeof(word_t));
    }

    // Set remaining bits to zero
    if (dest->n_words > max_words)
        memset(dest->words + max_words, 0, sizeof(word_t) * (dest->n_words - max_words));
    dest->n_ones = -1;
}

void bit_vector_xor_safe(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2) {
    // Ensure dst array is big enough
    bit_vector_ensure_size_critical(dest, MAX(src1->n_bits, src2->n_bits));
    bit_size_t min_words = MIN(src1->n_words, src2->n_words);
    bit_size_t max_words = MAX(src1->n_words, src2->n_words);

    for (bit_size_t i = 0; i < min_words; i++) {
        dest->words[i] = (src1->words[i]) ^ (src2->words[i]);
    }
    if (min_words != max_words) {
        const bit_vector_t * longer = src1->n_words > src2->n_words ? src1 : src2;
        memmove(dest->words + min_words, longer->words + min_words, (max_words - min_words) * sizeof(word_t));
    }

    // Set remaining bits to zero
    if (dest->n_words > max_words)
        memset(dest->words + max_words, 0, sizeof(word_t) * (dest->n_words - max_words));
    dest->n_ones = -1;
}

void bit_vector_not_safe(bit_vector_t *dest, const bit_vector_t *src) {
    // Ensure dst array is big enough
    bit_vector_ensure_size_critical(dest, src->n_bits);

    for (bit_size_t i = 0; i < src->n_words; i++) {
        dest->words[i] = ~(src->words[i]);
    }
    // Set remaining bits to one
    if (dest->n_words > src->n_words)
        memset(dest->words + src->n_words, BYTE_MASK, sizeof(word_t) * (dest->n_words - src->n_words));
    dest->n_ones = -1;
}

void bit_vector_nand_safe(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2) {
    bit_vector_ensure_size_critical(dest, MAX(src1->n_bits, src2->n_bits));
    bit_vector_flip_all(dest);
    dest->n_ones = -1;
}

void bit_vector_and_not_safe(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2) {
    // Ensure dst array is big enough
    bit_vector_ensure_size_critical(dest, MAX(src1->n_bits, src2->n_bits));
    bit_size_t min_words = MIN(src1->n_words, src2->n_words);

    bit_size_t i;

    for (i = 0; i < min_words; i++) {
        dest->words[i] = src1->words[i] & (~src2->words[i]);
    }

    // Set remaining bits to zero
    memset(dest->words + min_words, 0, sizeof(word_t) * (dest->n_words - min_words));
    dest->n_ones = -1;
}


//
// Set, Get
//

// set all elements of data to one
void bit_vector_set_all(bit_vector_t* bitvec)
{
    memset(bitvec->words, 0xFF, bitvec->n_words * sizeof(word_t));
    _mask_top_word(bitvec);
}

void bit_vector_flip_all(bit_vector_t* bitvec) {
    for (bit_size_t i = 0; i < bitvec->n_words; i++) {
        bitvec->words[i] ^= WORD_MAX;
    }
    _mask_top_word(bitvec);
}

//
// Statistics
//

bit_size_t bit_vector_n_ones(bit_vector_t* vec) {
    if (vec->n_ones < 0) {
        vec->n_ones = count_bit_vector_ones(vec);
    }
    return (bit_size_t) vec->n_ones;
}

bit_size_t
count_word_ones(word_t word) {
    bit_size_t count = 0;
    for (int i = 0; (unsigned) i < sizeof(word_t); i++) {
        count += byte_ones[word & BYTE_MASK];
        word >>= 8;
    }
    return (count);
}

bit_size_t count_bit_vector_ones(const bit_vector_t *vec) {
    bit_size_t count = 0;
    for (bit_size_t i = 0; i < vec->n_words; i++) {
        count += count_word_ones(vec->words[i]);
    }
    return count;
}

int bit_vector_first_set(bit_vector_t * bitvec, int start_pos) {
    for (bit_size_t i = start_pos; i < bitvec->n_bits; i++) {
        if (bit_vector_get(bitvec->words, i) != 0)
            return i;
    }
    return -1;
}


void
bit_vector_print(const bit_vector_t *v) {
    for (bit_size_t i = 0; i < v->n_words; i++)
        printf("0x%llx ", (unsigned long long)v->words[i]);
    printf("\n");
}

// Construct a BIT_ARRAY from a substring with given on and off characters.
bit_vector_t* bit_vector_from_str(const char *str)
{
    if (str == NULL) return NULL;
    unsigned long len = strlen(str);
    bit_vector_t *bits = bit_vector_init((bit_size_t) len);
    if (bits == NULL) return NULL;

    // BitArray region is now all 0s -- just set the 1s
    bit_size_t i = 0;
    const char *p;

    for (p = str; len-- > 0; p++) {
        switch (*p) {
            case '0':
                i++;
                break;
            case '1':
                bitvec_set(bits->words, i);
                i++;
                break;
            default:
                break;
        }
    }
    bit_vector_resize_critical(bits, i);
    return bits;
}

// Construct a BIT_ARRAY from a substring with given on and off characters.
bit_vector_t* bit_vector_from_bytes(const char *bytes, bit_size_t len)
{
    bit_vector_t *bits = bit_vector_init((bit_size_t) len);
    if (bits == NULL) return NULL;

    // BitArray region is now all 0s -- just set the 1s

    for (bit_size_t i = 0; i < len; i++) {
        if (bytes[i])
            bitvec_set(bits->words, i);
    }
    return bits;
}