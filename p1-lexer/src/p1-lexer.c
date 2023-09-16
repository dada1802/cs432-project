/**
 * @file p1-lexer.c
 * @brief Compiler phase 1: lexer
 * 
 * Group members: David Nguyen, Chris Simmons.
 * We did not use any AI-assist tools while creating this solution.
 */
#include "p1-lexer.h"

// Helper method
int findIncorrect(char *match) {
    char *correct[] = {"def", "if", "else", "while", "return", "break", "continue", "int", "bool", "void",
                        "true", "false"};
    
    char *incorrect[] = {"for", "callout", "class", "interface", "extends", "implements", "new", "this",
                        "string", "float", "double", "null"};

    int len = sizeof(correct) / sizeof(correct[0]);
    for (int i = 0; i < len; i++) {
        if (strcmp(match, correct[i]) == 0) {
            return 2;
        }
    }
    
    len = sizeof(incorrect) / sizeof(incorrect[0]);
    for (int i = 0; i < len; i++) {
        if (strcmp(match, incorrect[i]) == 0) {
            return 1;
        }
    }
    
    return 0;
}

TokenQueue* lex(const char* text)
{
    TokenQueue* tokens = TokenQueue_new();
 
    /* compile regular expressions */
    Regex* whitespace = Regex_new("^[\n \t]");
    Regex* identifiers = Regex_new("^[A-Za-z][A-Za-z0-9_]*");
    Regex* symbol = Regex_new("^([][%()*,;+{}-])|[><=!]{0,1}=|&&|(\\|\\|)");
    Regex* constant = Regex_new("^[1-9][0-9]*|0");
    Regex* string = Regex_new("^(\"[a-zA-Z \\\n\\\t\\\"\\\\]*\")");
    Regex* hex = Regex_new("^0x([1-9a-f]*|0)");
    Regex* comment = Regex_new("^//[a-zA-Z \n\t\\\"]*");
    int line_count = 1;

    if (text == NULL) {
        Error_throw_printf("String is NULL!\n");
    }

    /* read and handle input */
    char match[MAX_TOKEN_LEN];
    while (*text != '\0') {
        
        /* match regular expressions */
        if (Regex_match(whitespace, text, match) || Regex_match(comment, text, match)) {
            /* ignore whitespace */
            if (strcmp(match, "\n") == 0) {
                line_count++;
            }

        } else if (Regex_match(identifiers, text, match)) {
            /* TODO: implement line count and replace placeholder (-1) */

            if (findIncorrect(match) == 2) {
                TokenQueue_add(tokens, Token_new(KEY, match, line_count));
            }

            else if (findIncorrect(match) == 1) {
                Error_throw_printf("Invalid token!\n");
            }
            
            else {
                TokenQueue_add(tokens, Token_new(ID, match, line_count));
            }

        } else if (Regex_match(string, text, match)) {
            TokenQueue_add(tokens, Token_new(STRLIT, match, line_count));

        } else if (Regex_match(symbol, text, match)) {
            TokenQueue_add(tokens, Token_new(SYM, match, line_count));

        } else if (Regex_match(hex, text, match)) {
            TokenQueue_add(tokens, Token_new(HEXLIT, match, line_count));

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
    Regex_free(string);
    Regex_free(hex);
    Regex_free(comment);

    return tokens;
}