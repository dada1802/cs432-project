/**
 * @file p2-parser.c
 * @brief Compiler phase 2: parser
 * 
 * Group members: David Nguyen, Chris Simmons.
 * We used an AI to help us extracting the string literal from the token.
 */

#include "p2-parser.h"
ASTNode* parse_block();
ASTNode* parse_and_or_expression();

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
    int line = get_next_token_line(token);
    bool array = false;
    int length = 1;
    DecafType type = parse_type(token);
    parse_id(token, buffer);

    if (check_next_token(token, SYM, "[")) {
        match_and_discard_next_token(token, SYM, "[");

        if (check_next_token_type(token, DECLIT)) {
            array = true;
            Token* tok = TokenQueue_remove(token);
            length = strtol(tok->text, NULL, 0);

            Token_free(tok);
        }

        else {
            Error_throw_printf("Size of array has to be a Decimal on line %d\n", line);
        }

        match_and_discard_next_token(token, SYM, "]");
    }

    match_and_discard_next_token(token, SYM, ";");

    return VarDeclNode_new(buffer, type, array, length, line);
}

// Args -> Expr (',' Expr)*
NodeList* parse_args (TokenQueue* token)
{
    NodeList* args = NodeList_new();

    while (!check_next_token(token, SYM, ")")) {
        ASTNode* expr = parse_and_or_expression(token);
        NodeList_add(args, expr);

        if (check_next_token(token, SYM, ",")) {
            match_and_discard_next_token(token, SYM, ",");
        }
    }

    return args;
}

void parse_string(char* input, char* output, size_t size) {
    size_t len = strlen(input);
    size_t outputIndex = 0;
    int insideQuotes = 0;

    for (size_t i = 0; i < len; i++) {
        if (input[i] == '\"') {
            insideQuotes = 1;
            continue;
        }

        if (insideQuotes) {
            if (outputIndex < size - 1) {

                if (input[i] == '\\') {
                    char temp = input[i + 1];

                    if (temp == 'n') {
                        output[outputIndex++] = '\n';

                    }

                    else if (temp == 't') {
                        output[outputIndex++] = '\t';
                    }

                    else if (temp == '"') {
                        output[outputIndex++] = '\"';
                    }
                    i++;
                }

                else {
                    output[outputIndex++] = input[i];
                }
            }
        }
    }

    output[outputIndex] = '\0';
}

ASTNode* parse_baseExpr (TokenQueue* token) {
    int line = get_next_token_line(token);
    ASTNode* res = NULL;

    // Decimal
    if (check_next_token_type(token, DECLIT)) {
        Token* tok = TokenQueue_remove(token);
        int value = strtol(tok->text, NULL, 0);
        res = LiteralNode_new_int(value, line);
        Token_free(tok);
    }

    // Hexdecimal
    else if (check_next_token_type(token, HEXLIT)) {
        Token* tok = TokenQueue_remove(token);
        int value = strtol(tok->text, NULL, 16);
        res = LiteralNode_new_int(value, line);
        Token_free(tok);
    }

    // true and false
    else if (check_next_token(token, KEY, "false")) {
        res = LiteralNode_new_bool(false, line);
        match_and_discard_next_token(token, KEY, "false");
    }

    else if (check_next_token(token, KEY, "true")) {
        res = LiteralNode_new_bool(true, line);
        match_and_discard_next_token(token, KEY, "true");
    }

    // String
    else if (check_next_token_type(token, STRLIT)) {
        Token* tok = TokenQueue_remove(token);
        char text[MAX_TOKEN_LEN];
        parse_string(tok->text, text, sizeof(text));
        res = LiteralNode_new_string(text, line);
    }

    // '(' Expr ')'
    else if (check_next_token(token, SYM, "(")) {
        match_and_discard_next_token(token, SYM, "(");
        res = parse_and_or_expression(token);
        match_and_discard_next_token(token, SYM, ")");
    }

    else {
        char buffer[MAX_TOKEN_LEN];
        parse_id(token, buffer);

        // FuncCall
        if (check_next_token(token, SYM, "(")) {
            match_and_discard_next_token(token, SYM, "(");

            NodeList* args = parse_args(token);
            match_and_discard_next_token(token, SYM, ")");
            res = FuncCallNode_new(buffer, args, line);
        }

        // Loc
        else {
            ASTNode* index = NULL;
            if (check_next_token(token, SYM, "[")) {
                match_and_discard_next_token(token, SYM, "[");
                index = parse_and_or_expression(token);
                match_and_discard_next_token(token, SYM, "]");
            }

            res = LocationNode_new(buffer, index, line);
        }
    }

    return res;
}

