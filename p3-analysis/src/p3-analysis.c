/**
 * @file p3-analysis.c
 * @brief Compiler phase 3: static analysis
 * 
 * Group members: David Nguyen, Chris Simmons.
 * We did not use any AI-assist tools while creating this solution.
 */
#include "p3-analysis.h"

/**
 * @brief State/data for static analysis visitor
 */
typedef struct AnalysisData
{
    /**
     * @brief List of errors detected
     */
    ErrorList* errors;

    /* BOILERPLATE: TODO: add any new desired state information (and clean it up in AnalysisData_free) */
    int depth_whileLoop;    // This variable checks if we are in a loop.
    char* name;             // Store the name of a function.
    int in_block;           // This variable checks if we are in a block.

} AnalysisData;

/**
 * @brief Allocate memory for analysis data
 * 
 * @returns Pointer to allocated structure
 */
AnalysisData* AnalysisData_new ()
{
    AnalysisData* data = (AnalysisData*)calloc(1, sizeof(AnalysisData));
    CHECK_MALLOC_PTR(data);
    data->errors = ErrorList_new();
    data->depth_whileLoop = 0;
    data->in_block = 0;
    return data;
}

/**
 * @brief Deallocate memory for analysis data
 * 
 * @param data Pointer to the structure to be deallocated
 */
void AnalysisData_free (AnalysisData* data)
{
    /* free everything in data that is allocated on the heap except the error
     * list; it needs to be returned after the analysis is complete */

    /* free "data" itself */
    free(data);
}

/**
 * @brief Macro for more convenient access to the data inside a @ref AnalysisVisitor
 * data structure
 */
#define DATA ((AnalysisData*)visitor->data)

/**
 * @brief Macro for more convenient access to the error list inside a
 * @ref AnalysisVisitor data structure
 */
#define ERROR_LIST (((AnalysisData*)visitor->data)->errors)

/**
 * @brief Wrapper for @ref lookup_symbol that reports an error if the symbol isn't found
 * 
 * @param visitor Visitor with the error list for reporting
 * @param node AST node to begin the search at
 * @param name Name of symbol to find
 * @returns The @ref Symbol if found, otherwise @c NULL
 */
Symbol* lookup_symbol_with_reporting(NodeVisitor* visitor, ASTNode* node, const char* name)
{
    Symbol* symbol = lookup_symbol(node, name);
    if (symbol == NULL) {
        ErrorList_printf(ERROR_LIST, "Symbol '%s' undefined on line %d", name, node->source_line);
    }
    return symbol;
}

/**
 * @brief Macro for shorter storing of the inferred @c type attribute
 */
#define SET_INFERRED_TYPE(T) ASTNode_set_printable_attribute(node, "type", (void*)(T), \
                                 type_attr_print, dummy_free)

/**
 * @brief Macro for shorter retrieval of the inferred @c type attribute
 */
#define GET_INFERRED_TYPE(N) (DecafType)(long)ASTNode_get_attribute(N, "type")

void AnalysisVisitor_previsit_program (NodeVisitor* visitor, ASTNode* node)
{
    Symbol* symbol = lookup_symbol(node, "main");

    if (symbol == NULL)
        ErrorList_printf(ERROR_LIST, "Program does not contain a 'main' function");

    else if (symbol->symbol_type != FUNCTION_SYMBOL)
        ErrorList_printf(ERROR_LIST, "'main' must be a function");
}

// Helper method to check if str is already in the array for postvisit_program_block.
bool helper_isDuplicate(char** array, int size, char* str)
{
    for (int i = 0; i < size; i++) {
        if (!strcmp(array[i], str)) {
            return 1; // String already exists in the array
        }
    }

    return 0; // String does not exist in the array
}

// This function checks for duplicate variables in a block, or in the whole program outside of blocks.
void AnalysisVisitor_postvisit_program_block(NodeVisitor* visitor, ASTNode* node)
{
    SymbolTable* scope = (SymbolTable*)ASTNode_get_attribute(node, "symbolTable");
    SymbolList* local = scope->local_symbols;
    Symbol* first = local->head;
    char** names = (char**)calloc(local->size, sizeof(char*));
    int index = 0;

    while (first != NULL) {
        Symbol* second = first->next;

        while (second != NULL) {
            // If the names are the same, and is not already in the array for duplicates.
            if (!strcmp(first->name, second->name) && !helper_isDuplicate(names, index, first->name)) {
                names[index] = first->name;
                ErrorList_printf(ERROR_LIST, "Duplicate symbols named '%s' in scope started on line %d",
                        first->name, node->source_line);
                index++;
            }
            
            second = second->next;
        }

        first = first->next;
    }

    // Decrement the variable if we get out of a block.
    if (DATA->in_block != 0)
        DATA->in_block--;
    
    free(names);
}

