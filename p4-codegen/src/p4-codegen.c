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

    /* add any new desired state information (and clean it up in CodeGenData_free) */
    Operand* conditional_l1;
    Operand* conditional_l2;
    Operand* conditional_l3;
    int depth_conditionals;
    char *predefined_functions[3];
    Operand* whileloop_l1;
    Operand* whileloop_l2;
    Operand* whileloop_l3;
    Operand* load_256;
    int depth_whileloops;
    int index;
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
    data->whileloop_l1 = (Operand*)calloc(100, sizeof(Operand));
    data->whileloop_l2 = (Operand*)calloc(100, sizeof(Operand));
    data->whileloop_l3 = (Operand*)calloc(100, sizeof(Operand));
    data->depth_whileloops = 0;
    data->conditional_l1 = (Operand*)calloc(100, sizeof(Operand));
    data->conditional_l2 = (Operand*)calloc(100, sizeof(Operand));
    data->conditional_l3 = (Operand*)calloc(100, sizeof(Operand));
    data->depth_conditionals = 0;
    data->predefined_functions[0] = "print_int";
    data->predefined_functions[1] = "print_str";
    data->predefined_functions[2] = "print_bool";
    data->load_256 = (Operand*)calloc(100, sizeof(Operand));
    data->index = 0;
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
    free(data->whileloop_l1);
    free(data->whileloop_l2);
    free(data->whileloop_l3);
    free(data->conditional_l1);
    free(data->conditional_l2);
    free(data->conditional_l3);
    free(data->load_256);

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

/* ============================== PREVISIT FUNCDECL ============================== */
void CodeGenVisitor_previsit_funcdecl (NodeVisitor* visitor, ASTNode* node)
{
    /* generate a label reference for the epilogue that can be used while
     * generating the rest of the function (e.g., to be used when generating
     * code for a "return" statement) */
    DATA->current_epilogue_jump_label = anonymous_label();
}

/* ============================== POSTVISIT FUNCDECL ============================== */
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

    /* BOILERPLATE: TODO: implement epilogue */
    EMIT1OP(LABEL, DATA->current_epilogue_jump_label);
    EMIT2OP(I2I, base_reg, stack_reg);
    EMIT1OP(POP, base_reg);
    EMIT0OP(RETURN);
}

/* ============================== POSTVISIT BLOCK ============================== */
void CodeGenVisitor_postvisit_block (NodeVisitor* visitor, ASTNode* node)
{
    /* copy code from each statement */
    FOR_EACH(ASTNode*, statement, node->block.statements) {
        ASTNode_copy_code(node, statement);
    }
}

/* ============================== POSTVISIT ASSIGNMENT ============================== */
void CodeGenVisitor_postvisit_assignment (NodeVisitor* visitor, ASTNode* node)
{
    /* Copy code from the value */
    ASTNode_copy_code(node, node->assignment.value);
    Symbol* loc = lookup_symbol(node, node->assignment.location->location.name);
    Symbol* val = lookup_symbol(node, node->assignment.value->location.name);

    /* If the location exists */
    if (loc != NULL) {
        Operand ret = ASTNode_get_temp_reg(node->assignment.value);

        /* If the right hand side of the assignment is a location */
        if (val != NULL && val->symbol_type == ARRAY_SYMBOL) {
            EMIT3OP(LOAD_AO, DATA->load_256[DATA->index - 1], ret, ret);
            DATA->index = DATA->index - 1;
        }

        /* If the left hand side of the assignment is an array */
        if (loc->symbol_type == ARRAY_SYMBOL) {
            ASTNode_copy_code(node, node->assignment.location);
            EMIT3OP(STORE_AO, ret, DATA->load_256[DATA->index - 1], ASTNode_get_temp_reg(node->assignment.location));
            DATA->index = DATA->index - 1;
        }

        /* Else it is a simple variable */
        else {
            EMIT3OP(STORE_AI, ret, base_register(), var_offset(node, loc));
        }
    }
}

