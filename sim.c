#include <stdio.h>
#include <stdint.h>
#include "shell.h"

// Function prototypes
void fetch();
void decode();
void execute();

uint32_t instruction; // Holds the fetched instruction
uint32_t opcode, funct3, funct7, rs1, rs2, rd, imm; // Decoded fields

void fetch() {
    // Fetch instruction from memory at PC
    instruction = mem_read_32(CURRENT_STATE.PC);
    
    // Advance PC for next instruction (4 bytes for 32-bit RISC-V)
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
}

void decode() {
    // Extract opcode (lower 7 bits)
    opcode = instruction & 0x7F;
    
    // Extract rd (bits 7-11)
    rd = (instruction >> 7) & 0x1F;
    
    // Extract funct3 (bits 12-14)
    funct3 = (instruction >> 12) & 0x7;
    
    // Extract rs1 (bits 15-19)
    rs1 = (instruction >> 15) & 0x1F;
    
    // Extract rs2 (bits 20-24)
    rs2 = (instruction >> 20) & 0x1F;
    
    // Extract funct7 (bits 25-31)
    funct7 = (instruction >> 25) & 0x7F;
    
    // Extract immediate values depending on instruction type
    switch (opcode) {
        case 0x13:  // I-type (addi, slli)
            imm = (instruction >> 20) & 0xFFF;
            // Sign extend if needed
            if (imm & 0x800) 
                imm |= 0xFFFFF000;
            break;
            
        case 0x23:  // S-type (sw)
            imm = ((instruction >> 25) & 0x7F) << 5 | ((instruction >> 7) & 0x1F);
            // Sign extend if needed
            if (imm & 0x800) 
                imm |= 0xFFFFF000;
            break;
            
        case 0x63:  // SB-type (bne)
            imm = ((instruction >> 31) << 12) | 
                  (((instruction >> 7) & 0x1) << 11) | 
                  (((instruction >> 25) & 0x3F) << 5) | 
                  (((instruction >> 8) & 0xF) << 1);
            // Sign extend if needed
            if (imm & 0x1000) 
                imm |= 0xFFFFE000;
            break;
            
        case 0x17:  // U-type (auipc)
            imm = instruction & 0xFFFFF000; // Upper 20 bits
            break;
            
        case 0x6F:  // UJ-type (jal)
            imm = ((instruction >> 31) << 20) | 
                  (((instruction >> 12) & 0xFF) << 12) |
                  (((instruction >> 20) & 0x1) << 11) | 
                  (((instruction >> 21) & 0x3FF) << 1);
            // Sign extend if needed
            if (imm & 0x100000) 
                imm |= 0xFFE00000;
            break;
    }
}

void execute() {
    // Copy over current register state to next state
    // This ensures registers that aren't modified maintain their values
    for (int i = 0; i < RISCV_REGS; i++) {
        NEXT_STATE.REGS[i] = CURRENT_STATE.REGS[i];
    }

    // Check for no instruction (all zeros) or addi x0, x0, 0 (nop)
    if (instruction == 0) {
        RUN_BIT = 0;
        return;
    }

    switch (opcode) {
        case 0x33: // R-type (add, slt)
            if (funct3 == 0x0 && funct7 == 0x00) { // ADD
                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] + CURRENT_STATE.REGS[rs2];
            } else if (funct3 == 0x2 && funct7 == 0x00) { // SLT
                NEXT_STATE.REGS[rd] = ((int32_t)CURRENT_STATE.REGS[rs1] < (int32_t)CURRENT_STATE.REGS[rs2]) ? 1 : 0;
            }
            break;
            
        case 0x13: // I-type (addi, slli)
            if (funct3 == 0x0) { // ADDI
                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] + (int32_t)imm;
            } else if (funct3 == 0x1) { // SLLI
                NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs1] << (imm & 0x1F);
            }
            break;
            
        case 0x23: // S-type (sw)
            mem_write_32(CURRENT_STATE.REGS[rs1] + imm, CURRENT_STATE.REGS[rs2]);
            break;
            
        case 0x63: // SB-type (bne)
            if (CURRENT_STATE.REGS[rs1] != CURRENT_STATE.REGS[rs2]) {
                NEXT_STATE.PC = CURRENT_STATE.PC + imm; // Branch taken
            }
            break;
            
        case 0x17: // U-type (auipc)
            NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + imm;
            break;
            
        case 0x6F: // UJ-type (jal)
            NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + 4; // Store return address
            NEXT_STATE.PC = CURRENT_STATE.PC + imm; // Jump
            break;
            
        default:
            printf("Unsupported instruction: 0x%08x\n", instruction);
            RUN_BIT = 0; // Stop execution if an invalid instruction is encountered
            break;
    }
    
    // Ensure x0 is always 0
    NEXT_STATE.REGS[0] = 0;
}

void process_instruction() {
    /* execute one instruction here. You should use CURRENT_STATE and modify
     * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
     * access memory. */
    fetch();
    decode();
    execute();
}
