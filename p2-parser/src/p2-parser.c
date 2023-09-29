/**
 * @file p2-parser.c
 * @brief Compiler phase 2: parser
 * 
 * Group members: David Nguyen, Chris Simmons.
 * We did not use any AI-assist tools while creating this solution.
 */

#include "p2-parser.h"

/*
 * helper functions
 */

/**
 * @brief Look up the source line of the next token in the queue.
 * 
 * @param input Token queue to examine
 * @returns Source line
 */
int get_next_token_line (TokenQueue* input)
{
    if (TokenQueue_is_empty(input)) {
        Error_throw_printf("Unexpected end of input\n");
    }
    return TokenQueue_peek(input)->line;
}

/**
 * @brief Check next token for a particular type and text and discard it
 * 
 * Throws an error if there are no more tokens or if the next token in the
 * queue does not match the given type or text.
 * 
 * @param input Token queue to modify
 * @param type Expected type of next token
 * @param text Expected text of next token
 */
void match_and_discard_next_token (TokenQueue* input, TokenType type, const char* text)
{
    if (TokenQueue_is_empty(input)) {
        Error_throw_printf("Unexpected end of input (expected \'%s\')\n", text);
    }
    Token* token = TokenQueue_remove(input);
    if (token->type != type || !token_str_eq(token->text, text)) {
        Error_throw_printf("Expected \'%s\' but found '%s' on line %d\n",
                text, token->text, get_next_token_line(input));
    }
    Token_free(token);
}

/**
 * @brief Remove next token from the queue
 * 
 * Throws an error if there are no more tokens.
 * 
 * @param input Token queue to modify
 */
void discard_next_token (TokenQueue* input)
{
    if (TokenQueue_is_empty(input)) {
        Error_throw_printf("Unexpected end of input\n");
    }
    Token_free(TokenQueue_remove(input));
}

/**
 * @brief Look ahead at the type of the next token
 * 
 * @param input Token queue to examine
 * @param type Expected type of next token
 * @returns True if the next token is of the expected type, false if not
 */
bool check_next_token_type (TokenQueue* input, TokenType type)
{
    if (TokenQueue_is_empty(input)) {
        return false;
    }
    Token* token = TokenQueue_peek(input);
    return (token->type == type);
}

/**
 * @brief Look ahead at the type and text of the next token
 * 
 * @param input Token queue to examine
 * @param type Expected type of next token
 * @param text Expected text of next token
 * @returns True if the next token is of the expected type and text, false if not
 */
bool check_next_token (TokenQueue* input, TokenType type, const char* text)
{
    if (TokenQueue_is_empty(input)) {
        return false;
    }
    Token* token = TokenQueue_peek(input);
    return (token->type == type) && (token_str_eq(token->text, text));
}

/**
 * @brief Parse and return a Decaf type
 * 
 * @param input Token queue to modify
 * @returns Parsed type (it is also removed from the queue)
 */
DecafType parse_type (TokenQueue* input)
{
    if (TokenQueue_is_empty(input)) {
        Error_throw_printf("Unexpected end of input (expected type)\n");
    }
    Token* token = TokenQueue_remove(input);
    if (token->type != KEY) {
        Error_throw_printf("Invalid type '%s' on line %d\n", token->text, get_next_token_line(input));
    }
    DecafType t = VOID;
    if (token_str_eq("int", token->text)) {
        t = INT;
    } else if (token_str_eq("bool", token->text)) {
        t = BOOL;
    } else if (token_str_eq("void", token->text)) {
        t = VOID;
    } else {
        Error_throw_printf("Invalid type '%s' on line %d\n", token->text, get_next_token_line(input));
    }
    Token_free(token);
    return t;
}

/**
 * @brief Parse and return a Decaf identifier
 * 
 * @param input Token queue to modify
 * @param buffer String buffer for parsed identifier (should be at least
 * @c MAX_TOKEN_LEN characters long)
 */
void parse_id (TokenQueue* input, char* buffer)
{
    if (TokenQueue_is_empty(input)) {
        Error_throw_printf("Unexpected end of input (expected identifier)\n");
    }
    Token* token = TokenQueue_remove(input);
    if (token->type != ID) {
        Error_throw_printf("Invalid ID '%s' on line %d\n", token->text, get_next_token_line(input));
    }
    snprintf(buffer, MAX_ID_LEN, "%s", token->text);
    Token_free(token);
}

ASTNode* parse_vardecl (TokenQueue* token) 
{
    char buffer[MAX_TOKEN_LEN];
    DecafType type = parse_type(token);
    parse_id(token, buffer);
    int line = get_next_token_line(token);
    match_and_discard_next_token(token, SYM, ";");

    return VarDeclNode_new(buffer, type, false, 0, line);
}

