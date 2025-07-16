/**
 * @file hash.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-01
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef _HASH_H  // NOLINT
#define _HASH_H

typedef unsigned long hash_t;  // NOLINT

hash_t hash(const char *str,
                     unsigned int length);


#endif  // _HASH_H  // NOLINT