/* ============================== PREVISIT CONDITIONAL ============================== */
void CodeGenVisitor_previsit_conditional (NodeVisitor* visitor, ASTNode* node)
{
    DATA->conditional_l1[DATA->depth_conditionals] = anonymous_label();
    DATA->conditional_l2[DATA->depth_conditionals] = anonymous_label();
    
    DATA->depth_conditionals = DATA->depth_conditionals + 1;
}

/* ============================== POSTVISIT CONDITIONAL ============================== */
void CodeGenVisitor_postvisit_conditional (NodeVisitor* visitor, ASTNode* node)
{
    Operand virtual_reg = ASTNode_get_temp_reg(node->conditional.condition);

    /* Copy the code from the condition */
    ASTNode_copy_code(node, node->conditional.condition);

    EMIT3OP(CBR, virtual_reg, DATA->conditional_l1[DATA->depth_conditionals - 1], DATA->conditional_l2[DATA->depth_conditionals - 1]);

    /* Code for the if block */
    EMIT1OP(LABEL, DATA->conditional_l1[DATA->depth_conditionals - 1]);
    ASTNode_copy_code(node, node->conditional.if_block);

    /* If there is an else block */
    if (node->conditional.else_block != NULL) {
        DATA->conditional_l3[DATA->depth_conditionals - 1] = anonymous_label();
        EMIT1OP(JUMP, DATA->conditional_l3[DATA->depth_conditionals - 1]);
        EMIT1OP(LABEL, DATA->conditional_l2[DATA->depth_conditionals - 1]);
        ASTNode_copy_code(node, node->conditional.else_block);
        EMIT1OP(LABEL, DATA->conditional_l3[DATA->depth_conditionals - 1]);
    }

    else {
        EMIT1OP(LABEL, DATA->conditional_l2[DATA->depth_conditionals - 1]);
    }

    DATA->depth_conditionals = DATA->depth_conditionals - 1;
}

/* ============================== PREVISIT WHILELOOP ============================== */
void CodeGenVisitor_previsit_whileloop (NodeVisitor* visitor, ASTNode* node)
{
    DATA->whileloop_l1[DATA->depth_whileloops] = anonymous_label();
    DATA->whileloop_l2[DATA->depth_whileloops] = anonymous_label();
    DATA->whileloop_l3[DATA->depth_whileloops] = anonymous_label();

    DATA->depth_whileloops = DATA->depth_whileloops + 1;
}

/* ============================== POSTVISIT WHILELOOP ============================== */
void CodeGenVisitor_postvisit_whileloop (NodeVisitor* visitor, ASTNode* node)
{
    /* Copy code from condition */
    EMIT1OP(LABEL, DATA->whileloop_l1[DATA->depth_whileloops - 1]);
    ASTNode_copy_code(node, node->whileloop.condition);
    EMIT3OP(CBR, ASTNode_get_temp_reg(node->whileloop.condition), DATA->whileloop_l2[DATA->depth_whileloops - 1], DATA->whileloop_l3[DATA->depth_whileloops - 1]);

    /* Copy code from body */
    EMIT1OP(LABEL, DATA->whileloop_l2[DATA->depth_whileloops - 1]);
    ASTNode_copy_code(node, node->whileloop.body);
    EMIT1OP(JUMP, DATA->whileloop_l1[DATA->depth_whileloops - 1]);

    EMIT1OP(LABEL, DATA->whileloop_l3[DATA->depth_whileloops - 1]);

    DATA->depth_whileloops = DATA->depth_whileloops - 1;
}

/* ============================== POSTVISIT RETURN ============================== */
void CodeGenVisitor_postvisit_return (NodeVisitor* visitor, ASTNode* node)
{
    /* Copy code from the return value node */
    ASTNode_copy_code(node, node->funcreturn.value);

    Symbol* val = lookup_symbol(node, node->funcreturn.value->location.name);
    Operand ret = ASTNode_get_temp_reg(node->funcreturn.value);

    /* If the return value is a location */
    if (val != NULL) {
        if (val->symbol_type == ARRAY_SYMBOL) {
            EMIT3OP(LOAD_AO, DATA->load_256[DATA->index - 1], ret, ret);
            DATA->index = DATA->index - 1;
        }
    }

    EMIT2OP(I2I, ret, return_register());
    EMIT1OP(JUMP, DATA->current_epilogue_jump_label);
}