// - and ! operators
ASTNode* parse_unary_expression(TokenQueue* token) {
    int line = get_next_token_line(token);
    ASTNode* child;
    ASTNode* op;

    if (check_next_token(token, SYM, "-")) {
        match_and_discard_next_token(token, SYM, "-");
        child = parse_baseExpr(token);
        op = UnaryOpNode_new(NEGOP, child, line); 
    }

    else if (check_next_token(token, SYM, "!")) {
        match_and_discard_next_token(token, SYM, "!");
        child = parse_baseExpr(token);
        op = UnaryOpNode_new(NOTOP, child, line);
    }

    else {
        return parse_baseExpr(token);
    }

    return op;
}

bool div_mul_mod_helper(TokenQueue* token)
{
    return check_next_token(token, SYM, "/")
        || check_next_token(token, SYM, "*")
        || check_next_token(token, SYM, "%");
}

// /, *, % operators
ASTNode* parse_div_mul_mod_expression(TokenQueue* token) {
    int line = get_next_token_line(token);
    ASTNode* left = parse_unary_expression(token);
    ASTNode* right;

    while (div_mul_mod_helper(token)) {
        if (check_next_token(token, SYM, "/")) {
            match_and_discard_next_token(token, SYM, "/");
            right = parse_unary_expression(token);
            left = BinaryOpNode_new(DIVOP, left, right, line); 
        }

        else if (check_next_token(token, SYM, "*")) {
            match_and_discard_next_token(token, SYM, "*");
            right = parse_unary_expression(token);
            left = BinaryOpNode_new(MULOP, left, right, line);
        }

        else if (check_next_token(token, SYM, "%")) {
            match_and_discard_next_token(token, SYM, "%");
            right = parse_unary_expression(token);
            left = BinaryOpNode_new(MODOP, left, right, line);
        }
    }

    return left;
}

bool add_sub_helper(TokenQueue* token)
{
    return check_next_token(token, SYM, "+")
        || check_next_token(token, SYM, "-");
}

// + and - operators
ASTNode* parse_add_sub_expression(TokenQueue* token) {
    int line = get_next_token_line(token);
    ASTNode* left = parse_div_mul_mod_expression(token);
    ASTNode* right;

    while (add_sub_helper(token)) {
        if (check_next_token(token, SYM, "+")) {
            match_and_discard_next_token(token, SYM, "+");
            right = parse_div_mul_mod_expression(token);
            left = BinaryOpNode_new(ADDOP, left, right, line); 
        }

        else if (check_next_token(token, SYM, "-")) {
            match_and_discard_next_token(token, SYM, "-");
            right = parse_div_mul_mod_expression(token);
            left = BinaryOpNode_new(SUBOP, left, right, line);
        }
    }

    return left;
}

bool booleans_helper(TokenQueue* token)
{
    return check_next_token(token, SYM, "<")
        || check_next_token(token, SYM, "<=")
        || check_next_token(token, SYM, ">")
        || check_next_token(token, SYM, ">=")
        || check_next_token(token, SYM, "==")
        || check_next_token(token, SYM, "!=");
}

// <, <=, >, >=, ==, != operators
ASTNode* parse_boolean_expression(TokenQueue* token) {
    int line = get_next_token_line(token);
    ASTNode* left = parse_add_sub_expression(token);
    ASTNode* right;

    while (booleans_helper(token)) {
        if (check_next_token(token, SYM, "<")) {
            match_and_discard_next_token(token, SYM, "<");
            right = parse_add_sub_expression(token);
            left = BinaryOpNode_new(LTOP, left, right, line); 
        }

        else if (check_next_token(token, SYM, "<=")) {
            match_and_discard_next_token(token, SYM, "<=");
            right = parse_add_sub_expression(token);
            left = BinaryOpNode_new(LEOP, left, right, line);
        }

        else if (check_next_token(token, SYM, ">")) {
            match_and_discard_next_token(token, SYM, ">");
            right = parse_add_sub_expression(token);
            left = BinaryOpNode_new(GTOP, left, right, line);
        }

        else if (check_next_token(token, SYM, ">=")) {
            match_and_discard_next_token(token, SYM, ">=");
            right = parse_add_sub_expression(token);
            left = BinaryOpNode_new(GEOP, left, right, line);
        }

        else if (check_next_token(token, SYM, "==")) {
            match_and_discard_next_token(token, SYM, "==");
            right = parse_add_sub_expression(token);
            left = BinaryOpNode_new(EQOP, left, right, line);
        }

        else if (check_next_token(token, SYM, "!=")) {
            match_and_discard_next_token(token, SYM, "!=");
            right = parse_add_sub_expression(token);
            left = BinaryOpNode_new(NEQOP, left, right, line);
        }
    }

    return left;
}

