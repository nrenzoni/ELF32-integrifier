#include <stdio.h>

#include "capstone.h"
#include "misc_functions.h"
#include "disas_functions.h"
#include "byte_functions.h"

void printDisas(uint8_t *data, uint32_t data_len, uint32_t base_address) {
    csh handle;
    cs_insn *insn;
    uint32_t count;

    if(cs_open(CS_ARCH_X86, CS_MODE_32, &handle) != CS_ERR_OK)
        fatal("opening capstone lib");
    count = cs_disasm(handle, data, data_len, base_address, 0, &insn);
    if(count > 0) {
        for(uint32_t i=0; i<count; i++) {
            char* p_opcodeHexStr = genHexStr(insn[i].bytes, insn[i].size);
            printf("%04lu: %-16s  %-4s  %s\n",
                    insn[i].address, // long unsigned int
                    p_opcodeHexStr,
                    insn[i].mnemonic,
                    insn[i].op_str);
            freeHexStr(p_opcodeHexStr);
        }

        cs_free(insn, count);
    }
    else
        fatal("disassembling code");

    cs_close(&handle);
}

void printJMPinstrns(uint8_t *opcode_data, uint32_t length, uint32_t base_address) {
    csh handle;
    cs_insn *insn;
    uint32_t count;
    uint32_t jmp_count = 0, call_count = 0;

    if(cs_open(CS_ARCH_X86, CS_MODE_32, &handle) != CS_ERR_OK)
        fatal("opening capstone lib");

    if(cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON )!= CS_ERR_OK) // enable optional details for locating jmps by using group
       fatal("enabling OPT_DETAIL in capstone lib");

    if((count = cs_disasm(handle, opcode_data, length, base_address, 0, &insn)) <= 0)
        fatal("disassembling code in capstone lib");

    // loop over all disassembled instructions
    for(uint32_t i=0; i<count; i++) {

        cs_detail *x86_detail = insn[i].detail;

        // loop over groups of current instruction
        for(uint32_t j=0; j< x86_detail->groups_count; j++) {

            uint8_t group = x86_detail->groups[j];

            int jmp_or_call = 0; // default false

            if(group == X86_GRP_JUMP) {
                jmp_or_call = 1;
                jmp_count++;
            }

            else if(group == X86_GRP_CALL) {
                jmp_or_call = 1;
                call_count++;
            }

            if(jmp_or_call) {
                char* p_opcodeHexStr = genHexStr(insn[i].bytes, insn[i].size);
                printf("%04lu: %-16s  %-4s  %s\n",
                       insn[i].address, // long unsigned int
                       p_opcodeHexStr,
                       insn[i].mnemonic,
                       insn[i].op_str);
                freeHexStr(p_opcodeHexStr);
                break; // out of groups of current instruction
            }

        } // groups of cur instruction

    } // all instructions

    printf("\njmp total = %u, call total = %u\n", jmp_count, call_count);

    cs_free(insn, count);
    cs_close(&handle);
}

// returns number of found jump/jcc/call instructions in given section
// and branch instrns in array pp_branch_instrns
// after using pp_branch_instrns, calling function must take care to call free(*pp_branch_instrns)
uint32_t getBranchInstrns(uint8_t *p_section, uint32_t section_length, branch_instrn **pp_branch_instrns) {
    branch_instrn *p_branch_instrns;
    csh handle;
    cs_insn *insn;
    uint32_t i, j;
    uint32_t disas_instrn_count = 0;
    uint32_t branch_instrn_count = 0;

    if(cs_open(CS_ARCH_X86, CS_MODE_32, &handle) != CS_ERR_OK)
        fatal("opening capstone lib");

    if(cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON) != CS_ERR_OK) // enable optional details for group, and x86 arch specific details
       fatal("enabling OPT_DETAIL in capstone lib");

    if((disas_instrn_count = cs_disasm(handle, p_section, section_length, 0, 0, &insn)) <= 0)
        fatal("disassembling code in capstone lib");

    // allocate array of flags to store flag if i'th instrn is a branch instrn ; calloc memory allocation initialized to 0
    uint8_t *p_is_branch_instrn_flags = calloc(1, disas_instrn_count * sizeof(uint8_t) );
    if (NULL == p_is_branch_instrn_flags)
        fatal("calloc for branch instruction structs");