/* ============================== POSTVISIT BREAK ============================== */
void CodeGenVisitor_postvisit_break (NodeVisitor* visitor, ASTNode* node)
{
    EMIT1OP(JUMP, DATA->whileloop_l3[DATA->depth_whileloops - 1]);
}

/* ============================== POSTVISIT CONTINUE ============================== */
void CodeGenVisitor_postvisit_continue (NodeVisitor* visitor, ASTNode* node)
{
    EMIT1OP(JUMP, DATA->whileloop_l1[DATA->depth_whileloops - 1]);
}

/* ============================== POSTVISIT BINARYOP ============================== */
void CodeGenVisitor_postvisit_binaryop (NodeVisitor* visitor, ASTNode* node)
{
    Symbol* left_val = lookup_symbol(node, node->binaryop.left->location.name);
    Symbol* right_val = lookup_symbol(node, node->binaryop.right->location.name);
    Operand left = ASTNode_get_temp_reg(node->binaryop.left);
    Operand right = ASTNode_get_temp_reg(node->binaryop.right);
    Operand result = virtual_register();

    /* Code for left child */
    ASTNode_copy_code(node, node->binaryop.left);

    /* If left operand is an array */
    if (left_val != NULL && left_val->symbol_type == ARRAY_SYMBOL) {
        EMIT3OP(LOAD_AO, DATA->load_256[DATA->index - 2], left, left);
        DATA->index = DATA->index - 1;
    }

    /* Code for right child */
    ASTNode_copy_code(node, node->binaryop.right);

    /* If right operand is an array */
    if (right_val != NULL && right_val->symbol_type == ARRAY_SYMBOL) {
        EMIT3OP(LOAD_AO, DATA->load_256[DATA->index], right, right);
        DATA->index = DATA->index - 1;
    }

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

    case LTOP:
        EMIT3OP(CMP_LT, left, right, result);
        break;

    case LEOP:
        EMIT3OP(CMP_LE, left, right, result);
        break;

    case GTOP:
        EMIT3OP(CMP_GT, left, right, result);
        break;

    case GEOP:
        EMIT3OP(CMP_GE, left, right, result);
        break;

    case EQOP:
        EMIT3OP(CMP_EQ, left, right, result);
        break;

    case NEQOP:
        EMIT3OP(CMP_NE, left, right, result);
        break;

    case ANDOP:
        EMIT3OP(AND, left, right, result);
        break;

    case OROP:
        EMIT3OP(OR, left, right, result);
        break;
    
    default:
        break;
    }
    
    ASTNode_set_temp_reg(node, result);
}