// This function checks that variable are non-void types, and that arrays can't be length of 0 or less.
void AnalysisVisitor_previsit_vardecl(NodeVisitor* visitor, ASTNode* node)
{
    // Reject if type is void.
    if (node->vardecl.type == VOID)
        ErrorList_printf(ERROR_LIST, "Void variable '%s' on line %d",
            node->vardecl.name, node->source_line);

    // Reject if the size of the array is less or equal to 0.
    else if (node->vardecl.is_array && node->vardecl.array_length == 0)
        ErrorList_printf(ERROR_LIST, "Array '%s' on line %d must have positive non-zero length",
            node->vardecl.name, node->source_line);
}

// This function checks that variable inside a block can't be arrays.
void AnalysisVisitor_postvisit_vardecl(NodeVisitor* visitor, ASTNode* node)
{
    /**
     *  If DATA->in_block is not equal to 0, then we are inside a block.
     * Make sure the variable is not an array if we are in a block.
     */
    if (DATA->in_block != 0 && node->vardecl.is_array)
        ErrorList_printf(ERROR_LIST, "Local variable '%s' on line %d cannot be an array",
            node->vardecl.name, node->source_line);
}

// This function mostly checks for the main function.
void AnalysisVisitor_previsit_funcdecl(NodeVisitor* visitor, ASTNode* node)
{
    DATA->name = node->funcdecl.name;

    // If the name of the function is main.
    if (!strcmp(DATA->name, "main")) {
        Symbol* symbol = lookup_symbol_with_reporting(visitor, node, "main");

        // Make sure it returns an int.
        if (symbol->type != INT)
            ErrorList_printf(ERROR_LIST, "'main' must return an integer");

        // Make sure it doesn't have any parameter.
        if (!ParameterList_is_empty(symbol->parameters))
            ErrorList_printf(ERROR_LIST, "'main' must take no parameters");
    }
}

// Increment the variable DATA->in_block when we enter a block.
void AnalysisVisitor_previsit_block(NodeVisitor* visitor, ASTNode* node)
{
    DATA->in_block++;
}

// Set the inferred type of the assignment to the type of the left hand side of the assignment.
void AnalysisVisitor_previsit_assignment(NodeVisitor* visitor, ASTNode* node)
{
    SET_INFERRED_TYPE(node->assignment.location->type);
}

// Make sure the types of the two sides of the assignment match.
void AnalysisVisitor_postvisit_assignment(NodeVisitor* visitor, ASTNode* node)
{
    DecafType actual = GET_INFERRED_TYPE(node->assignment.value);
    DecafType expected = GET_INFERRED_TYPE(node->assignment.location);

    /**
     * Reject if the types don't match.
     * Make sure that the types are not UNKNOWN, meaning the variables were previously declared.
     */
    if (expected != actual && actual != UNKNOWN && expected != UNKNOWN)
        ErrorList_printf(ERROR_LIST, "Type mismatch: %s is incompatible with %s on line %d",
            DecafType_to_string(expected), DecafType_to_string(actual) , node->source_line);
}

// Set the type of a conditional to BOOL.
void AnalysisVisitor_previsit_conditional(NodeVisitor* visitor, ASTNode* node)
{
    SET_INFERRED_TYPE(BOOL);
}

// This function checks that the conditional is a BOOL.
void AnalysisVisitor_postvisit_conditional(NodeVisitor* visitor, ASTNode* node)
{
    DecafType actual = GET_INFERRED_TYPE(node->conditional.condition);
    DecafType expected = GET_INFERRED_TYPE(node);

    /**
     * Reject if the types don't match, or if the actual type is UNKNOWN.
     */
    if (expected != actual && actual != UNKNOWN)
        ErrorList_printf(ERROR_LIST, "Type mismatch: %s expected but %s found on line %d",
            DecafType_to_string(expected), DecafType_to_string(actual) , node->source_line);
}

/**
 * Increase the DATA->depth_whileLoop variable before we get in a loop.
 */
