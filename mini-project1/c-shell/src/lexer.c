#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../include/lexer.h"

typedef enum {
    STATE_NORMAL,
    STATE_IN_DOUBLE_QUOTES,
    STATE_IN_SINGLE_QUOTES
} LexerState;

int tokenize_input(const char *input, Token *tokens, int max_tokens) {
    int token_count = 0;
    LexerState state = STATE_NORMAL;
    
    char current_word[256];
    int char_index = 0;
    
    for (size_t i = 0; i <= strlen(input); i++) {
        char c = input[i];

        if (state == STATE_NORMAL) {
            if (isspace(c) || c == '\0') {
                if (char_index > 0) {
                    current_word[char_index] = '\0';
                    tokens[token_count].type = TOKEN_WORD;
                    strcpy(tokens[token_count].value, current_word);
                    token_count++;
                    char_index = 0; 
                }
            } 
            else if (c == '\"') {
                state = STATE_IN_DOUBLE_QUOTES;
            } 
            else if (c == '\'') {
                state = STATE_IN_SINGLE_QUOTES;
            }
            else {
                current_word[char_index++] = c;
            }
        } 
        else if (state == STATE_IN_DOUBLE_QUOTES) {
            if (c == '\"') {
                state = STATE_NORMAL;
            } else if (c != '\0') {
                current_word[char_index++] = c;
            }
        }
        else if (state == STATE_IN_SINGLE_QUOTES) {
            if (c == '\'') {
                state = STATE_NORMAL;
            } else if (c != '\0') {
                current_word[char_index++] = c;
            }
        }

        // Prevent overflow
        if (token_count >= max_tokens) break;
    }
    
    return token_count;
}