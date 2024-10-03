/**
 * @file cc0.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-09-01
 * 
 * @copyright Copyright (c) 2024
 * 
 * This file implements a lexical analyzer (lexer) that processes an input C source file
 * to identify various tokens such as operators, keywords, literals, and identifiers.
 * 
 * The lexer reads the input file line by line, using helper functions to match patterns
 * for different token types. Identified tokens are stored for further processing.
 * 
 * ## Key Components
 * 
 * - **Macros and Type Definitions**: Simplify token handling and identification.
 * - **Token Type Arrays**: Define known operators and keywords.
 * - **Global Variables**: Track the current file position and line number.
 * - **Token Identification Functions**: Match input against known token patterns.
 * - **Escape Sequence Processing**: Handle escape sequences in literals.
 * - **Directive Parsing**: Process preprocessor directives.
 * - **Token Extraction**: Read and identify tokens from the input file.
 * - **Token Printing**: Output token details for debugging.
 * 
 * ## Main Function
 * 
 * The main function initializes the string and token stores, processes the input file
 * to extract tokens, and prints each token. It handles cleanup before exiting.
 * 
 * This lexer is designed to facilitate the initial stages of a compiler or interpreter
 * by converting source code into a stream of tokens.
 * 
 * @startuml
 * !define RECTANGLE class
 * 
 * RECTANGLE cc0 {
 *   +main(argc: int, argv: char*[]): int
 *   +nextToken(fp: FILE*): Token_t*
 *   +printToken(token: Token_t*): void
 *   +parse_directive(buf: char*): void
 *   +process_esc(c: int): int
 *   +is_operator(buf: char*): TokenType_t*
 *   +is_keyword(buf: char*): TokenType_t*
 *   +is_litnumber(lexeme: char*): TokenType_t*
 *   +is_litstr(buf: char*, delim: char, id: TokenID_t): TokenType_t*
 *   +is_id(buf: char*): TokenType_t*
 * }
 * 
 * RECTANGLE StringStore {
 *   +sstore_init(filename: char*): int
 *   +sstore_close(): void
 *   +sstore_str(str: char*, len: int): sstore_pos_t
 * }
 * 
 * RECTANGLE TokenStore {
 *   +tstore_init(filename: char*): int
 *   +tstore_close(): void
 *   +tstore_add(token: Token_t*): void
 * }
 * 
 * cc0 --> StringStore : uses
 * cc0 --> TokenStore : uses
 * 
 * @enduml
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "sstore.h"
#include "tstore.h"

#define TOKEN_TYPE_NAME(t)  t, #t
#define SET_LEXEME_LEN(l)  l, (sizeof(l) - 1)
#define isid(c)  (isalnum(c) || (c) == '_')

typedef struct TokenType_t {
  const char *lexeme;
  int len;
  TokenID_t id;
  const char *name;
  sstore_pos_t sstpos;
} TokenType_t;

TokenType_t operatorTypes[] = {
  {SET_LEXEME_LEN("..."), TOKEN_TYPE_NAME(T_ELLIPSIS), 0},
  {SET_LEXEME_LEN("<<="), TOKEN_TYPE_NAME(T_LSHIFTEQ), 0},
  {SET_LEXEME_LEN(">>="), TOKEN_TYPE_NAME(T_RSHIFTEQ), 0},
  {SET_LEXEME_LEN("=="), TOKEN_TYPE_NAME(T_EQ), 0},
  {SET_LEXEME_LEN("!="), TOKEN_TYPE_NAME(T_NEQ), 0},
  {SET_LEXEME_LEN("<="), TOKEN_TYPE_NAME(T_LTE), 0},
  {SET_LEXEME_LEN(">="), TOKEN_TYPE_NAME(T_GTE), 0},
  {SET_LEXEME_LEN("&&"), TOKEN_TYPE_NAME(T_LOGAND), 0},
  {SET_LEXEME_LEN("||"), TOKEN_TYPE_NAME(T_LOGOR), 0},
  {SET_LEXEME_LEN("++"), TOKEN_TYPE_NAME(T_INC), 0},
  {SET_LEXEME_LEN("--"), TOKEN_TYPE_NAME(T_DEC), 0},
  {SET_LEXEME_LEN("->"), TOKEN_TYPE_NAME(T_ARROW), 0},
  {SET_LEXEME_LEN("<<"), TOKEN_TYPE_NAME(T_LSHIFT), 0},
  {SET_LEXEME_LEN(">>"), TOKEN_TYPE_NAME(T_RSHIFT), 0},
  {SET_LEXEME_LEN("&="), TOKEN_TYPE_NAME(T_ANDEQ), 0},
  {SET_LEXEME_LEN("|="), TOKEN_TYPE_NAME(T_OREQ), 0},
  {SET_LEXEME_LEN("^="), TOKEN_TYPE_NAME(T_XOREQ), 0},
  {SET_LEXEME_LEN("+="), TOKEN_TYPE_NAME(T_PLUSEQ), 0},
  {SET_LEXEME_LEN("-="), TOKEN_TYPE_NAME(T_MINUSEQ), 0},
  {SET_LEXEME_LEN("*="), TOKEN_TYPE_NAME(T_STAREQ), 0},
  {SET_LEXEME_LEN("/="), TOKEN_TYPE_NAME(T_SLASHEQ), 0},
  {SET_LEXEME_LEN("%="), TOKEN_TYPE_NAME(T_PERCENTEQ), 0},
  {SET_LEXEME_LEN("+"), TOKEN_TYPE_NAME(T_PLUS), 0},
  {SET_LEXEME_LEN("-"), TOKEN_TYPE_NAME(T_MINUS), 0},
  {SET_LEXEME_LEN("*"), TOKEN_TYPE_NAME(T_STAR), 0},
  {SET_LEXEME_LEN("/"), TOKEN_TYPE_NAME(T_SLASH), 0},
  {SET_LEXEME_LEN("%"), TOKEN_TYPE_NAME(T_PERCENT), 0},
  {SET_LEXEME_LEN("="), TOKEN_TYPE_NAME(T_ASSIGN), 0},
  {SET_LEXEME_LEN("<"), TOKEN_TYPE_NAME(T_LESS), 0},
  {SET_LEXEME_LEN(">"), TOKEN_TYPE_NAME(T_GREATER), 0},
  {SET_LEXEME_LEN("&"), TOKEN_TYPE_NAME(T_AMPERSAND), 0},
  {SET_LEXEME_LEN("|"), TOKEN_TYPE_NAME(T_PIPE), 0},
  {SET_LEXEME_LEN("^"), TOKEN_TYPE_NAME(T_CARET), 0},
  {SET_LEXEME_LEN("~"), TOKEN_TYPE_NAME(T_TILDE), 0},
  {SET_LEXEME_LEN("!"), TOKEN_TYPE_NAME(T_EXCLAMATION), 0},
  {SET_LEXEME_LEN("?"), TOKEN_TYPE_NAME(T_QUESTION), 0},
  {SET_LEXEME_LEN(":"), TOKEN_TYPE_NAME(T_COLON), 0},
  {SET_LEXEME_LEN("("), TOKEN_TYPE_NAME(T_LPAREN), 0},
  {SET_LEXEME_LEN(")"), TOKEN_TYPE_NAME(T_RPAREN), 0},
  {SET_LEXEME_LEN("{"), TOKEN_TYPE_NAME(T_LBRACE), 0},
  {SET_LEXEME_LEN("}"), TOKEN_TYPE_NAME(T_RBRACE), 0},
  {SET_LEXEME_LEN("["), TOKEN_TYPE_NAME(T_LBRACKET), 0},
  {SET_LEXEME_LEN("]"), TOKEN_TYPE_NAME(T_RBRACKET), 0},
  {SET_LEXEME_LEN(","), TOKEN_TYPE_NAME(T_COMMA), 0},
  {SET_LEXEME_LEN("."), TOKEN_TYPE_NAME(T_DOT), 0},
  {SET_LEXEME_LEN(";"), TOKEN_TYPE_NAME(T_SEMICOLON), 0},
  {NULL, 0, TOKEN_TYPE_NAME(T_UNKNOWN), 0}  // Sentinel value to mark the end of the array
};

TokenType_t keywordTypes[] = {
  {SET_LEXEME_LEN("int"), TOKEN_TYPE_NAME(T_INT), 0},
  {SET_LEXEME_LEN("long"), TOKEN_TYPE_NAME(T_LONG), 0},
  {SET_LEXEME_LEN("short"), TOKEN_TYPE_NAME(T_SHORT), 0},
  {SET_LEXEME_LEN("float"), TOKEN_TYPE_NAME(T_FLOAT), 0},
  {SET_LEXEME_LEN("double"), TOKEN_TYPE_NAME(T_DOUBLE), 0},
  {SET_LEXEME_LEN("char"), TOKEN_TYPE_NAME(T_CHAR), 0},
  {SET_LEXEME_LEN("void"), TOKEN_TYPE_NAME(T_VOID), 0},
  {SET_LEXEME_LEN("return"), TOKEN_TYPE_NAME(T_RETURN), 0},
  {SET_LEXEME_LEN("if"), TOKEN_TYPE_NAME(T_IF), 0},
  {SET_LEXEME_LEN("else"), TOKEN_TYPE_NAME(T_ELSE), 0},
  {SET_LEXEME_LEN("while"), TOKEN_TYPE_NAME(T_WHILE), 0},
  {SET_LEXEME_LEN("for"), TOKEN_TYPE_NAME(T_FOR), 0},
  {SET_LEXEME_LEN("do"), TOKEN_TYPE_NAME(T_DO), 0},
  {SET_LEXEME_LEN("switch"), TOKEN_TYPE_NAME(T_SWITCH), 0},
  {SET_LEXEME_LEN("case"), TOKEN_TYPE_NAME(T_CASE), 0},
  {SET_LEXEME_LEN("default"), TOKEN_TYPE_NAME(T_DEFAULT), 0},
  {SET_LEXEME_LEN("break"), TOKEN_TYPE_NAME(T_BREAK), 0},
  {SET_LEXEME_LEN("continue"), TOKEN_TYPE_NAME(T_CONTINUE), 0},
  {SET_LEXEME_LEN("goto"), TOKEN_TYPE_NAME(T_GOTO), 0},
  {SET_LEXEME_LEN("sizeof"), TOKEN_TYPE_NAME(T_SIZEOF), 0},
  {SET_LEXEME_LEN("typedef"), TOKEN_TYPE_NAME(T_TYPEDEF), 0},
  {SET_LEXEME_LEN("extern"), TOKEN_TYPE_NAME(T_EXTERN), 0},
  {SET_LEXEME_LEN("static"), TOKEN_TYPE_NAME(T_STATIC), 0},
  {SET_LEXEME_LEN("auto"), TOKEN_TYPE_NAME(T_AUTO), 0},
  {SET_LEXEME_LEN("register"), TOKEN_TYPE_NAME(T_REGISTER), 0},
  {SET_LEXEME_LEN("const"), TOKEN_TYPE_NAME(T_CONST), 0},
  {SET_LEXEME_LEN("volatile"), TOKEN_TYPE_NAME(T_VOLATILE), 0},
  {SET_LEXEME_LEN("signed"), TOKEN_TYPE_NAME(T_SIGNED), 0},
  {SET_LEXEME_LEN("unsigned"), TOKEN_TYPE_NAME(T_UNSIGNED), 0},
  {SET_LEXEME_LEN("struct"), TOKEN_TYPE_NAME(T_STRUCT), 0},
  {SET_LEXEME_LEN("union"), TOKEN_TYPE_NAME(T_UNION), 0},
  {SET_LEXEME_LEN("enum"), TOKEN_TYPE_NAME(T_ENUM), 0},
  {SET_LEXEME_LEN("short"), TOKEN_TYPE_NAME(T_SHORT), 0},
  {NULL, 0, TOKEN_TYPE_NAME(T_UNKNOWN), 0}  // Sentinel value to mark the end of the array
};

TokenType_t otherTypes;

sstore_pos_t currfilepos = 0;
unsigned int currline = 0;

/**
 * @brief Identifies if the given buffer contains an operator.
 * 
 * This function checks if the provided buffer matches any known operator lexeme.
 * If a match is found, it returns a pointer to the corresponding TokenType_t structure.
 * 
 * @param buf The input string to be checked.
 * @return TokenType_t* A pointer to the TokenType_t structure representing the operator, or NULL if not an operator.
 */