#define IS_BRANCH 1

    // loop over all disassembled instructions (to find jmp/jcc/call instrns)
    for(i=0; i<disas_instrn_count; i++) {

        cs_detail *x86_detail = insn[i].detail;

        // loop over groups of current instruction
        for(j=0; j< x86_detail->groups_count; j++) {

            uint8_t group = x86_detail->groups[j];

            if(group == X86_GRP_JUMP || group == X86_GRP_CALL) {
                branch_instrn_count++;
                p_is_branch_instrn_flags[i] = IS_BRANCH; // true
                break;
            }

        } // end cur instr groups
    } // end all disassembled instrn

    // allocate to store custom data of branch instrns
    p_branch_instrns = malloc(branch_instrn_count * sizeof(branch_instrn));


    uint32_t dist_to_offset_from_beginning = 0;

    // find all branch instrns and copy to p_branch_instrns[]
    // stop loop after iterating over all disassembled instrns, or already found all branch instrns
    // i iterates over all disassembled instrns
    // j iterates over p_branch_instrns[]
    for(i=0, j=0; i<disas_instrn_count && j<branch_instrn_count; i++) {

            if(p_is_branch_instrn_flags[i] == IS_BRANCH) {

                if(insn[i].detail->x86.op_count != 1)
                    fatal("branch instrn does not contain only 1 operand"); // is this even possible Dr. Suess?

                uint8_t skip_amount_to_start_of_offset = insn[i].size - insn[i].detail->x86.operands[0].size;

                cs_x86_op operand_0 = insn[i].detail->x86.operands[0];

                p_branch_instrns[j].branch_type = checkBranchRELorABS(&insn[i]);

                x86_op_type op_type = insn[i].detail->x86.operands[0].type;

                if (X86_OP_IMM == op_type) {

                    uint64_t addrss = insn[i].address;
                    // offset value = difference between current insns address and offset stored in capstone's calculated offset
                    p_branch_instrns[j].offset_val = (operand_0.imm > addrss) ? operand_0.imm - addrss : addrss - operand_0.imm;

                    p_branch_instrns[j].offset_byte_length = operand_0.size;

                    p_branch_instrns[j].distance_to_offset_from_buf_start = dist_to_offset_from_beginning + skip_amount_to_start_of_offset;

                    switch(p_branch_instrns[j].branch_type) {

                    case ABS_IMM:
                        p_branch_instrns[j].distance_to_offset_destination_from_buf_start = operand_0.imm;
                        break;

                    case REL_IMM:
                        p_branch_instrns[j].distance_to_offset_destination_from_buf_start =
                            p_branch_instrns[j].distance_to_offset_from_buf_start + operand_0.imm; // maybe need to add size of curr instn?
                                                                                                    // is rel imm relative to next instrn?
                        break;

                    default:
                        fatal("branch insn is of type X86_OP_IMM but not ABS_IMM or REL_IMM");

                    } // end switch on branch type (REL | ABS)(IMM | REG)

                } // end if current branch instrn is X86_OP_IMM

                else if (X86_OP_MEM == op_type) {
                    // assume branches that reference MEM don't need to be corrected
                    // info stored in: insn[i]->detail->x86.opcodes[0].mem (x86_op_mem struct)
                    //fatal("branch is of type x86_OP_MEM");
                } // end if current branch instrn is x86_OP_MEM

                else if (X86_OP_REG == op_type) {
                    // assume branches that reference registers don't need to be corrected
                    //fatal("branch is of type X86_OP_REG");
                } // end if current branch instrn is X86_OP_REG

                else {
                    fatal("first operand type of branch instrn isn't of type IMM, MEM, or REG");
                }

                // increment count of found branch instrns
                j++;

            } // end if current instrn is of type branch

        // incremented even if current instrn isn't a branch instrn
        dist_to_offset_from_beginning += insn[i].size; // byte size

    } // end loop finding branch instrns

    free(p_is_branch_instrn_flags);
    cs_free(insn, disas_instrn_count);
    cs_close(&handle);

    *pp_branch_instrns = p_branch_instrns;
    return branch_instrn_count;
}

// returns REL, ABS, or NONE
uint8_t checkBranchRELorABS(cs_insn *insn) {

// can check

    switch((char)insn->bytes[0]) {

    case '\xE8':  // call REL
        return REL_IMM;
    case '\xFF': // call and jmp ABS
        return ABS_REG;
    case '\xEB': // jmp REL
        return REL_IMM;
    case '\xE9': // jmp REL
        return REL_IMM;
    case '\xEA': // jmp ABS
        return ABS_REG;

    }

    // should loop over all groups
    if (X86_GRP_JUMP == insn->detail->groups[0] && X86_OP_IMM == insn->detail->x86.operands[0].type) {
        // must be jmp conditional (which is REL)
        return REL_IMM;
    }

    // otherwise
    return NONE;
}





