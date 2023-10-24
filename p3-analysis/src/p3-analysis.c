/**
 * @file p3-analysis.c
 * @brief Compiler phase 3: static analysis
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
    int depth_whileLoop;
    int return_value;
    char* name;

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
        ErrorList_printf(ERROR_LIST, "Program does not contain a 'main' function!");

    else if (!ParameterList_is_empty(symbol->parameters) || symbol->type != INT)
        ErrorList_printf(ERROR_LIST, "Incorrect 'main' function declaration!");
}

void AnalysisVisitor_postvisit_program(NodeVisitor* visitor, ASTNode* node)
{
    
}

void AnalysisVisitor_previsit_vardecl (NodeVisitor* visitor, ASTNode* node)
{
    if (node->vardecl.type == VOID)
        ErrorList_printf(ERROR_LIST, "Void variable '%s' on line %d",
            node->vardecl.name, node->source_line);

    else if (node->vardecl.is_array && node->vardecl.array_length == 0)
        ErrorList_printf(ERROR_LIST, "Array '%s' on line %d must have positive non-zero length",
            node->vardecl.name, node->source_line);
}

void AnalysisVisitor_previsit_funcdecl(NodeVisitor* visitor, ASTNode* node)
{
    DATA->name = node->funcdecl.name;
}

void AnalysisVisitor_previsit_block(NodeVisitor* visitor, ASTNode* node)
{
    SymbolTable* table = ASTNode_get_attribute(node, "symbolTable");
    Symbol* symbol = SymbolTable_lookup(table->parent, "main");
    if (symbol)
        ErrorList_printf(ERROR_LIST, "Duplicate symbols named '%s' in scope started on line %d",
            symbol->name, node->source_line);
}

void AnalysisVisitor_previsit_assignment(NodeVisitor* visitor, ASTNode* node)
{
    SET_INFERRED_TYPE(node->assignment.location->type);
}

void AnalysisVisitor_postvisit_assignment(NodeVisitor* visitor, ASTNode* node)
{
    DecafType actual = GET_INFERRED_TYPE(node->assignment.value);
    DecafType expected = GET_INFERRED_TYPE(node->assignment.location);
    if (expected != actual && actual != UNKNOWN)
        ErrorList_printf(ERROR_LIST, "Type mismatch: %s is incompatible with %s on line %d",
            DecafType_to_string(expected), DecafType_to_string(actual) , node->source_line);
}

void AnalysisVisitor_previsit_conditional(NodeVisitor* visitor, ASTNode* node)
{
    SET_INFERRED_TYPE(BOOL);
}

void AnalysisVisitor_postvisit_conditional(NodeVisitor* visitor, ASTNode* node)
{
    DecafType actual = GET_INFERRED_TYPE(node->conditional.condition);
    DecafType expected = GET_INFERRED_TYPE(node);
    if (expected != actual && actual != UNKNOWN)
        ErrorList_printf(ERROR_LIST, "Type mismatch: %s expected but %s found on line %d",
            DecafType_to_string(expected), DecafType_to_string(actual) , node->source_line);

}

void AnalysisVisitor_previsit_whileLoop(NodeVisitor* visitor, ASTNode* node)
{
    DATA->depth_whileLoop++;
    SET_INFERRED_TYPE(BOOL);
}

void AnalysisVisitor_postvisit_whileLoop(NodeVisitor* visitor, ASTNode* node)
{
    DATA->depth_whileLoop--;
    DecafType actual = GET_INFERRED_TYPE(node->whileloop.condition);
    DecafType expected = GET_INFERRED_TYPE(node);
    if (expected != actual && actual != UNKNOWN)
        ErrorList_printf(ERROR_LIST, "Type mismatch: %s expected but %s found on line %d",
            DecafType_to_string(expected), DecafType_to_string(actual) , node->source_line);

}

void AnalysisVisitor_previsit_return(NodeVisitor* visitor, ASTNode* node)
{
    Symbol* symbol = lookup_symbol_with_reporting(visitor, node, DATA->name);
    if (symbol != NULL)
        SET_INFERRED_TYPE(symbol->type);
}

void AnalysisVisitor_postvisit_return(NodeVisitor* visitor, ASTNode* node)
{
    
    DecafType expected = GET_INFERRED_TYPE(node);
    DecafType actual = GET_INFERRED_TYPE(node->funcreturn.value);
    if (expected != actual && actual != UNKNOWN)
        ErrorList_printf(ERROR_LIST, "Type mismatch: %s expected but %s found on line %d",
            DecafType_to_string(expected), DecafType_to_string(actual) , node->source_line);

    else
        DATA->return_value = node->funcreturn.value;
}

void AnalysisVisitor_previsit_break(NodeVisitor* visitor, ASTNode* node)
{
    if (!DATA->depth_whileLoop)
        ErrorList_printf(ERROR_LIST, "Invalid 'break' outside loop on line %d",
            node->source_line);
}

void AnalysisVisitor_previsit_continue(NodeVisitor* visitor, ASTNode* node)
{
    if (!DATA->depth_whileLoop)
        ErrorList_printf(ERROR_LIST, "Invalid 'continue' outside loop on line %d",
            node->source_line);
}

bool helper_binaryop_isOperation(ASTNode* node)
{
    BinaryOpType operator = node->binaryop.operator;

    if (operator == ADDOP || operator == SUBOP || operator == DIVOP ||
        operator == MODOP || operator == MULOP)
        return true;
    
    return false;
}

void AnalysisVisitor_previsit_binaryop(NodeVisitor* visitor, ASTNode* node)
{
    if (helper_binaryop_isOperation(node))
        SET_INFERRED_TYPE(INT);

    else
        SET_INFERRED_TYPE(BOOL);
}

void AnalysisVisitor_postvisit_binaryop(NodeVisitor* visitor, ASTNode* node)
{
    DecafType left = GET_INFERRED_TYPE(node->binaryop.left);
    DecafType right = GET_INFERRED_TYPE(node->binaryop.right);
    DecafType operator = GET_INFERRED_TYPE(node);

    if (left != operator || right != operator)
        ErrorList_printf(ERROR_LIST, "Type mismatch: %s expected but %s found on line %d",
            DecafType_to_string(left), DecafType_to_string(right) , node->source_line);
}

void AnalysisVisitor_previsit_location (NodeVisitor* visitor, ASTNode* node)
{
    Symbol* symbol = lookup_symbol_with_reporting (visitor, node, node->location.name);
    if (symbol == NULL)
        SET_INFERRED_TYPE(UNKNOWN);

    else
        SET_INFERRED_TYPE(symbol->type);
}

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
    v->postvisit_program     = AnalysisVisitor_postvisit_program;
    v->previsit_vardecl      = AnalysisVisitor_previsit_vardecl;
    v->postvisit_vardecl     = NULL;
    v->previsit_funcdecl     = AnalysisVisitor_previsit_funcdecl;
    v->postvisit_funcdecl    = NULL;
    v->previsit_block        = AnalysisVisitor_previsit_block;
    v->postvisit_block       = NULL;
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
    v->previsit_unaryop      = NULL;
    v->postvisit_unaryop     = NULL;
    v->previsit_location     = AnalysisVisitor_previsit_location;
    v->postvisit_location    = NULL;
    v->previsit_funccall     = NULL;
    v->postvisit_funccall    = NULL;
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

