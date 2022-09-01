#include <stdint.h>
#include <config.h>
#include <kernel/syscalls.h>
#include <kernel/thread.h>
#include <kernel/kprintf.h>
#include <arch/bsp/uart.h>
#include <arch/bsp/intr.h>
#include <arch/bsp/timer.h>
#include <arch/bsp/mmu.h>

#define BASE_ADDR_OFF 20
#define BASE_ADDR_L2_OFF 12

#define BASE_ADDR_MASK_SECT  0xFFF00000
#define BASE_ADDR_MASK_L2    0xFFFFFC00
#define BASE_ADDR_MASK_SP    0xFFFFF000

#define SECTION_OFFSET  0
#define SECTION_UNUSED  0
#define SECTION_L2      1
#define SECTION_ENTRY   2
#define SECTION_SMALL_PAGE 2

#define RIGHT_OFF_L1_01 10
#define RIGHT_OFF_L1_2  15
#define RIGHT_OFF_L2_01 4
#define RIGHT_OFF_L2_2  9

#define DOMAIN_NO_ACCESS    0
#define DOMAIN_PERMISSION   1
#define DOMAIN_NO_CHECKS    3

#define EXEC_NEVER_L1_SEC_OFF 4
#define EXEC_NEVER_L2_SEC_OFF 0
#define EXEC_NEVER_PRIV_L1_SEC_OFF 0
#define EXEC_NEVER_PRIV_L1_REF_OFF 2

#define SCTLR_MMU_BIT   0
#define SCTLR_CACHE_BIT 2

extern uint32_t L1_PAGE_SIZE;
extern uint32_t _init_start;
extern uint32_t _init_end;
extern uint32_t _text_kernel_start;
extern uint32_t _text_kernel_end;
extern uint32_t _bss_kernel_start;
extern uint32_t _bss_kernel_end;
extern uint32_t _data_kernel_start;
extern uint32_t _data_kernel_end;
extern uint32_t _ram_kernel_start;
extern uint32_t _ram_kernel_end;
extern uint32_t _orig_globals_start;
extern uint32_t _orig_globals_end;
extern uint32_t _phys_ram_user_start;
extern uint32_t _phys_ram_user_end;
extern uint32_t _text_user_start;
extern uint32_t _text_user_end;
extern uint32_t _ram_user_start;
extern uint32_t _ram_user_end;
#define LINKER2VAL_ALIGN(linker)    (( LINKER2VAL(linker) / LINKER2VAL(L1_PAGE_SIZE)) * LINKER2VAL(L1_PAGE_SIZE) )

#define L1_SIZE 4096
#define L1_MEM (4096*4)

__attribute__((aligned(L1_MEM))) uint32_t L1[L1_SIZE];

