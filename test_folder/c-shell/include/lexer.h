#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,     // |
    TOKEN_AMP,      // &
    TOKEN_SEMI,     // ;
    TOKEN_LT,       // <
    TOKEN_GT,       // >
    TOKEN_GTGT      // >>
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
} Token;

int tokenize_input(const char *input, Token *tokens, int max_tokens);

int execute_pipeline(Token *tokens, int token_count);

#endif