#ifndef MMU_H
#define MMU_H

#include <stdint.h>

#define RIGHT_NO_ACCESS 0
#define RIGHT_SYS_ONLY 1
#define RIGHT_SYS_READ_ONLY 5
#define RIGHT_BOTH_READ_ONLY 7
#define RIGHT_READ_ONLY 2 // (priviliged can write)
#define RIGHT_FULL_ACCESS 3

#define L2_SIZE 256
#define L2_PAGE_SIZE    0x1000

#define LINKER2VAL(linker)  ((uint32_t) &linker)

void mmu_init(void);
void L1_init(uint32_t phy_adr, uint32_t vir_adr, uint32_t right, uint8_t isL2, uint8_t xn[] /* execute never */);
uint32_t L2_init(uint32_t phy_adr, uint32_t right, uint8_t isGuard, uint8_t xn /* execute never */);

void print_L_table(uint32_t table[], uint32_t n_entries);

#endif