void mmu_init()
{
    // generate L1
    for(uint32_t i=0; i<L1_SIZE ; i++) {
        uint32_t virt_adr = i << BASE_ADDR_OFF;
        uint32_t phy_adr = virt_adr;
        // allow nothing per default
        uint32_t right = RIGHT_NO_ACCESS;
        uint8_t xn[] = {1,1}; // {execute never, priviliged xn}
        
        /** Kernel **/
        // text
        if((virt_adr >= LINKER2VAL_ALIGN(_text_kernel_start)) && (virt_adr <= LINKER2VAL_ALIGN(_text_kernel_end))) {
            right = RIGHT_SYS_READ_ONLY;
            xn[0] = 0;
            xn[1] = 0;
        }
            
        // bss + data + ram
        else if((virt_adr >= LINKER2VAL_ALIGN(_bss_kernel_start)) && (virt_adr <= LINKER2VAL_ALIGN(_ram_kernel_end))) {
            right = RIGHT_SYS_ONLY;
        }

        // address space for kernel to access original user space
        else if((virt_adr >= LINKER2VAL_ALIGN(_orig_globals_start)) && (virt_adr <= LINKER2VAL_ALIGN(_orig_globals_end))) {
            right = RIGHT_SYS_READ_ONLY;
            phy_adr = LINKER2VAL_ALIGN(_ram_user_start);
        }

        // user physical ram adresses
        else if((virt_adr >= LINKER2VAL_ALIGN(_phys_ram_user_start)) && (virt_adr <= LINKER2VAL_ALIGN(_phys_ram_user_end))) {
            right = RIGHT_SYS_ONLY;
        }

        /** User **/
        // text
        else if((virt_adr >= LINKER2VAL_ALIGN(_text_user_start)) && (virt_adr <= LINKER2VAL_ALIGN(_text_user_end))) {
            right = RIGHT_BOTH_READ_ONLY;
            xn[0] = 0;
        }
        // permission for user ram is granted later, when threads are initialized

        /** Peripherials **/
        else if (
            (virt_adr == (TIMER_BASE & BASE_ADDR_MASK_SECT)) ||
            (virt_adr == (UART_BASE & BASE_ADDR_MASK_SECT)) ||
            (virt_adr == (INTR_BASE & BASE_ADDR_MASK_SECT))
            ) 
        {
            right = RIGHT_SYS_ONLY;
        }

        L1_init(phy_adr, virt_adr, right, 0, xn);
    }

    // put L1 in the TTBR0 reg
    asm("mrc p15, 0, r0, c2, c0, 0" ::: "r0");
    asm("orr r0, r0, %0" :: "r" (L1) : "r0");
    asm("mcr p15, 0, r0, c2, c0, 0");

    // set domain access DACR
    uint32_t domain = 0;
    uint32_t dacr = (DOMAIN_PERMISSION << domain);
    asm("mrc p15, 0, r0, c3, c0, 0" ::: "r0");
    asm("orr r0, r0, %0" :: "r" (dacr) : "r0");
    asm("mcr p15, 0, r0, c3, c0, 0");

    // Activation of 32-Bit Translation in TTBCR
    uint32_t ttbcr = 0;
    asm("mcr p15, 0, %0, c2, c0, 2" :: "r" (ttbcr));

    // disable caches and enable MMU
    uint32_t sctlr = 0;
    asm("mrc p15, 0, %0, c1, c0, 0" : "+r" (sctlr));
    sctlr |= (1 << SCTLR_MMU_BIT);
    sctlr &= ~(1 << SCTLR_CACHE_BIT);
    asm("mcr p15, 0, %0, c1, c0, 0" :: "r" (sctlr));
}

void L1_init(uint32_t phy_adr, uint32_t vir_adr, uint32_t right, uint8_t isL2, 
    uint8_t xn[] /* execute never / priviliged xn */)
{
    uint32_t L1_entry = (isL2 ? SECTION_L2 : SECTION_ENTRY);

    uint32_t base_addr_mask = ( isL2 ? BASE_ADDR_MASK_L2 : BASE_ADDR_MASK_SECT);
    L1_entry |= (phy_adr & base_addr_mask);

    if (isL2) {
        L1_entry |= (xn[1] << EXEC_NEVER_PRIV_L1_REF_OFF);
    }
    else {
        L1_entry |= ((right & 0b11) << RIGHT_OFF_L1_01); // Set lower two bits of acces rights
        L1_entry |= ((right >> 2) << RIGHT_OFF_L1_2); // Set upper bit of access rights

        L1_entry |= (xn[0] << EXEC_NEVER_L1_SEC_OFF);
        L1_entry |= (xn[1] << EXEC_NEVER_PRIV_L1_SEC_OFF);
    }

    uint32_t index = vir_adr >> BASE_ADDR_OFF;
    L1[index] = L1_entry;
}

// returns an entry for an L2 table
uint32_t L2_init(uint32_t phy_adr, uint32_t right, uint8_t isGuard, uint8_t xn /* execute never */)
{
    uint32_t L2_entry = (isGuard ? SECTION_UNUSED : SECTION_SMALL_PAGE);

    if (!isGuard) {
        L2_entry |= (phy_adr & BASE_ADDR_MASK_SP);

        L2_entry |= ((right & 0b11) << RIGHT_OFF_L2_01); // Set lower two bits of acces rights
        L2_entry |= ((right >> 2) << RIGHT_OFF_L2_2); // Set upper bit of access rights

        L2_entry |= (xn << EXEC_NEVER_L2_SEC_OFF); // Set upper bit of access rights
    }

    return L2_entry;
}

void print_L_table(uint32_t table[], uint32_t n_entries)
{
    kprintf("#### Table at %p ####\n", table);

    for (uint32_t i=0; i<n_entries; i++)
        kprintf("%4x\t %08x\n", (unsigned int) i, (unsigned int) table[i]);

    kprintf("#################################\n");
}