bool and_or_helper(TokenQueue* token)
{
    return check_next_token(token, SYM, "&&")
        || check_next_token(token, SYM, "||");
}

// && and || operators
ASTNode* parse_and_or_expression(TokenQueue* token)
{
    int line = get_next_token_line(token);
    ASTNode* left = parse_boolean_expression(token);
    ASTNode* right;

    while (and_or_helper(token)) {
            if (check_next_token(token, SYM, "&&")) {
            match_and_discard_next_token(token, SYM, "&&");
            right = parse_boolean_expression(token);
            left = BinaryOpNode_new(ANDOP, left, right, line); 
        }

        else if (check_next_token(token, SYM, "||")) {
            match_and_discard_next_token(token, SYM, "||");
            right = parse_boolean_expression(token);
            left = BinaryOpNode_new(OROP, left, right, line);
        }
    }

    return left;
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

    if (check_next_token(token, KEY, "if")) {
        // if '(' Expr ')' Block (else Block)?
        match_and_discard_next_token(token, KEY, "if");
        match_and_discard_next_token(token, SYM, "(");
        ASTNode* expr = parse_and_or_expression(token);
        match_and_discard_next_token(token, SYM, ")");
        ASTNode* if_block = parse_block(token);
        ASTNode* else_block = NULL;

        if (check_next_token(token, KEY, "else")) {
            match_and_discard_next_token(token, KEY, "else");
            else_block = parse_block(token);
        }

        res = ConditionalNode_new(expr, if_block, else_block, line);
    }

    else if (check_next_token(token, KEY, "while")) {
        // while '(' Expr ')' BLOCK
        int line = get_next_token_line(token);
        match_and_discard_next_token(token, KEY, "while");
        match_and_discard_next_token(token, SYM, "(");
        ASTNode* expr = parse_and_or_expression(token);
        match_and_discard_next_token(token, SYM, ")");
        
        ASTNode* block = parse_block(token);
        
        return WhileLoopNode_new(expr, block, line);
    }

    else if (check_next_token(token, KEY, "return")) {
        match_and_discard_next_token(token, KEY, "return");

        ASTNode* expr = NULL;

        if (!check_next_token(token, SYM, ";")) {
            expr = parse_and_or_expression(token);
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

    else {
        char buffer[MAX_TOKEN_LEN];
        parse_id(token, buffer);

        // FuncCall
        if (check_next_token(token, SYM, "(")) {
            match_and_discard_next_token(token, SYM, "(");

            NodeList* args = parse_args(token);
            match_and_discard_next_token(token, SYM, ")");
            res = FuncCallNode_new(buffer, args, line);
        }

        // Loc
        else {
            ASTNode* index = NULL;
            if (check_next_token(token, SYM, "[")) {
                match_and_discard_next_token(token, SYM, "[");
                index = parse_and_or_expression(token);
                match_and_discard_next_token(token, SYM, "]");
            }

            ASTNode* loc = LocationNode_new(buffer, index, line);
            match_and_discard_next_token(token, SYM, "=");
            ASTNode* expr = parse_and_or_expression(token);
            match_and_discard_next_token(token, SYM, ";");

            res = AssignmentNode_new(loc, expr, line);
        }
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
    while (!check_next_token(token, SYM, ")")) {
        DecafType param_type = parse_type(token);
        char param_name[MAX_TOKEN_LEN];
        parse_id(token, param_name);

        ParameterList_add_new(list, param_name, param_type);

        if (check_next_token(token, SYM, ",")) {
            match_and_discard_next_token(token, SYM, ",");
        }
    }
    
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
    if (input == NULL) {
        Error_throw_printf("Input is NULL!");
    }

    return parse_program(input);
}
