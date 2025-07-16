/**
 * @file astore.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-09-08
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef ASTORE_H  // NOLINT
#define ASTORE_H

#include "../ast/ast_types.h"

/**
 * @brief Initialize the abstract syntax tree store.
 *
 * @param filename The name of the file to store the AST.
 * @return int 0 on success,
                     non-zero on failure.
 */
int astore_init(const char *filename);

/**
 * @brief Open an existing abstract syntax tree store file.
 *
 * @param filename The name of the file to open.
 * @return int 0 on success,
                     non-zero on failure.
 */
int astore_open(const char *filename);

/**
 * @brief Close the abstract syntax tree store file.
 *
 * @return void
 */
void astore_close(void);

/**
 * @brief Add a node to the abstract syntax tree store.
 *
 * @param node The node to add.
 * @return ASTNodeID_t The ID of the added node,
                     or 0 on failure.
 */
ASTNodeIdx_t astore_add(ASTNode *node);

/**
 * @brief Update a node in the abstract syntax tree store.
 *
 * @param idx The ID of the node to update.
 * @param node The updated node.
 * @return ASTNodeID_t The ID of the updated node,
                     or 0 on failure.
 */
ASTNodeIdx_t astore_update(ASTNodeIdx_t idx,
                     ASTNode *node);

/**
 * @brief Get a node from the abstract syntax tree store by ID.
 *
 * @param idx The ID of the node to retrieve.
 * @return ASTNode The node at the specified ID.
 */
ASTNode astore_get(ASTNodeIdx_t idx);

/**
 * @brief Set the current index in the abstract syntax tree store.
 *
 * @param idx The index to set.
 * @return int 0 on success,
                     non-zero on failure.
 */
int astore_setidx(ASTNodeIdx_t idx);

/**
 * @brief Get the current index in the abstract syntax tree store.
 *
 * @return ASTNodeID_t The current index in the abstract syntax tree store.
 */
ASTNodeIdx_t astore_getidx(void);




#endif  // ASTORE_H  // NOLINT