// Expr -> Expr BINOP Expr
//      |  UNOP BaseExpr
//      |  BaseExpr
ASTNode* parse_expression (TokenQueue* token) 
{
    int line = get_next_token_line(token);
}

// STMT -> Loc '=' Expr ';'
//      |  FUNCCALL ';'
//      |  if '(' Expr ')' BLOCK
//      |  while '(' Expr ')' BLOCK
//      |  return Expr? ';'
//      |  break ';'
//      |  continue ';'
ASTNode* parse_statement (TokenQueue* token)
{
    int line = get_next_token_line(token);
    ASTNode* res;

    // WIP
    if (check_next_token(token, KEY, "Loc")) {
        res = ReturnNode_new(parse_expression(token), line);
        match_and_discard_next_token(token, SYM, ";");
    }

    // WIP
    else if (check_next_token(token, KEY, "FuncCall")) {
        res = ReturnNode_new(parse_expression(token), line);
        match_and_discard_next_token(token, SYM, ";");
    }

    else if (check_next_token(token, KEY, "if")) {
        match_and_discard_next_token(token, SYM, "(");

        // Expr

        match_and_discard_next_token(token, SYM, ")");
        res = ReturnNode_new(parse_expression(token), line);
        match_and_discard_next_token(token, SYM, ";");
    }

    else if (check_next_token(token, KEY, "while")) {
        // while '(' Expr ')' BLOCK
        // match_and_discard_next_token(token, SYM, "(");

        // ASTNode* expr = parse_expression(token);

        // match_and_discard_next_token(token, SYM, ")");
        
        // ASTNode* block = parse_block(token);
        
        // return WhileLoopNode_new(expr, block, line);
    }

    else if (check_next_token(token, KEY, "return")) {
        match_and_discard_next_token(token, KEY, "return");

        ASTNode* expr = NULL;

        if (!check_next_token(token, SYM, ";")) {
            expr = parse_expression(token);
        }

        res = ReturnNode_new(expr, line);
        match_and_discard_next_token(token, SYM, ";");
    }

    // Done
    else if (check_next_token(token, KEY, "break") ) {
        res = BreakNode_new(line);
        match_and_discard_next_token(token, KEY, "break");
        match_and_discard_next_token(token, SYM, ";");
    }
    
    // Done
    else if (check_next_token(token, KEY, "continue")) {
        res = ContinueNode_new(line);
        match_and_discard_next_token(token, KEY, "continue");
        match_and_discard_next_token(token, SYM, ";");
    }

    return res;
}

ASTNode* parse_block (TokenQueue* token)
{
    // Initialize the lists for the variables & statements
    // as well as discovering the next token line
    NodeList* vars = NodeList_new();
    NodeList* stmt = NodeList_new(); 
    int line = get_next_token_line(token);

    match_and_discard_next_token(token, SYM, "{");
    
    // Parse all preceding variable declarations
    while (check_next_token(token, KEY, "int") || check_next_token(token, KEY, "bool")) {
        NodeList_add(vars, parse_vardecl(token));
    }

    // Parse all following statements
    while (!check_next_token(token, SYM, "}")) {
        NodeList_add(stmt, parse_statement(token));
    }

    match_and_discard_next_token(token, SYM, "}");
    
    // Build & return the block node
    return BlockNode_new(vars, stmt, line);
}

ASTNode* parse_fundecl (TokenQueue* token)
{
    char buffer[MAX_TOKEN_LEN];
    int line = get_next_token_line(token);
    ParameterList* list = ParameterList_new();
    
    match_and_discard_next_token(token, KEY, "def");
    DecafType return_type = parse_type(token);
    parse_id(token, buffer);
    match_and_discard_next_token(token, SYM, "(");
    // Add parameters
    match_and_discard_next_token(token, SYM, ")");

    ASTNode* body = parse_block(token);

    return FuncDeclNode_new(buffer, return_type, list, body, line);
}

/*
 * node-level parsing functions
 */

ASTNode* parse_program (TokenQueue* input)
{
    NodeList* vars = NodeList_new();
    NodeList* funcs = NodeList_new();

    while (!TokenQueue_is_empty(input)) {
        if (check_next_token(input, KEY, "def")) {
            NodeList_add(funcs, parse_fundecl(input));
        }

        else {
            NodeList_add(vars, parse_vardecl(input));
        }
    }

    return ProgramNode_new(vars, funcs);
}

ASTNode* parse (TokenQueue* input)
{
    return parse_program(input);
}