void AnalysisVisitor_previsit_whileLoop(NodeVisitor* visitor, ASTNode* node)
{
    DATA->depth_whileLoop++;
    SET_INFERRED_TYPE(BOOL);
}

/**
 * Decrease the DATA->depth_whileLoop before exiting the loop. 
 */
void AnalysisVisitor_postvisit_whileLoop(NodeVisitor* visitor, ASTNode* node)
{
    DATA->depth_whileLoop--;
    DecafType actual = GET_INFERRED_TYPE(node->whileloop.condition);
    DecafType expected = GET_INFERRED_TYPE(node);

    // Make sure the condition is a boolean, and that it is not UNKNOWn.
    if (expected != actual && actual != UNKNOWN)
        ErrorList_printf(ERROR_LIST, "Type mismatch: %s expected but %s found on line %d",
            DecafType_to_string(expected), DecafType_to_string(actual) , node->source_line);
}

/**
 * Set the inferred type of the return value to be the return type of the function.
 */
void AnalysisVisitor_previsit_return(NodeVisitor* visitor, ASTNode* node)
{
    Symbol* symbol = lookup_symbol_with_reporting(visitor, node, DATA->name);
    if (symbol != NULL)
        SET_INFERRED_TYPE(symbol->type);

    else
        SET_INFERRED_TYPE(UNKNOWN);
}

/**
 * Make sure that the actual return type matches the expected one.
 */
void AnalysisVisitor_postvisit_return(NodeVisitor* visitor, ASTNode* node)
{
    DecafType expected = GET_INFERRED_TYPE(node);

    // If node is not NULL.
    if (node->funcreturn.value != NULL) {
        DecafType actual = GET_INFERRED_TYPE(node->funcreturn.value);
        if (expected != actual && actual != UNKNOWN)
            ErrorList_printf(ERROR_LIST, "Type mismatch: %s expected but %s found on line %d",
                DecafType_to_string(expected), DecafType_to_string(actual) , node->source_line);
    }

    else {
        // Expects a return type but the return type is void.
        if (expected != VOID)
            ErrorList_printf(ERROR_LIST, "Invalid void return from non-void function on line %d",
                node->source_line);
    }
}

// Make sure that break statements can't be outside a loop.
void AnalysisVisitor_previsit_break(NodeVisitor* visitor, ASTNode* node)
{
    if (!DATA->depth_whileLoop)
        ErrorList_printf(ERROR_LIST, "Invalid 'break' outside loop on line %d",
            node->source_line);
}

// Make sure that continue statements can't be outside a loop.
void AnalysisVisitor_previsit_continue(NodeVisitor* visitor, ASTNode* node)
{
    if (!DATA->depth_whileLoop)
        ErrorList_printf(ERROR_LIST, "Invalid 'continue' outside loop on line %d",
            node->source_line);
}

/**
 * Helper method that returns true if the binary operator is either +, -, *, /, or %.
*/
bool helper_binaryop_isOperation(ASTNode* node)
{
    BinaryOpType operator = node->binaryop.operator;

    if (operator == ADDOP || operator == SUBOP || operator == DIVOP ||
        operator == MODOP || operator == MULOP)
        return true;
    
    return false;
}

// Set the inferred type of the operation properly.
void AnalysisVisitor_previsit_binaryop(NodeVisitor* visitor, ASTNode* node)
{   
    // If the operator is one from the helper method above.
    if (helper_binaryop_isOperation(node))
        SET_INFERRED_TYPE(INT);

    else
        SET_INFERRED_TYPE(BOOL);
}

