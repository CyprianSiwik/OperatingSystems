#include <stdio.h>
#include "shell.h"

void fetch()
{

} 

void decode()
{

}

void execute()
{
  
}

void process_instruction()
{
  /* execute one instruction here. You should use CURRENT_STATE and modify
   * values in NEXT_STATE. You can call mem_read_32() and mem_write_32() to
   * access memory. */
  fetch();
  decode();
  execute();
}


10000917    # auipc x18, 0x10000
00090913    # addi x18, x18, 0
015ea023    # sw x21, 0(x29)
002e1e93    # slli x29, x28, 2
015a0b33    # add x22, x20, x21
013e2f33    # slt x30, x28, x19
fe0f10e3    # bne x30, x0, -32