TokenType_t *is_operator(char *buf) {
  for (int i = 0; operatorTypes[i].lexeme != NULL; i++) {
    if (strncmp(buf, operatorTypes[i].lexeme, operatorTypes[i].len) == 0) {
      if (operatorTypes[i].sstpos == 0) {
        operatorTypes[i].sstpos = sstore_str(operatorTypes[i].lexeme, operatorTypes[i].len);
      }
      return &operatorTypes[i];
    }
  }
  return NULL;
}



/**
 * @brief Identifies if the given buffer contains a keyword.
 * 
 * This function checks if the provided buffer matches any known keyword lexeme.
 * If a match is found and it's not a prefix of an identifier, it returns a pointer
 * to the corresponding TokenType_t structure.
 * 
 * @param buf The input string to be checked.
 * @return TokenType_t* A pointer to the TokenType_t structure representing the keyword, or NULL if not a keyword.
 */
TokenType_t *is_keyword(char *buf) {
  for (int i = 0; keywordTypes[i].lexeme != NULL; i++) {
    if (strncmp(buf, keywordTypes[i].lexeme, keywordTypes[i].len) == 0) {
      if (keywordTypes[i].sstpos == 0) {
        keywordTypes[i].sstpos = sstore_str(keywordTypes[i].lexeme, keywordTypes[i].len);
      }
      if (isid(buf[keywordTypes[i].len])) {
        break;  // Check if the keyword is a prefix of an identifier, e.g. "intx"
      }
      return &keywordTypes[i];
    }
  }
  return NULL;
}



