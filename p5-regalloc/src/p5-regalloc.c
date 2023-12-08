/**
 * @file p5-regalloc.c
 * @brief Compiler phase 5: register allocation
 * 
 * Group members: David Nguyen, Chris Simmons.
 * We did not use any AI-assist tools while creating this solution.
 */
#include "p5-regalloc.h"

int name[MAX_PHYSICAL_REGS];
int offset[MAX_VIRTUAL_REGS];
#define INVALID -1
#define INFINITY -2

/**
 * @brief Replace a virtual register id with a physical register id
 * 
 * Every virtual register operand with ID "vr" will be replaced by a physical
 * register operand with ID "pr" in the given instruction.
 * 
 * @param vr Virtual register id that should be replaced
 * @param pr Physical register id that it should be replaced by
 * @param isnsn Instruction to modify
 */
void replace_register(int vr, int pr, ILOCInsn* insn)
{
    for (int i = 0; i < 3; i++) {
        if (insn->op[i].type == VIRTUAL_REG && insn->op[i].id == vr) {
            insn->op[i].type = PHYSICAL_REG;
            insn->op[i].id = pr;
        }
    }
}

/**
 * @brief Insert a store instruction to spill a register to the stack
 * 
 * We need to allocate a new slot in the stack from for the current
 * function, so we need a reference to the local allocator instruction.
 * This instruction will always be the third instruction in a function
 * and will be of the form "add SP, -X => SP" where X is the current
 * stack frame size.
 * 
 * @param pr Physical register id that should be spilled
 * @param prev_insn Reference to an instruction; the new instruction will be
 * inserted directly after this one
 * @param local_allocator Reference to the local frame allocator instruction
 * @returns BP-based offset where the register was spilled
 */
int insert_spill(int pr, ILOCInsn* prev_insn, ILOCInsn* local_allocator)
{
    /* adjust stack frame size to add new spill slot */
    int bp_offset = local_allocator->op[1].imm - WORD_SIZE;
    local_allocator->op[1].imm = bp_offset;

    /* create store instruction */
    ILOCInsn* new_insn = ILOCInsn_new_3op(STORE_AI,
            physical_register(pr), base_register(), int_const(bp_offset));

    /* insert into code */
    new_insn->next = prev_insn->next;
    prev_insn->next = new_insn;

    return bp_offset;
}

/**
 * @brief Insert a load instruction to load a spilled register
 * 
 * @param bp_offset BP-based offset where the register value is spilled
 * @param pr Physical register where the value should be loaded
 * @param prev_insn Reference to an instruction; the new instruction will be
 * inserted directly after this one
 */
void insert_load(int bp_offset, int pr, ILOCInsn* prev_insn)
{
    /* create load instruction */
    ILOCInsn* new_insn = ILOCInsn_new_3op(LOAD_AI,
            base_register(), int_const(bp_offset), physical_register(pr));

    /* insert into code */
    new_insn->next = prev_insn->next;
    prev_insn->next = new_insn;
}

void spill(int pr, ILOCInsn* previous, ILOCInsn* local_allocator) {
    /* emit store from pr onto the stack at some offset x*/
    int x = insert_spill(pr, previous, local_allocator);
    offset[name[pr]] = x;
    name[pr] = INVALID;
}

int dist(Operand vr, ILOCInsn* ins)
{
    // return number of instructions until vr is next used (INFINITY if no use)
    int distance = 0;
    
    while (ins->next) {
        // for each read vr in i:
        ILOCInsn* read_regs = ILOCInsn_get_read_registers(ins);
        for (int op = 0; op < 3; op++) {
            Operand vr1 = read_regs->op[op];
            if (vr1.type == VIRTUAL_REG && vr.id == vr1.id) {
                free(read_regs);
                return distance;
            }
        }

        free(read_regs);
        ins = ins->next;
    }

    return INFINITY;
}

/* This helper method returns the index that has the max distance */
int findMaxDistance(Operand vr, int size, ILOCInsn* ins)
{
    int index = 0;
    int max = 0;

    for (int i = 0; i < size; i++) {
        int current = dist(vr, ins);
        if (current > max) {
            max = current;
            index = i;
        }
    }

    return index;
}

int allocate(Operand vr, int size, ILOCInsn* ins, ILOCInsn* previous, ILOCInsn* local_allocator)
{
    for (int pr = 0; pr < size; pr++) {
        if (name[pr] == INVALID) {                      // if there's a free register
            name[pr] = vr.id;                           // then allocate it
            return pr;                                  // and use it
        }
    }

    /* SPILLING PART */

    int pr = findMaxDistance(vr, size, ins);                     // otherwise, find register to spill
    spill(pr, previous, local_allocator);                        // spill value to stack
    name[pr] = vr.id;                                           // reallocate it
    return pr;                                                  // and use it
}

int ensure(Operand vr, int size,ILOCInsn* ins, ILOCInsn* previous, ILOCInsn* local_allocator)
{
    for (int pr = 0; pr < size; pr++) {
        if (name[pr] == vr.id) {                                            // if the vr is in a phys reg
            return pr;                                                      // then use it
        }
    }

    int pr = allocate(vr, size, ins, previous, local_allocator);            // otherwise, allocate a phys reg
    if (offset[vr.id] != INVALID)                                           // if vr was spilled, load it
        /* emit load into pr from offset[vr] */
        insert_load(offset[vr.id], pr, previous);

    return pr;                                                              // and use it
}

void allocate_registers (InsnList* list, int num_physical_registers)
{
    if (list == NULL)
        return;

    /* Set unused registers to INVALID */
    for (int i = 0; i < num_physical_registers; i++)
        name[i] = INVALID;

    ILOCInsn* previous = NULL;
    ILOCInsn* local_allocator;

    // each instruction i in program:
    FOR_EACH(ILOCInsn*, i, list) {

        /* save reference to stack allocator instruction if i is a call label */
        if (i->op->type == CALL_LABEL && i->form != CALL)
            local_allocator = i->next->next->next;

        // for each read vr in i:
        ILOCInsn* read_regs = ILOCInsn_get_read_registers(i);
        for (int op = 0; op < 3; op++) {
            Operand vr = read_regs->op[op];
            int pr = INVALID;
            if (vr.type == VIRTUAL_REG) {
                // make sure vr is in a phys reg
                pr = ensure(vr, num_physical_registers, i, previous, local_allocator);
                replace_register(vr.id, pr, i);     // change register id
            }

            int distance = dist(vr, i);
            /* If no future use, then free pr */
            name[pr] = distance == INFINITY ? INVALID : vr.id;
        }

        free(read_regs);

        // for each written vr in i:
        Operand vr = ILOCInsn_get_write_register(i);
        int pr = -1;
        if (vr.type == VIRTUAL_REG) {
            // make sure phys reg is available
            pr = allocate(vr, num_physical_registers, i, previous, local_allocator);
            replace_register(vr.id, pr, i);     // change register id
        }

        if (i->form == CALL) {
            for (int pr = 0; pr < num_physical_registers; pr++) {
                if (name[pr] != INVALID)
                    spill(pr, previous, local_allocator);
            }
        }

        previous = i;
    }
}
