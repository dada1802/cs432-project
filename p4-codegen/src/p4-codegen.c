/**
 * @file p4-codegen.c
 * @brief Compiler phase 4: code generation
 * 
 * Group members: David Nguyen, Chris Simmons.
 * We did not use any AI-assist tools while creating this solution.
 */
#include "p4-codegen.h"

/**
 * @brief State/data for the code generator visitor
 */
typedef struct CodeGenData
{
    /**
     * @brief Reference to the epilogue jump label for the current function
     */
    Operand current_epilogue_jump_label;
    Operand offset;


    /* add any new desired state information (and clean it up in CodeGenData_free) */
} CodeGenData;

/**
 * @brief Allocate memory for code gen data
 * 
 * @returns Pointer to allocated structure
 */
CodeGenData* CodeGenData_new (void)
{
    CodeGenData* data = (CodeGenData*)calloc(1, sizeof(CodeGenData));
    CHECK_MALLOC_PTR(data);
    data->current_epilogue_jump_label = empty_operand();
    data->offset = empty_operand();
    return data;
}

/**
 * @brief Deallocate memory for code gen data
 * 
 * @param data Pointer to the structure to be deallocated
 */
void CodeGenData_free (CodeGenData* data)
{
    /* free everything in data that is allocated on the heap */

    /* free "data" itself */
    free(data);
}

/**
 * @brief Macro for more convenient access to the error list inside a @c visitor
 * data structure
 */
#define DATA ((CodeGenData*)visitor->data)

/**
 * @brief Fills a register with the base address of a variable.
 * 
 * @param node AST node to emit code into (if needed)
 * @param variable Desired variable
 * @returns Virtual register that contains the base address
 */
Operand var_base (ASTNode* node, Symbol* variable)
{
    Operand reg = empty_operand();
    switch (variable->location) {
        case STATIC_VAR:
            reg = virtual_register();
            ASTNode_emit_insn(node,
                    ILOCInsn_new_2op(LOAD_I, int_const(variable->offset), reg));
            break;
        case STACK_PARAM:
        case STACK_LOCAL:
            reg = base_register();
            break;
        default:
            break;
    }
    return reg;
}

/**
 * @brief Calculates the offset of a scalar variable reference and fills a register with that offset.
 * 
 * @param node AST node to emit code into (if needed)
 * @param variable Desired variable
 * @returns Virtual register that contains the base address
 */
Operand var_offset (ASTNode* node, Symbol* variable)
{
    Operand op = empty_operand();
    switch (variable->location) {
        case STATIC_VAR:    op = int_const(0); break;
        case STACK_PARAM:
        case STACK_LOCAL:   op = int_const(variable->offset);
        default:
            break;
    }
    return op;
}

#ifndef SKIP_IN_DOXYGEN

/*
 * Macros for more convenient instruction generation
 */

#define EMIT0OP(FORM)             ASTNode_emit_insn(node, ILOCInsn_new_0op(FORM))
#define EMIT1OP(FORM,OP1)         ASTNode_emit_insn(node, ILOCInsn_new_1op(FORM,OP1))
#define EMIT2OP(FORM,OP1,OP2)     ASTNode_emit_insn(node, ILOCInsn_new_2op(FORM,OP1,OP2))
#define EMIT3OP(FORM,OP1,OP2,OP3) ASTNode_emit_insn(node, ILOCInsn_new_3op(FORM,OP1,OP2,OP3))

void CodeGenVisitor_gen_program (NodeVisitor* visitor, ASTNode* node)
{
    /*
     * make sure "code" attribute exists at the program level even if there are
     * no functions (although this shouldn't happen if static analysis is run
     * first); also, don't include a print function here because there's not
     * really any need to re-print all the functions in the program node *
     */
    ASTNode_set_attribute(node, "code", InsnList_new(), (Destructor)InsnList_free);

    /* copy code from each function */
    FOR_EACH(ASTNode*, func, node->program.functions) {
        ASTNode_copy_code(node, func);
    }
}