/**
 * @brief Identifies if the given lexeme is a numeric literal.
 * 
 * This function checks if the provided lexeme represents a numeric literal.
 * It determines whether the number is an integer or a floating-point number.
 * 
 * @param lexeme The input string to be checked.
 * @return TokenType_t* A pointer to the TokenType_t structure representing the numeric literal, or NULL if not a numeric literal.
 */
TokenType_t *is_litnumber(char *lexeme) {
  int i = 0;
  int dot = 0;    // Flag for decimal point, number must be a float
  int e = 0;      // Flag for exponent, number must be a float
  int isint = 0;  // Flag for integer, number must be an integer

  // leading sign is handled separately
  if (lexeme[i] == '0') {
    ++i;
    if (lexeme[i] == 'x' || lexeme[i] == 'X') {   // Hexadecimal
      ++i;
      isint = 1;
    } else if (isdigit(lexeme[i])) {  // Octal
      ++i;
      isint = 1;
    }
  }
  while (lexeme[i] != '\0') {
    if (isdigit(lexeme[i])) {
      i++;
    } else if (lexeme[i] == '.' && !dot && !e && !isint) {
      i++;
      dot = 1;
      if (!isdigit(lexeme[i])) {
        return NULL;
      }
    } else if ((lexeme[i] == 'e' || lexeme[i] == 'E') && !e && !isint) {
      i++;
      e = 1;
      if (lexeme[i] == '+' || lexeme[i] == '-') {
        i++;
      }
      if (!isdigit(lexeme[i])) {
        return NULL;
      }
    } else {
      if (lexeme[i] != 'e' && lexeme[i] != 'E' && lexeme[i] != '.')
        break;
      return NULL;
    }
  }
  if (i == 0) {
    return NULL;
  }
  otherTypes.lexeme = NULL;
  otherTypes.len = i;
  otherTypes.sstpos = sstore_str(lexeme, i);
  if (dot || e) {
    otherTypes.id = T_LITFLOAT;
  } else {
    otherTypes.id = T_LITINT;
  }
  return &otherTypes;
}



