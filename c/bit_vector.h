//
// Created by Ming Yao on 7/5/18.
//

#ifndef SBRL_BIT_VECTOR_H
#define SBRL_BIT_VECTOR_H
#include <stdint.h>
/*
 * Define types for bit vectors.
 */
typedef uint64_t word_t;
typedef uint32_t bit_size_t;

//typedef word_t *VECTOR;

#define BITS_PER_WORD (sizeof(word_t) * 8)
#define WORD_MAX (~(word_t)0)

typedef struct bit_vector {
    bit_size_t n_bits;
    bit_size_t n_words;
    int n_ones;
    word_t *words;
} bit_vector_t;


// Functions for bit vector

// Creators and Destroyers
bit_vector_t * bit_vector_init(bit_size_t nbits);
void bit_vector_free(bit_vector_t *vec);
void bit_vector_dealloc(bit_vector_t *vec);

// Resize

// Change the size of a bit array. Enlarging an array will add zeros
// to the end of it. Returns 1 on success, 0 on failure (e.g. not enough memory)
char bit_vector_resize(bit_vector_t* bitarr, bit_size_t new_num_of_bits);
void bit_vector_resize_critical(bit_vector_t* bitvec, bit_size_t n_bits);
void bit_vector_ensure_size_critical(bit_vector_t* bitvec, bit_size_t n_bits);

// Copy
bit_vector_t* bit_vector_clone(const bit_vector_t* bitarr);
void bit_vector_copy(bit_vector_t *dest, const bit_vector_t* src);

// Print
void bit_vector_print(const bit_vector_t *vec);
bit_vector_t* bit_vector_from_str(const char *str);
bit_vector_t* bit_vector_from_bytes(const char *bytes, bit_size_t len);

// Logic operations

void bit_vector_and_eq(bit_vector_t *dest, const bit_vector_t *src);
void bit_vector_or_eq(bit_vector_t *dest, const bit_vector_t *src);
void bit_vector_xor_eq(bit_vector_t *dest, const bit_vector_t *src);
void bit_vector_and(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2);
void bit_vector_or(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2);
void bit_vector_and_eq_not(bit_vector_t *dest, const bit_vector_t *src);
void bit_vector_or_eq_and(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2);

void bit_vector_and_safe(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2);
void bit_vector_or_safe(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2);
void bit_vector_xor_safe(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2);
void bit_vector_not_safe(bit_vector_t *dest, const bit_vector_t *src);
void bit_vector_nand_safe(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2);
void bit_vector_and_not_safe(bit_vector_t *dest, const bit_vector_t *src1, const bit_vector_t *src2);

// Set bits
void bit_vector_set_all(bit_vector_t* bitvec);
void bit_vector_flip_all(bit_vector_t* bitvec);

#define bitvec_word(pos) ((pos) / (BITS_PER_WORD))
#define bitvec_idx(pos) ((pos) % (BITS_PER_WORD))
#define bitvec_op(func, arr, pos)      func(arr, bitvec_word(pos), bitvec_idx(pos))

#define _TYPESHIFT(arr,word,shift) \
        ((__typeof(*(arr)))((__typeof(*(arr)))(word) << (shift)))
#define bitvec2_set(arr,wrd,idx)     ((arr)[wrd] |=  _TYPESHIFT(arr,1,idx))
#define bitvec_set(arr,pos)     bitvec_op(bitvec2_set, arr, pos)

// Vector statistics
bit_size_t bit_vector_n_ones(bit_vector_t* vec);
bit_size_t count_word_ones(word_t word);
bit_size_t count_bit_vector_ones(const bit_vector_t *vec);

// Find


#define bit_vector_get(vec, pos) \
    (((vec)[bitvec_word((pos))] >> bitvec_idx((pos))) & 0x1)

int bit_vector_first_set(bit_vector_t * bitvec, int start_pos);

#endif //SBRL_BIT_VECTOR_H
