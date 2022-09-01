
#include <kernel/kprintf.h>
#include <kernel/thread.h>
#include <kernel/syscalls.h>
#include <arch/bsp/intr.h>
#include <arch/bsp/timer.h>
#include <arch/bsp/uart.h>
#include <arch/cpu/arm.h>
#include <arch/cpu/exceptions.h>
#include <lib/primfunc.h>
#include <user/sys.h>

#include <stdint.h>

#define IRQ_MAX_SOURCES 64
#define MODE_STR_LEN    12
#define NO_IRQ_FOUND    -1

const char *dfsr_fsrc[]=
        {
                "No function, reset value\0",
                "Alignment fault\0",
                "Debug event fault\0",
                "Access Flag fault on Section\0",
                "Cache maintenance operation fault\0",
                "Translation fault on Section\0",
                "Access Flag fault on Page\0",
                "Translation fault on Page\0",
                "Precise External Abort\0",
                "Domain fault on Section\0",
                "No function\0",
                "Domain fault on Page\0",
                "External abort on translation, first level\0",
                "Permission fault on Section\0",
                "External abort on translation, second level\0",
                "Permission fault on Page\0",
                "Imprecise External Abort\0"
        };

const char *ifsr_fsrc[]=
        {
                "No function, reset value\0",
                "No function\0",
                "Debug event fault\0",
                "Access Flag fault on Section\0",
                "No function\0",
                "Translation fault on Section\0",
                "Access Flag fault on Page\0",
                "Translation fault on Page\0",
                "Precise External Abort\0",
                "Domain fault on Section\0",
                "No function\0",
                "Domain fault on Page\0",
                "External Abort on Section\0",
                "Permission fault on Section\0",
                "External Abort on Page\0",
                "Permission fault on Page\0",
        };
const char *mode_names[N_MODES] =
{
    "User/System",
    "FIQ",
    "IRQ",
    "Supervisor",
    "Undefined",
    "Abort",
    "",
};

uint8_t enable_intr_regdump = 0;

void handle_mode(struct registers_t *reg)
{
    uint32_t cpsr, spsr, spsr_mode;
    _get_cpsr_spsr(&cpsr, &spsr);
    spsr_mode = spsr & PSR_MODE_MASK;

    if (spsr_mode == PSR_USR) {
        kprintf("\nFault occured in current thread. Thread is terminated.\n");
        terminate_current_thread(reg);
    }
    else {
        kprintf("\nFault occured in Kernel. System is halted.\n");
        while (1);
    }
}

void print_register(struct registers_t reg, uint32_t index){
    switch(index){
        case 0 ... 12 :
            kprintf("R%i: 0x%08x", (int)index , (unsigned int)reg.base_registers[index]);
            break;
        case 13:
            kprintf("SP: 0x%08x", (unsigned int)reg.sp);
            break;
        case 14:
            kprintf("LR: 0x%08x", (unsigned int)reg.lr);
            break;
        case 15:
            kprintf("PC: 0x%08x", (unsigned int)reg.pc);
            break;

    }
}

void print_flags(uint32_t psr){
    char *status_flag= "NCZV E IFT";
    uint32_t flags[]= {N_bit,C_bit,Z_bit,V_bit,0,E_bit,0,I_bit,F_bit,T_bit};
    for(int32_t i=0;i<10;i++){
        if((i!=4)&&(i!=6)){
            if((psr & (1<<flags[i])) !=0 ) 
                kprintf("%c",status_flag[i]);
            else 
                kprintf("_");
        }
        else 
            kprintf(" ");
    }
}

enum modes_t get_mode(uint32_t psr) {
    uint32_t psr_masked = psr & 0x1F;
    switch (psr_masked) {
        case PSR_USR:    return MODE_USR;
        case PSR_FIQ:    return MODE_FIQ;
        case PSR_IRQ:    return MODE_IRQ;
        case PSR_SUP:    return MODE_SVC;
        case PSR_UND:    return MODE_UND;
        case PSR_ABT:    return MODE_ABT;
        case PSR_SYS:    return MODE_USR;
        default:         return MODE_NONE;
    };
}

const char *get_modename(uint32_t psr){
    enum modes_t current_mode = get_mode(psr);
    return mode_names[current_mode];
}

void print_exception_header(char * exception_name, uint32_t cause_pc)
{
    kprintf("########################################\n");
    kprintf("%s an Adresse 0x%08x \n", exception_name, (unsigned int)cause_pc);
}