/**
 * @brief Identifies if the given buffer contains a string or character literal.
 * 
 * This function checks if the provided buffer represents a string or character literal.
 * It handles escape sequences within the literals.
 * 
 * @param buf The input string to be checked.
 * @param delim The delimiter character (either '"' for strings or '\'' for characters).
 * @param id The token ID to be assigned if a match is found.
 * @return TokenType_t* A pointer to the TokenType_t structure representing the literal, or NULL if not a literal.
 */
TokenType_t *is_litstr(char *buf, char delim, TokenID_t id) {
  if (buf[0] == delim) {
    buf++;
    int i = 0;
    while (buf[i] != '\0') {
      if (buf[i] == delim) {
        if (buf[i - 1] != '\\') {  // Check if the quote is escaped
          break;
        } else {
          if (i > 0 && buf[i - 2] == '\\') {  // Check if the backslash is escaped
            break;
          }
        }
      }
      i++;
    }
    if (buf[i] == delim) {
      otherTypes.lexeme = NULL;
      otherTypes.len = i + 2;  // Include the quotes
      otherTypes.sstpos = sstore_str(buf, i);
      otherTypes.id = id;
      return &otherTypes;
    }
  }
  return NULL;
}



/**
 * @brief Identifies if the given buffer contains an identifier.
 * 
 * This function checks if the provided buffer represents an identifier.
 * If a match is found, it returns a pointer to the corresponding TokenType_t structure.
 * 
 * @param buf The input string to be checked.
 * @return TokenType_t* A pointer to the TokenType_t structure representing the identifier, or NULL if not an identifier.
 */
