/**
 * @file hash.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-09-22
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "hash.h"

#define TOP5BITS 0xF8000000
#define INITIAL_HASH 0x811C9DC5  // Better initial hash value (FNV offset basis)
#define PRIME_MULTIPLIER 0x01000193  // Larger prime multiplier (FNV prime)

/**
 * @brief calculate the hash value of a string
 * 
 * @param str       string to hash
 * @param length    length of the string
 * @return hash_t   hash value
 */
hash_t hash(const char *str, unsigned int length) {
  hash_t hash = INITIAL_HASH;  // start value
  unsigned int i;

  hash ^= length;
  for (i = 0; i < length; i++) {
    hash_t tmp = hash & TOP5BITS;
    hash = (hash ^ str[i]) * PRIME_MULTIPLIER;
    hash ^= tmp >> 27;
  }

  return hash;
}