// Handle all the type mistmatch.
void AnalysisVisitor_postvisit_binaryop(NodeVisitor* visitor, ASTNode* node)
{
    DecafType left = GET_INFERRED_TYPE(node->binaryop.left);
    DecafType right = GET_INFERRED_TYPE(node->binaryop.right);

    // Make sure both sides of the operator have a type.
    if (left != UNKNOWN && right != UNKNOWN) {

        // Make sure that for the && and || operators, both operands are booleans.
        if (node->binaryop.operator == ANDOP || node->binaryop.operator == OROP) {
            if (left != BOOL)
                ErrorList_printf(ERROR_LIST, "Type mismatch: bool expected but %s found on line %d",
                    DecafType_to_string(left), node->source_line);

            if (right != BOOL)
                ErrorList_printf(ERROR_LIST, "Type mismatch: bool expected but %s found on line %d",
                    DecafType_to_string(right) , node->source_line);
        }

        // Make sure that for the == and != operators, both operands are the same type, and none of them are strings.
        else if (node->binaryop.operator == EQOP || node->binaryop.operator == NEQOP) {
            if (left != right)
                ErrorList_printf(ERROR_LIST, "Type mismatch: %s is incompatible with %s on line %d",
                    DecafType_to_string(left), DecafType_to_string(right), node->source_line);

            if (left == STR || right == STR)
                ErrorList_printf(ERROR_LIST, "Unsupported string equality check on line %d",
                    node->source_line);
        }

        // Make sure that for the remaining operators, both operands are integers.
        else {
            if (left != INT)
                ErrorList_printf(ERROR_LIST, "Type mismatch: int expected but %s found on line %d",
                    DecafType_to_string(left), node->source_line);

            if (right != INT)
                ErrorList_printf(ERROR_LIST, "Type mismatch: int expected but %s found on line %d",
                    DecafType_to_string(right) , node->source_line);
        }
    }
}

/**
 * For the negation operator, set inferred type to integer.
 * For the not operator, set inferred type to boolean.
 */
void AnalysisVisitor_previsit_unaryop(NodeVisitor* visitor, ASTNode* node)
{
    if (node->unaryop.operator == NEGOP)
        SET_INFERRED_TYPE(INT);

    else
        SET_INFERRED_TYPE(BOOL);
}

// Make sure that the actual type matches the expected type.
void AnalysisVisitor_postvisit_unaryop(NodeVisitor* visitor, ASTNode* node)
{
    DecafType operator = GET_INFERRED_TYPE(node);
    DecafType child = GET_INFERRED_TYPE(node->unaryop.child);

    if (operator != child && child != UNKNOWN)
        ErrorList_printf(ERROR_LIST, "Type mismatch: %s expected but %s found on line %d",
            DecafType_to_string(operator), DecafType_to_string(child), node->source_line);
}

/**
 * Lookup the location node in the symbol table.
 * Set the inferred type to its type, else set it to UNKNOWN.
 */
void AnalysisVisitor_previsit_location(NodeVisitor* visitor, ASTNode* node)
{
    Symbol* symbol = lookup_symbol_with_reporting(visitor, node, node->location.name);
    if (symbol == NULL)
        SET_INFERRED_TYPE(UNKNOWN);

    else {
        SET_INFERRED_TYPE(symbol->type);

        if (symbol != NULL) {

            // If the location is an array.
            if (symbol->symbol_type == ARRAY_SYMBOL) {
                
                // Make sure that it is accessed with an index.
                if (node->location.index == NULL)
                    ErrorList_printf(ERROR_LIST, "Array '%s' accessed without index on line %d",
                        symbol->name, node->source_line);
            }
            
            // If the location is a variable, and if it is accessed with an index.
            else if (symbol->symbol_type == SCALAR_SYMBOL && node->location.index != NULL)
                ErrorList_printf(ERROR_LIST, "Scalar '%s' accessed as an array on line %d",
                    symbol->name, node->source_line);

            // If the function is accessed as a variable.
            else if (symbol->symbol_type == FUNCTION_SYMBOL)
                ErrorList_printf(ERROR_LIST, "Function '%s' accessed as a variable on line %d",
                    symbol->name, node->source_line);
        }
    }
}

// Make sure the types match.
void AnalysisVisitor_postvisit_location(NodeVisitor* visitor, ASTNode* node)
{
    if (GET_INFERRED_TYPE(node) != UNKNOWN) {
        
        // Rejects if the index is not NULL and the type is not an integer.
        if (node->location.index != NULL && GET_INFERRED_TYPE(node->location.index) != INT)
            ErrorList_printf(ERROR_LIST, "Type mismatch: int expected but %s found on line %d",
                DecafType_to_string(GET_INFERRED_TYPE(node->location.index)), node->source_line);
    }
}