void print_mode_registers(const char * mode_name, struct mode_registers * mreg)
{
    const uint32_t mode_name_forced_len = MODE_STR_LEN;
    uint32_t mode_name_len = strlength(mode_name);
    uint32_t n_spaces = mode_name_forced_len - mode_name_len;

    kprintf("%s:", mode_name);
    for (uint32_t i=0; i<n_spaces; i++) 
        kprintf(" ");

    kprintf(" 0x%08x 0x%08x ",(unsigned int)mreg->lr, (unsigned int)mreg->sp);

    /* user mode does not have spsr -> 0xDEADDA7A marks this */
    if (mreg->spsr != 0xDEADDA7A) {
        const char *return_mode = get_modename(mreg->spsr);
        mode_name_len = strlength(return_mode);
        n_spaces = mode_name_forced_len - mode_name_len;

        print_flags(mreg->spsr);
        kprintf(" %s ", return_mode);
        for (uint32_t i=0; i<n_spaces; i++) 
            kprintf(" ");
        kprintf(" (0x%08x)", (unsigned int)mreg->spsr);
    }
    kprintf("\n");
}

void print_exception(struct registers_t *reg)
{
    struct mode_registers mreg;
    uint32_t cpsr;
    uint32_t spsr;
    enum modes_t current_mode;
    
    _get_cpsr_spsr(&cpsr, &spsr);
    current_mode = get_mode(cpsr);

    kprintf("\n>>> Registerschnappschuss (aktueller Modus) <<<\n");
    for(int32_t i=0 ; i<8 ; i++){
        print_register(*reg,i);
        kprintf("    ");
        print_register(*reg,i+8);
        kprintf("\n");
    }

    kprintf("\n>>> Aktuelle Statusregister (SPSR des aktuellen Modus) <<<\n");

    
    kprintf("CPSR: ");
    print_flags(cpsr);
    kprintf(" %s   (0x%08x)\n", get_modename(cpsr),(unsigned int)cpsr);
    kprintf("SPSR: ");
    print_flags(spsr);
    kprintf(" %s   (0x%08x)\n", get_modename(spsr),(unsigned int)spsr);

    kprintf("\n>>> Aktuelle modusspezifische Register <<<\n");
    kprintf("             LR         SP         SPSR \n");
   
    GET_MODE_REG(MODE_USR, _get_regs_usr);
    print_mode_registers(mode_names[MODE_USR], &mreg);
    GET_MODE_REG(MODE_SVC,_get_regs_svc);
    print_mode_registers(mode_names[MODE_SVC], &mreg);
    GET_MODE_REG(MODE_ABT, _get_regs_abt);
    print_mode_registers(mode_names[MODE_ABT], &mreg);
    GET_MODE_REG(MODE_FIQ, _get_regs_fiq);
    print_mode_registers(mode_names[MODE_FIQ], &mreg);
    GET_MODE_REG(MODE_IRQ, _get_regs_irq);
    print_mode_registers(mode_names[MODE_IRQ], &mreg);
    GET_MODE_REG(MODE_UND, _get_regs_und);
    print_mode_registers(mode_names[MODE_UND], &mreg);
}

void undefined_instruction(struct registers_t *reg)
{
    uint32_t cause_pc = reg->lr - UND_LR_OFFSET + UND_LR_CORRECTION;

    kprintf("########################################\n");
    kprintf("Undefined instruction an Adresse 0x%08x \n", (unsigned int)cause_pc);

    print_exception(reg);
    handle_mode(reg);
}

void software_interrupt(struct registers_t *reg)
{
    uint32_t cause_pc = reg->lr - SVC_LR_OFFSET + SVC_LR_CORRECTION;
    uint8_t svc_error = 0;

    // only allow software interrupts during user mode
    uint32_t cpsr, spsr, spsr_mode;
    _get_cpsr_spsr(&cpsr, &spsr);
    spsr_mode = spsr & PSR_MODE_MASK;

    if (spsr_mode == PSR_USR) {
        /*  Supervisor Code is:
            lower 3 bytes in ARM mode
            lower 1 byte in Thumb mode
            Less than 256 different codes are used, so always 
            only the lowest byte is interpreted as supvervisor code

            If svc code does not exist, svc call is treated as error
        */
        #define SVC_CODE_MASK   0xFF
        uint32_t svc_code = SVC_CODE_MASK & *((uint32_t*) cause_pc);

        svc_error = process_svc_code(svc_code, reg);
    }
    else {
        svc_error = 1;
    }

    if (svc_error) {
        kprintf("########################################\n");
        kprintf("Software interrupt an Adresse 0x%08x \n", (unsigned int)cause_pc);

        print_exception(reg);
        handle_mode(reg);
    }
}

