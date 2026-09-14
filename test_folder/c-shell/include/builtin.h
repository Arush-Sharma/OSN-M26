#ifndef BUILTIN_H
#define BUILTIN_H

#include "lexer.h"

// Expose the global home_dir from main.c
extern char home_dir[1024];

int execute_builtin(Token *tokens, int token_count);

#endif