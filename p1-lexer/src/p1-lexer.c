/**
 * @file p1-lexer.c
 * @brief Compiler phase 1: lexer
 * 
 * 
 */
#include "p1-lexer.h"

TokenQueue* lex(const char* text)
{
    TokenQueue* tokens = TokenQueue_new();
 
    /* compile regular expressions */
    Regex* whitespace = Regex_new("^[ \n]");
    Regex* identifiers = Regex_new("^[A-Za-z][A-Za-z0-9_]*");
    // Regex* letter = Regex_new("^[a-z]");
    Regex* symbol = Regex_new("^[()+*]");
    Regex* constant = Regex_new("^[1-9][0-9]*|0");
    int line_count = 1;
 
    /* read and handle input */
    char match[MAX_TOKEN_LEN];
    while (*text != '\0') {
 
        /* match regular expressions */
        if (Regex_match(whitespace, text, match)) {
            /* ignore whitespace */
            if (strcmp(text, "\n")) {
                line_count++;
            }
            
        } else if (Regex_match(identifiers, text, match)) {
            /* TODO: implement line count and replace placeholder (-1) */
            TokenQueue_add(tokens, Token_new(ID, match, line_count));

        } else if (Regex_match(symbol, text, match)) {
            TokenQueue_add(tokens, Token_new(SYM, match, line_count));

        } else if (Regex_match(constant, text, match)) {
            TokenQueue_add(tokens, Token_new(DECLIT, match, line_count));

        } else {
            Error_throw_printf("Invalid token!\n");
        }
 
        /* skip matched text to look for next token */
        text += strlen(match);
    }
 
    /* clean up */
    Regex_free(whitespace);
    Regex_free(identifiers);
    Regex_free(symbol);
    Regex_free(constant);

    return tokens;
}