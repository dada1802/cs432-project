/**
 * @file p5-regalloc.c
 * @brief Compiler phase 5: register allocation
 */
#include "p5-regalloc.h"

int name[MAX_PHYSICAL_REGS];
int offset[MAX_VIRTUAL_REGS];
#define INFINITY -2
#define INVALID -1

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

int ensure(Operand vr, int size)
{
    for (int pr = 0; pr < size; pr++) {
        if (name[pr] == vr.id) {                    // if the vr is in a phys reg
            return pr;                              // then use it
        }
    // else
    //     pr = allocate(vr)                       // otherwise, allocate a phys reg
    //     if offset[vr] is valid:                 // if vr was spilled, load it
    //         emit load into pr from offset[vr]
    //     return pr                               // and use it
    }
    
    return -1;
}

int allocate(Operand vr, int size)
{
    for (int pr = 0; pr < size; pr++) {
        if (name[pr] == INVALID) {                       // if there's a free register
            name[pr] = vr.id;                       // then allocate it
            return pr;                              // and use it
        }
    }
//     else:
//         find pr that maximizes dist(name[pr])   // otherwise, find register to spill
//         spill(pr)                               // spill value to stack
//         name[pr] = vr                           // reallocate it
//         return pr                               // and use it
    return -1;
}

double dist(Operand vr, int start, InsnList* list)
{
    // return number of instructions until vr is next used (INFINITY if no use)
    int current = 0;
    bool check = false;

    FOR_EACH(ILOCInsn*, i, list) {
        // Save reference to stack allocator instruction if i is a call label.
        if (i->form == CALL) {

        }

        // for each read vr in i:
        ILOCInsn* read_regs = ILOCInsn_get_read_registers(i);
        for (int op = 0; op < 3; op++) {
            Operand vr1 = read_regs->op[op];
            if (vr1.type == VIRTUAL_REG && current > start && vr.id == vr1.id) {
                check = true;
                break;
            }
        }

        free(read_regs);

        if (check) {
            break;
        }

        current++;
    }

    if (check) {
        return current;
    }

    return INFINITY;
}

// void spill(int pr) {
//     emit store from pr onto the stack at some offset x
//     offset[name[pr]] = x
//     name[pr] = INVALID
// }

void allocate_registers (InsnList* list, int num_physical_registers)
{
    if (list == NULL) {
        return;
    }

    int current = 0;

    /* Set unused registers to INVALID */
    for (int i = 0; i < num_physical_registers; i++) {
        name[i] = INVALID;
    }

    // each instruction i in program:
    FOR_EACH(ILOCInsn*, i, list) {
        // for each read vr in i:
        ILOCInsn* read_regs = ILOCInsn_get_read_registers(i);
        for (int op = 0; op < 3; op++) {
            Operand vr = read_regs->op[op];
            int pr = INVALID;
            if (vr.type == VIRTUAL_REG) {
                // make sure vr is in a phys reg
                pr = ensure(vr, num_physical_registers);
                replace_register(vr.id, pr, i);     // change register id
            }

            if (dist(vr, current, list) == INFINITY) {          // if no future use
                name[pr] = INVALID;                                  // then free pr
            }
        }

        free(read_regs);

        // for each written vr in i:
        Operand vr = ILOCInsn_get_write_register(i);
        int pr = -1;
        if (vr.type == VIRTUAL_REG) {
            // make sure phys reg is available
            pr = allocate(vr, num_physical_registers);
            replace_register(vr.id, pr, i);     // change register id
        }

        current++;
    }
}
