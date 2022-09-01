#ifndef MM_H
#define MM_H

/* 
    Memory Map 
*/

/* Peripherial Management */
#define PERIPH_OFFSET 0x3F000000

/* Stack Management */
#define STACK_ALIGNMENT 8
#define ALIGN_SP(sp)     ( (sp / STACK_ALIGNMENT) * STACK_ALIGNMENT)

/* kernel stacks (max 1mb) */
#define STACK_SIZE_DEFAULT  0x10000
#define STACK_SIZE_FIQ  STACK_SIZE_DEFAULT
#define STACK_SIZE_IRQ  STACK_SIZE_DEFAULT 
#define STACK_SIZE_SVC  STACK_SIZE_DEFAULT
#define STACK_SIZE_UND  STACK_SIZE_DEFAULT
#define STACK_SIZE_ABT  STACK_SIZE_DEFAULT

/* User Stack Management (0x0B001 - 0x1B000) */
#define STACK_SIZE_THREAD   0x800 

#endif // MM_H