void CodeGenVisitor_previsit_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    /* generate a label reference for the epilogue that can be used while
     * generating the rest of the function (e.g., to be used when generating
     * code for a "return" statement) */
    DATA->current_epilogue_jump_label = anonymous_label();
}

void CodeGenVisitor_postvisit_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    /* every function begins with the corresponding call label */
    EMIT1OP(LABEL, call_label(node->funcdecl.name));

    /* BOILERPLATE: TODO: implement prologue */
    Operand base_reg = base_register();
    Operand stack_reg = stack_register();

    EMIT1OP(PUSH, base_reg);
    EMIT2OP(I2I, stack_reg, base_reg);
    EMIT3OP(ADD_I, stack_reg, int_const(-8 * node->funcdecl.body->block.variables->size), stack_reg);

    /* copy code from body */
    ASTNode_copy_code(node, node->funcdecl.body);
    EMIT1OP(JUMP, DATA->current_epilogue_jump_label);

    /* BOILERPLATE: TODO: implement epilogue */
    EMIT1OP(LABEL, DATA->current_epilogue_jump_label);
    EMIT2OP(I2I, base_reg, stack_reg);
    EMIT1OP(POP, base_reg);
    EMIT0OP(RETURN);
}

void CodeGenVisitor_postvisit_block (NodeVisitor* visitor, ASTNode* node)
{
    /* copy code from each statement */
    FOR_EACH(ASTNode*, statement, node->block.statements) {
        ASTNode_copy_code(node, statement);
    }
}

void CodeGenVisitor_postvisit_assignment (NodeVisitor* visitor, ASTNode* node)
{
    /* Copy code from the value */
    ASTNode_copy_code(node, node->assignment.value);

    EMIT3OP(STORE_AI, ASTNode_get_temp_reg(node->assignment.value), base_register(), DATA->offset);
}

void CodeGenVisitor_postvisit_return (NodeVisitor* visitor, ASTNode* node)
{
    /* Copy code from the return value node */
    ASTNode_copy_code(node, node->funcreturn.value);

    EMIT2OP(I2I, ASTNode_get_temp_reg(node->funcreturn.value), return_register());
}

void CodeGenVisitor_postvisit_binaryop (NodeVisitor* visitor, ASTNode* node)
{
    /* Copy code from the left and right subchildren and assign them to the registers */
    ASTNode_copy_code(node, node->binaryop.left);
    ASTNode_copy_code(node, node->binaryop.right);

    Operand left = ASTNode_get_temp_reg(node->binaryop.left);
    Operand right = ASTNode_get_temp_reg(node->binaryop.right);
    Operand result = virtual_register();

    /* switch statement for the different operators */
    switch (node->binaryop.operator)
    {
    case ADDOP:
        EMIT3OP(ADD, left, right, result);
        break;

    case SUBOP:
        EMIT3OP(SUB, left, right, result);
        break;
    
    case MULOP:
        EMIT3OP(MULT, left, right, result);
        break;
    
    case DIVOP:
        EMIT3OP(DIV, left, right, result);
        break;

    case MODOP:
        EMIT3OP(DIV, left, right, result);
        EMIT3OP(MULT, right, result, result);
        EMIT3OP(SUB, left, result, result);
        break;
    
    default:
        break;
    }
    
    ASTNode_set_temp_reg(node, result);
}

void CodeGenVisitor_postvisit_unaryop (NodeVisitor* visitor, ASTNode* node)
{
    /* Copy code from the child */
    ASTNode_copy_code(node, node->unaryop.child);

    Operand child = ASTNode_get_temp_reg(node->unaryop.child);
    Operand result = virtual_register();

    /* Switch statement for the 2 unary operators */
    switch (node->unaryop.operator)
    {
    case NEGOP:
        EMIT2OP(NEG, child, result);
        break;

    case NOTOP:
        EMIT2OP(NOT, child, result);
        break;
    
    default:
        break;
    }

    ASTNode_set_temp_reg(node, result);
}