/**
 * Lookup the function name in the symbol table.
 * Set the inferred type to be its return type, else set it to be UNKNOWN.
*/
void AnalysisVisitor_postvisit_funccall(NodeVisitor* visitor, ASTNode* node)
{
    Symbol* symbol = lookup_symbol_with_reporting(visitor, node, node->funccall.name);

    if (symbol == NULL)
        SET_INFERRED_TYPE(UNKNOWN);

    else {
        SET_INFERRED_TYPE(symbol->type);

        // If we call a variable as a function.
        if (symbol->symbol_type != FUNCTION_SYMBOL)
            ErrorList_printf(ERROR_LIST, "Invalid call to non-function '%s' on line %d",
                symbol->name, node->source_line);

        // If the number of parameters doesn't match.
        if (symbol->parameters->size != node->funccall.arguments->size)
            ErrorList_printf(ERROR_LIST, "Invalid number of function arguments on line %d",
                node->source_line);
        
        else {
            NodeList* list = node->funccall.arguments;
            ASTNode* current_actual = list->head;
            ParameterList* parameters = symbol->parameters;
            Parameter* current_expected = parameters->head;
            
            // Make sure that the types from the actual parameters match the types from the expected parameters.
            for (int i = 0; i < list->size; i++) {
                DecafType actual = GET_INFERRED_TYPE(current_actual);

                if (current_expected != NULL) {
                    DecafType expected = current_expected->type;
                    
                    if (actual != expected)
                        ErrorList_printf(ERROR_LIST, "Type mismatch in parameter %d of call to '%s': expected %s but found %s on line %d",
                            i, symbol->name, DecafType_to_string(expected), DecafType_to_string(actual), node->source_line);

                    current_actual = current_actual->next;
                    current_expected = current_expected->next;
                }
            }
        }
    }
}

// Set the type of the literal.
void AnalysisVisitor_previsit_literal(NodeVisitor* visitor, ASTNode* node)
{
    SET_INFERRED_TYPE(node->literal.type);
}

ErrorList* analyze (ASTNode* tree)
{
    /* allocate analysis structures */
    NodeVisitor* v = NodeVisitor_new();
    v->data = (void*)AnalysisData_new();
    v->dtor = (Destructor)AnalysisData_free;

    /* BOILERPLATE: TODO: register analysis callbacks */
    v->previsit_program      = AnalysisVisitor_previsit_program;
    v->postvisit_program     = AnalysisVisitor_postvisit_program_block;
    v->previsit_vardecl      = AnalysisVisitor_previsit_vardecl;
    v->postvisit_vardecl     = AnalysisVisitor_postvisit_vardecl;
    v->previsit_funcdecl     = AnalysisVisitor_previsit_funcdecl;
    v->postvisit_funcdecl    = NULL;
    v->previsit_block        = AnalysisVisitor_previsit_block;
    v->postvisit_block       = AnalysisVisitor_postvisit_program_block;
    v->previsit_assignment   = AnalysisVisitor_previsit_assignment;
    v->postvisit_assignment  = AnalysisVisitor_postvisit_assignment;
    v->previsit_conditional  = AnalysisVisitor_previsit_conditional;
    v->postvisit_conditional = AnalysisVisitor_postvisit_conditional;
    v->previsit_whileloop    = AnalysisVisitor_previsit_whileLoop;
    v->postvisit_whileloop   = AnalysisVisitor_postvisit_whileLoop;
    v->previsit_return       = AnalysisVisitor_previsit_return;
    v->postvisit_return      = AnalysisVisitor_postvisit_return;
    v->previsit_break        = AnalysisVisitor_previsit_break;
    v->postvisit_break       = NULL;
    v->previsit_continue     = AnalysisVisitor_previsit_continue;
    v->postvisit_continue    = NULL;
    v->previsit_binaryop     = AnalysisVisitor_previsit_binaryop;
    v->invisit_binaryop      = NULL;
    v->postvisit_binaryop    = AnalysisVisitor_postvisit_binaryop;
    v->previsit_unaryop      = AnalysisVisitor_previsit_unaryop;
    v->postvisit_unaryop     = AnalysisVisitor_postvisit_unaryop;
    v->previsit_location     = AnalysisVisitor_previsit_location;
    v->postvisit_location    = AnalysisVisitor_postvisit_location;
    v->previsit_funccall     = NULL;
    v->postvisit_funccall    = AnalysisVisitor_postvisit_funccall;
    v->previsit_literal      = AnalysisVisitor_previsit_literal;
    v->postvisit_literal     = NULL;

    /* perform analysis, save error list, clean up, and return errors */
    ErrorList* errors = ((AnalysisData*)v->data)->errors;

    if (tree == NULL)
        ErrorList_printf(errors, "Tree is NULL!");
    
    else {
        NodeVisitor_traverse(v, tree);
        NodeVisitor_free(v);
    }

    return errors;
}