/* ============================== POSTVISIT UNARYOP ============================== */
void CodeGenVisitor_postvisit_unaryop (NodeVisitor* visitor, ASTNode* node)
{
    Operand child = ASTNode_get_temp_reg(node->unaryop.child);
    Operand result = virtual_register();
    Symbol* child_val = lookup_symbol(node, node->unaryop.child->location.name);

    /* Copy code from the child */
    ASTNode_copy_code(node, node->unaryop.child);

    /* If the child is an array */
    if (child_val != NULL && child_val->symbol_type == ARRAY_SYMBOL) {
        EMIT3OP(LOAD_AO, DATA->load_256[DATA->index - 1], child, child);
        DATA->index = DATA->index - 1;
    }

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

/* ============================== POSTVISIT LOCATION ============================== */
void CodeGenVisitor_postvisit_location (NodeVisitor* visitor, ASTNode* node)
{
    Symbol* loc = lookup_symbol(node, node->location.name);

    if (loc != NULL) {
        Operand val = virtual_register();
        if (loc->symbol_type == ARRAY_SYMBOL) {
            Operand virtual_reg1 = virtual_register();
            ASTNode_copy_code(node, node->location.index);
            EMIT2OP(LOAD_I, int_const(256), virtual_reg1);
            DATA->load_256[DATA->index] = virtual_reg1;
            EMIT3OP(MULT_I, ASTNode_get_temp_reg(node->location.index), int_const(8), val);
            DATA->index = DATA->index + 1;
        }

        else {
            EMIT3OP(LOAD_AI, base_register(),var_offset(node, loc), val);
        }

        ASTNode_set_temp_reg(node, val);
    }
}

/* ============================== POSTVISIT FUNCCALL ============================== */
void CodeGenVisitor_postvisit_funccall (NodeVisitor* visitor, ASTNode* node)
{
    Symbol* function = lookup_symbol(node, node->funccall.name);
    Operand* arguments = (Operand*)calloc(function->parameters->size, sizeof(Operand));

    if (function != NULL) {
        
        /* Predefined functions */
        
        ASTNode* item;
        Symbol* param;
        Operand result;

        if (function->parameters->size != 0) {
            item = node->funccall.arguments->head;
            param = lookup_symbol(item, item->location.name);
            result = ASTNode_get_temp_reg(item);
        }

        if (!strcmp(function->name, "print_int")) {
            ASTNode_copy_code(node, item);
            
            if (param != NULL && param->symbol_type == ARRAY_SYMBOL) {
                EMIT3OP(LOAD_AO, DATA->load_256[DATA->index - 1], result, result);
                DATA->index = DATA->index - 1;
            }
            
            EMIT1OP(PRINT, result);
        }

        else if (!strcmp(function->name, "print_str")) {
            EMIT1OP(PRINT, str_const(item->literal.string));
        }

        else if (!strcmp(function->name, "print_bool")) {
            ASTNode_copy_code(node, item);
            EMIT1OP(PRINT, result);
        }

        else {
            Operand stack_reg = stack_register();
            Operand value = virtual_register();
            
            /* Copy code from arguments */
            NodeList* list1 = node->funccall.arguments;
            item = list1->head;
            
            for (int i = 0; i < list1->size; i++) {
                ASTNode_copy_code(node, item);

                Symbol* name = lookup_symbol(node, item->location.name);
                Operand val = ASTNode_get_temp_reg(item);

                if (name != NULL && name->symbol_type == ARRAY_SYMBOL) {
                    EMIT3OP(LOAD_AO, DATA->load_256[DATA->index - 1], val, val);
                    DATA->index = DATA->index - 1;
                }

                arguments[list1->size - 1 - i] = val;
                item = item->next;
            }

            /* Push registers of arguments */
            for (int i = 0; i < list1->size; i++) {
                EMIT1OP(PUSH, arguments[i]);
            }

            EMIT1OP(CALL, call_label(function->name));
            EMIT3OP(ADD_I, stack_reg, int_const(8 * node->funccall.arguments->size), stack_reg);
            EMIT2OP(I2I, return_register(), value);
            ASTNode_set_temp_reg(node, value);
        }
    }

    free(arguments);
}

/* ============================== POSTVISIT LITERAL ============================== */
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
    v->previsit_conditional  = CodeGenVisitor_previsit_conditional;
    v->postvisit_conditional = CodeGenVisitor_postvisit_conditional;
    v->previsit_whileloop    = CodeGenVisitor_previsit_whileloop;
    v->postvisit_whileloop   = CodeGenVisitor_postvisit_whileloop;
    v->previsit_return       = NULL;
    v->postvisit_return      = CodeGenVisitor_postvisit_return;
    v->previsit_break        = NULL;
    v->postvisit_break       = CodeGenVisitor_postvisit_break;
    v->previsit_continue     = NULL;
    v->postvisit_continue    = CodeGenVisitor_postvisit_continue;
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