void CodeGenVisitor_postvisit_location (NodeVisitor* visitor, ASTNode* node)
{
    Symbol* loc = lookup_symbol(node, node->location.name);
    Operand virtual_reg = virtual_register();

    if (loc != NULL) {
        DATA->offset = var_offset(node, loc);
        EMIT3OP(LOAD_AI, base_register(), DATA->offset, virtual_reg);
        ASTNode_set_temp_reg(node, virtual_reg);
    }
}

void CodeGenVisitor_postvisit_funccall (NodeVisitor* visitor, ASTNode* node)
{
    Symbol* function = lookup_symbol(node, node->funccall.name);

    if (function != NULL) {
        Operand stack_reg = stack_register();
        Operand value = virtual_register();
        EMIT1OP(CALL, call_label(function->name));
        EMIT3OP(ADD_I, stack_reg, int_const(8 * node->funccall.arguments->size), stack_reg);
        EMIT2OP(I2I, return_register(), value);
        ASTNode_set_temp_reg(node, value);
    }
}

void CodeGenVisitor_postvisit_literal (NodeVisitor* visitor, ASTNode* node)
{
    Operand r0 = virtual_register();
    EMIT2OP(LOAD_I, int_const(node->literal.integer), r0);

    ASTNode_set_temp_reg(node, r0);
}

#endif
InsnList* generate_code (ASTNode* tree)
{
    InsnList* iloc = InsnList_new();

    NodeVisitor* v = NodeVisitor_new();
    v->data = CodeGenData_new();
    v->dtor = (Destructor)CodeGenData_free;
    v->previsit_program      = NULL;
    v->postvisit_program     = CodeGenVisitor_gen_program;
    v->previsit_vardecl      = NULL;
    v->postvisit_vardecl     = NULL;
    v->previsit_funcdecl     = CodeGenVisitor_previsit_funcdecl;
    v->postvisit_funcdecl    = CodeGenVisitor_postvisit_funcdecl;
    v->previsit_block        = NULL;
    v->postvisit_block       = CodeGenVisitor_postvisit_block;
    v->previsit_assignment   = NULL;
    v->postvisit_assignment  = CodeGenVisitor_postvisit_assignment;
    v->previsit_conditional  = NULL;
    v->postvisit_conditional = NULL;
    v->previsit_whileloop    = NULL;
    v->postvisit_whileloop   = NULL;
    v->previsit_return       = NULL;
    v->postvisit_return      = CodeGenVisitor_postvisit_return;
    v->previsit_break        = NULL;
    v->postvisit_break       = NULL;
    v->previsit_continue     = NULL;
    v->postvisit_continue    = NULL;
    v->previsit_binaryop     = NULL;
    v->invisit_binaryop      = NULL;
    v->postvisit_binaryop    = CodeGenVisitor_postvisit_binaryop;
    v->previsit_unaryop      = NULL;
    v->postvisit_unaryop     = CodeGenVisitor_postvisit_unaryop;
    v->previsit_location     = NULL;
    v->postvisit_location    = CodeGenVisitor_postvisit_location;
    v->previsit_funccall     = NULL;
    v->postvisit_funccall    = CodeGenVisitor_postvisit_funccall;
    v->previsit_literal      = NULL;
    v->postvisit_literal     = CodeGenVisitor_postvisit_literal;

    /* generate code into AST attributes */
    if (tree == NULL)
        InsnList_add(iloc, NULL);
    
    else {
        NodeVisitor_traverse_and_free(v, tree);
        /* copy generated code into new list (the AST may be deallocated before
        * the ILOC code is needed) */
        FOR_EACH(ILOCInsn*, i, (InsnList*)ASTNode_get_attribute(tree, "code")) {
            InsnList_add(iloc, ILOCInsn_copy(i));
        }
    }

    return iloc;
}