TokenType_t *is_id(char *buf) {
  if (isalpha(buf[0]) || buf[0] == '_') {
    int i = 1;
    while (isalnum(buf[i]) || buf[i] == '_') {
      i++;
    }
    otherTypes.lexeme = NULL;
    otherTypes.len = i;
    otherTypes.sstpos = sstore_str(buf, i);
    otherTypes.id = T_ID;
    return &otherTypes;
  }
  return NULL;
}



/**
 * @brief Processes escape sequences in character and string literals.
 * 
 * This function converts escape sequences (e.g., '\n', '\t') into their corresponding
 * character values.
 * 
 * @param c The escape sequence character to be processed.
 * @return int The corresponding character value of the escape sequence.
 */
int process_esc(int c) {
  switch (c) {
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    case '\\': return '\\';
    case '\?': return '\?';
    case '0': return 0;
  }
  return c | 0x100;
}


/**
 * @brief Parses a preprocessor directive for line number and file name.
 * 
 * This function processes preprocessor directives in the format `# <line> "<file>"`.
 * It updates the current file position and line number accordingly.
 * 
 * @param buf The input string containing the preprocessor directive.
 */
void parse_directive(char *buf) {
  unsigned int line = 0;
  char *file = NULL;
  buf += 2;
  while (*buf != '\0') {
    if (isdigit(*buf)) {
      line = line * 10 + (*buf - '0');
    } else {
      break;
    }
    buf++;
  }
  while (*buf != '\0') {
    if (*buf == '"') {
      buf++;
      file = buf;
      break;
    }
    buf++;
  }
  while (*buf != '\0') {
    if (*buf == '"') {
      *buf = '\0';
      break;
    }
    buf++;
  }
  if (file == NULL) {
    return;
  }
  currfilepos = sstore_str(file, strlen(file));
  currline = line;
}