void prefetch_abort(struct registers_t *reg)
{
    uint32_t cause_pc = reg->lr - PREF_ABT_LR_OFFSET + PREF_ABT_LR_CORRECTION; 
    uint32_t fault_status;
    uint32_t fault_address;
    _get_fault_registers(&fault_status, &fault_address);

    kprintf("########################################\n");
    kprintf("Prefetch Abort an Adresse 0x%08x \n", (unsigned int)cause_pc);
    kprintf("Zugriff: Fehler an Adresse 0x%08x\n",(unsigned int)fault_address);
    if ((fault_status &(1<<(STATUS_4BIT-1)))!=0){ // im IFSR hat der 4.Statusbit ein 9 Offset statt 10
        kprintf("Fehler: No function");
    }
    fault_status &= 0xF; // get Status [0..3] bits
    kprintf("Fehler: %s\n", ifsr_fsrc[fault_status]);

    print_exception(reg);
    handle_mode(reg);
}

void data_abort(struct registers_t *reg)
{
    uint32_t cause_pc = reg->lr - DATA_ABT_LR_OFFSET + DATA_ABT_LR_CORRECTION;    
    uint32_t fault_status;
    uint32_t fault_address;
    _get_fault_registers(&fault_status, &fault_address);

    kprintf("########################################\n");
    kprintf("Data Abort an Adresse 0x%08x \n", (unsigned int)cause_pc);

    uint32_t  rw= fault_status & (1<<RW_OFFSET); //1=write 0=read
    char *rw_str[2]={"lesend", "schreibend"};
    if (rw>0)
        rw=1;

    kprintf("Zugriff: %s auf Adresse 0x%08x\n",rw_str[rw], (unsigned int)fault_address);
    if ((fault_status &(1<<STATUS_4BIT))!=0){
        if(fault_status==IMP_EX_ABT){
            kprintf("Fehler:  %s\n", dfsr_fsrc[16]);
        }
        else kprintf("Fehler: %s\n",dfsr_fsrc[10]);
    }
    fault_status &= 0xF; // get Status [0..3] bits
    kprintf("Fehler: %s\n", dfsr_fsrc[fault_status]);

    print_exception(reg);
    handle_mode(reg);
}

void unused_handler(struct registers_t *reg)
{
    (void) reg;
    kprintf ("UNUSED handler called\n");
}

void reset(struct registers_t *reg)
{
    (void) reg;
    kprintf ("Reset handler called\n");
}

void irq(struct registers_t *reg)
{
    if (enable_intr_regdump) {
        uint32_t cause_pc = reg->lr - IRQ_LR_OFFSET + IRQ_LR_CORRECTION; 
        kprintf("########################################\n");
        kprintf("IRQ an Adresse 0x%08x \n", (unsigned int)cause_pc);
        print_exception(reg);
    }

    /* Find interrupt source */
    const int32_t POSSIBLE_IRQS[] = {
        IRQ_TIMER_BASE + 0, // lowest timer
        IRQ_TIMER_BASE + 1,
        IRQ_TIMER_BASE + 2,
        IRQ_TIMER_BASE + 3, // highest timer
        IRQ_UART
    };
    const uint32_t N_IRQS = sizeof(POSSIBLE_IRQS) / sizeof(POSSIBLE_IRQS[0]);
    int32_t irq_src = NO_IRQ_FOUND;
    uint32_t pending_intrs[2];
    interrupt_get_pending(pending_intrs);

    for (uint32_t i=0; i<N_IRQS; i++) {
        /* 64 register bits are split into 2x32 bits */
        uint32_t reg_num = POSSIBLE_IRQS[i]/32;
        /*  register 1: map i=0...31 to 0...31
            register 2: map i=32...63 to 0...31 */
        uint32_t irq_reg = POSSIBLE_IRQS[i]-reg_num*32;

        uint8_t current_is_pending = (pending_intrs[reg_num] >> irq_reg) & 0x1;

        if (current_is_pending) {
            irq_src = POSSIBLE_IRQS[i];
        }
    }

    if (irq_src == NO_IRQ_FOUND)
        return;

    /* IRQ handlers */
    switch(irq_src) {
        case IRQ_TIMER_BASE ... (IRQ_TIMER_BASE + NUM_TIMERS - 1):
            timer_intr_h(reg);
            break;
        case IRQ_UART:
            uart_intr_h(reg);
            break;
        default:
            /* no handler for interrupt is defined */
            break;
    }
}

void fiq(struct registers_t *reg)
{
    if (enable_intr_regdump) {
        uint32_t cause_pc = reg->lr - FIQ_LR_OFFSET + FIQ_LR_CORRECTION;  
        kprintf("########################################\n");
        kprintf("FIQ an Adresse 0x%08x \n", (unsigned int)cause_pc);
        print_exception(reg);
    }
}

