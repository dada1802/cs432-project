/**
 * @file p1-lexer.c
 * @brief Compiler phase 1: lexer
 * 
 * Group members: David Nguyen, Chris Simmons.
 * We did not use any AI-assist tools while creating this solution.
 */
#include "p1-lexer.h"

TokenQueue* lex(const char* text)
{
    TokenQueue* tokens = TokenQueue_new();
 
    /* compile regular expressions */
    Regex* whitespace = Regex_new("^[ \n]");
    Regex* identifiers = Regex_new("^[A-Za-z][A-Za-z0-9_]*");
    Regex* symbol = Regex_new("^[()+*-]");
    Regex* constant = Regex_new("^[1-9][0-9]*|0");
    Regex* string = Regex_new("^\"[a-zA-Z]*\"");
    Regex* hex = Regex_new("^0x[0-9a-f]*");
    char *str[] = {"def"};
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

            bool check = true;
            int len = sizeof(str) / sizeof(str[0]);

            // for (int i = 0; i < len; i++) {
            //     if (strcmp(str[i], text)) {
            //         check = false;
            //         break;
            //     }
            // }

            if (check) {
                TokenQueue_add(tokens, Token_new(ID, match, line_count));
            }

            else {
                TokenQueue_add(tokens, Token_new(KEY, match, line_count));
            }

        } else if (Regex_match(symbol, text, match)) {
            TokenQueue_add(tokens, Token_new(SYM, match, line_count));

        } else if (Regex_match(constant, text, match)) {
            TokenQueue_add(tokens, Token_new(DECLIT, match, line_count));

        } else if (Regex_match(string, text, match)) {
            TokenQueue_add(tokens, Token_new(STRLIT, match, line_count));

        } else if (Regex_match(hex, text, match)) {
            TokenQueue_add(tokens, Token_new(HEXLIT, match, line_count));

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