/**
 * @brief Extracts the next token from the input file.
 * 
 * This function reads the input file line by line, identifies tokens using various
 * helper functions, and returns the next token. It handles whitespace, comments,
 * and preprocessor directives.
 * 
 * @param fp The file pointer to the input file.
 * @return Token_t* A pointer to the next token, or NULL if an error occurs.
 */
Token_t *nextToken(FILE *fp) {
  static char buf[1024] = "";
  static char *pos = buf;
  static int line = 0;
  static Token_t token;

  TokenType_t *type = NULL;
  do {
    if (*pos == '\0') {
      ++line;  // Increment line number of input file
      ++currline;  // Increment line number of preprocessed file
      if (fgets(buf, sizeof(buf), fp) == NULL) {
        if (feof(fp)) {
          token.id = T_EOF;
          token.pos = 0;
          token.line = line;
          return &token;
        }
        perror("Error: Cannot read from file");
        return NULL;
      }
      pos = buf;
      if (*pos == '#') {
        parse_directive(pos);
        *pos = '\0';
      }
    }
    while (*pos != '\0') {
      if (isspace(*pos)) {
        ++pos;
        continue;
      }
      if ((type = is_operator(pos)) != NULL) break;
      if ((type = is_keyword(pos)) != NULL) break;
      if ((type = is_litnumber(pos)) != NULL) break;
      if ((type = is_litstr(pos, '"', T_LITSTRING)) != NULL) break;
      if ((type = is_litstr(pos, '\'', T_LITCHAR)) != NULL) break;
      if ((type = is_id(pos)) != NULL) break;
      fprintf(stderr, "Error: Unknown token >%c< at line %d\n", *pos, line);
      return NULL;
    }
    if (type != NULL) {
      pos += type->len;
      break;
    }
  } while (*pos == '\0');
  token.id = type->id;
  token.pos = type->sstpos;
  token.file = currfilepos;
  token.line = currline;
  return &token;
}



/**
 * @brief Prints the details of a token.
 * 
 * This function outputs the line number, position, and ID of the given token.
 * 
 * @param token The token to be printed.
 */
void printToken(Token_t *token) {
  if (token == NULL) {
    return;
  }
  printf("%03u Token: %05d , ID: %02d\n", token->line, token->pos, token->id);
}



/**
 * @brief The main function of the lexer.
 * 
 * This function initializes the string store and token store, processes the input file
 * to extract tokens, and prints each token. It handles cleanup before exiting.
 * 
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return int 0 on success, or a non-zero error code on failure.
 */
int main(int argc, char *argv[]) {
  FILE *fp;
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <infile> <sstorfile> <tokenfile>\n", argv[0]);
    return 1;
  }
  if ((fp = fopen(argv[1], "r")) == NULL) {
    fprintf(stderr, "Error: Cannot open infile %s\n", argv[1]);
    return 1;
  }
  if (sstore_init(argv[2]) != 0) {
    fprintf(stderr, "Error: Cannot open sstorfile %s\n", argv[2]);
    fclose(fp);
    return 1;
  }
  if (tstore_init(argv[3]) != 0) {
    fprintf(stderr, "Error: Cannot open tokenfile %s\n", argv[3]);
    fclose(fp);
    sstore_close();
    return 1;
  }
  Token_t *token;
  while ((token = nextToken(fp)) != NULL) {
    printToken(token);
    tstore_add(token);
    if (token->id == T_EOF) {
      break;
    }
  }

  fclose(fp);
  tstore_close();
  sstore_close();
  return 0;
}

