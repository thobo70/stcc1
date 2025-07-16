/**
 * @file tstore.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief Token store header file
 * @version 0.1
 * @date 2024-09-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef TSTORE_H  // NOLINT
#define TSTORE_H

#include "../lexer/ctoken.h"


/**
 * @brief Initialize the token store.
 * 
 * @param filename The name of the file to store tokens.
 * @return int 0 on success, non-zero on failure.
 */
int tstore_init(const char *filename);

/**
 * @brief Open an existing token store file.
 * 
 * @param filename The name of the file to open.
 * @return int 0 on success, non-zero on failure.
 */
int tstore_open(const char *filename);

/**
 * @brief Close the token store file.
 * 
 * @return void
 */
void tstore_close(void);

/**
 * @brief Add a token to the store.
 * 
 * @param token The token to add.
 * @return TokenIdx_t The index of the added token, or 0 on failure.
 */
TokenIdx_t tstore_add(Token_t *token);

/**
 * @brief Get a token from the store by index.
 * 
 * @param idx The index of the token to retrieve.
 * @return Token_t The token at the specified index.
 */
Token_t tstore_get(TokenIdx_t idx);

/**
 * @brief Get the next token from the store.
 * 
 * @return Token_t The next token in the store.
 */
Token_t tstore_next(void);

/**
 * @brief Set the current index in the token store.
 * 
 * @param idx The index to set.
 * @return int 0 on success, non-zero on failure.
 */
int tstore_setidx(TokenIdx_t idx);

/**
 * @brief Get the current index in the token store.
 * 
 * @return TokenIdx_t The current index in the token store.
 */
TokenIdx_t tstore_getidx(void);

#endif  // TSTORE_H  // NOLINT