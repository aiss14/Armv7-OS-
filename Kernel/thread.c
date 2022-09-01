#include <stdint.h>
#include <config.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <arch/cpu/arm.h>
#include <arch/cpu/exceptions.h>
#include <arch/cpu/mm.h>
#include <arch/bsp/timer.h>
#include <arch/bsp/regcheck.h>
#include <arch/bsp/uart.h>
#include <arch/bsp/mmu.h>
#include <lib/primfunc.h>
#include <lib/time.h>
#include <user/sys.h>
#include <user/userthread.h>

extern uint32_t L1_PAGE_SIZE;
extern uint32_t _orig_globals_start;
extern uint32_t _bss_user_start;
extern uint32_t _data_user_end;
extern uint32_t _ram_user_start;
extern uint32_t _ram_user_end;
extern uint32_t _phys_ram_user_start;

#define MAX_THREADS     32
#define N_L2_TABLES     MAX_THREADS

#define USR_DEFAULT_CPSR  PSR_USR
#define NO_THREAD   ((volatile struct list_elem_t *) 0)
#define NO_TCB      ((volatile struct tcb_t *) 0)
#define NO_CONTEXT   ((struct registers_t *) 0)

enum thread_state_t {READY, RUNNING, WAITING, TERMINATED};

struct list_elem_t {
    volatile struct list_elem_t *prev;
    volatile struct list_elem_t *next;
};

struct tcb_t {
    struct  list_elem_t rq;
    struct  context_t context;
    enum    thread_state_t state;
    volatile struct  tcb_t * next_sleeping;
    time_t  wake_at;
    int32_t stack_i;
    int32_t L2_table_i;
};

volatile struct tcb_t tcbs[MAX_THREADS];
volatile struct list_elem_t * volatile runqueue = NO_THREAD;
volatile struct tcb_t * volatile sleepqueue = NO_TCB;
volatile struct tcb_t * volatile char_thread = NO_TCB;  // this thread will get the incoming char

/* L2 tables must be 1024(=0x400) Byte aligned in order to store
the L2 pointers in L1 table */
__attribute__((aligned(0x400))) uint32_t L2_Tables[N_L2_TABLES][L2_SIZE];
uint32_t L2_Table_references[N_L2_TABLES]; // stores number of references on L2 table

/*
Private function declarations
*/
void scheduler(void * arg);
void wake_threads(void);

void init_threads()
{
    for (uint32_t i=0; i<MAX_THREADS; i++) {
        volatile struct tcb_t *tcb = &(tcbs[i]);

        tcb->state = TERMINATED;
    }
}

void start_scheduling()
{
    setup_timer(SCHEDULER_TIMER, TIMER_INTERVAL, &scheduler);
}

void add_tcb_to_runqueue(volatile struct tcb_t * tcb)
{
    if (runqueue == NO_THREAD) {
        runqueue = &(tcb->rq);
        tcb->rq.prev = &(tcb->rq);
        tcb->rq.next = &(tcb->rq);
    }
    else {
        tcb->rq.prev = runqueue;
        tcb->rq.next = runqueue->next;

        runqueue->next->prev = &(tcb->rq);
        runqueue->next = &(tcb->rq);
    }
}

void remove_tcb_from_runqueue(volatile struct tcb_t * tcb)
{
    /* remove from runqueue */
    if (tcb->rq.prev == &(tcb->rq)) {   // is true if its the only task on runque
        runqueue = NO_THREAD;
    }
    else {
        tcb->rq.prev->next = tcb->rq.next;
        tcb->rq.next->prev = tcb->rq.prev;
    }
}

int32_t get_terminated_thread()
{
    for (uint32_t i=0; i<MAX_THREADS; i++) {
        if (tcbs[i].state == TERMINATED)
            return i;
    }
    return -1;
}

int32_t get_free_L2_table()
{
    for (uint32_t i=0; i<N_L2_TABLES; i++) {
        if (L2_Table_references[i] == 0)
            return i;
    }
    return -1;
}

struct tcb_t * get_current_thread()
{
    return (struct tcb_t*) runqueue;
}

void terminate_thread(struct tcb_t * tcb, struct registers_t *reg)
{
    remove_tcb_from_runqueue(tcb);
    tcb->state = TERMINATED;
    L2_Table_references[tcb->L2_table_i]--;
    L2_Tables[tcb->L2_table_i][tcb->stack_i] = 0;
    scheduler(reg);
}

void terminate_current_thread(struct registers_t *reg)
{
    struct tcb_t *current_thread = get_current_thread();
    terminate_thread(current_thread, reg);
}

void L1_table_update(uint32_t *L2_table)
{
    uint8_t L1_xn[] = {1,1}; // Stacks shall never be executed
    L1_init((uint32_t) L2_table, LINKER2VAL(_ram_user_start), 0, 1, L1_xn); // Pointer to L2 in L1 table

    asm("DSB"); // ensures visibility of the data cleaned from the D Cache
    asm("MCR p15,0,r5,c8,c3,0" ::: "r5"); // invalidate entire unified TLB Inner Shareable
    asm("MCR p15,0,r5,c8,c5,0" ::: "r5"); // invalidate entire instruction TLB
    asm("MCR p15,0,r5,c8,c6,0" ::: "r5"); // invalidate entire data TLB
    asm("MCR p15,0,r5,c8,c7,0" ::: "r5"); // invalidate entire unified TLB
    asm("DSB"); // ensure completion of the Invalidate TLB operation
    asm("DSB"); // ensure taböe changes visible to instruction fetch
}

uint32_t* get_thread_ram_start(volatile struct tcb_t *tcb)
{
    return &_phys_ram_user_start + LINKER2VAL(L1_PAGE_SIZE)*tcb->L2_table_i;
}


uint32_t phys2virt_adr(uint32_t phys_adr, volatile struct tcb_t *tcb)
{
    return phys_adr - (uint32_t)get_thread_ram_start(tcb) + LINKER2VAL(_ram_user_start);
}

uint32_t virt2phys_adr(uint32_t virt_adr, volatile struct tcb_t *tcb)
{
    return (uint32_t)get_thread_ram_start(tcb) - LINKER2VAL(_ram_user_start) + virt_adr;
}

void copy_globals(volatile struct tcb_t *tcb)
{
    uint32_t source = LINKER2VAL(_orig_globals_start);
    uint32_t dest = virt2phys_adr(LINKER2VAL(_bss_user_start), tcb);
    uint32_t size = ((int32_t) &_data_user_end - (int32_t) &_bss_user_start);
    kmemcpy((void*) dest, (void*) source, size);

    // set L2 entries for globals
    uint32_t *L2_table = L2_Tables[tcb->L2_table_i];
    uint8_t L2_xn = 1; // data shall not be executed
    uint32_t chunks_used = size / L2_PAGE_SIZE + (size%L2_PAGE_SIZE ? 1:0);
    for (uint32_t i=0; i<chunks_used; i++) {
        uint32_t phy_adr = (uint32_t)dest + i*L2_PAGE_SIZE;
        L2_table[i] = L2_init(phy_adr, RIGHT_FULL_ACCESS, 0, L2_xn);
    }        
}

/* Returns stack base for new thread in physical adress space */
uint32_t get_stack_base(volatile struct tcb_t *tcb)
{
    /* figure out how many entries in tcb are blocked by data */
    uint32_t bytes_blocked = (uint32_t)(&_data_user_end - &_bss_user_start);
    uint32_t pages_blocked = bytes_blocked / L2_PAGE_SIZE + (bytes_blocked%L2_PAGE_SIZE ? 1:0);

    uint32_t thread_ram_start = (uint32_t)get_thread_ram_start(tcb);
    uint32_t stack_base = 0;

    /* The stack base is found by traversing the address space for
    this process. A forbidden page is interpreted as free */
    uint32_t *L2_table = L2_Tables[tcb->L2_table_i];
    tcb->stack_i = -1;
    // even entries are guard pages; uneven entries are potential hits
    for (uint32_t i=L2_SIZE-2; i>pages_blocked; i-=2) {
        if (L2_table[i] == 0) {
            tcb->stack_i = i;

            // Allow access to stack in L2 table
            uint8_t L2_xn = 1; // stack shall not be executed
            stack_base = thread_ram_start + (tcb->stack_i+1)*L2_PAGE_SIZE; // add 1 because stack grows downwards
            L2_table[i] = L2_init(stack_base - 1, RIGHT_FULL_ACCESS, 0, L2_xn); 

            break;
        }
    }
    if (tcb->stack_i == -1)
        return -1;
    else
        return stack_base;
}

void kthread_create(struct registers_t *reg, void(*func)(void*), const void *args, uint32_t args_size, uint8_t is_proc)
{
    /* get free tcb */
    int32_t tcb_num = get_terminated_thread();
    if (tcb_num == -1) {
        WARN("No terminated thread found. New thread will not be created.");
        return;
    }
    volatile struct tcb_t *tcb = &(tcbs[tcb_num]);

    /* get L2 table */
    if (is_proc) {
        tcb->L2_table_i = get_free_L2_table();
        if (tcb->L2_table_i == -1) {
            WARN("No free L2 table entry found. New Process will not be created.");
            return;
        }
        L2_Table_references[tcb->L2_table_i] = 1;
        for (uint32_t i=0; i<L2_SIZE; i++)
            L2_Tables[tcb->L2_table_i][i] = 0;  // ensure all pages are set to guard pages
        copy_globals(tcb);
    }
    else {
        struct tcb_t *current_thread = get_current_thread();
        tcb->L2_table_i = current_thread->L2_table_i;
        L2_Table_references[tcb->L2_table_i]++;
    }
    
    uint32_t stack_base = get_stack_base(tcb);
    
    /* put args at beginning of stack */
    const uint32_t args_dest = stack_base - args_size;
    kmemcpy((void*) args_dest, args, args_size);
    // previous operations with stack were done using the physical adress which
    // is only accessible to kernel. For the thread, it must be translated to virtual space.
    uint32_t sp = ALIGN_SP(phys2virt_adr(args_dest, tcb)); 

    tcb->context.base_registers[0] = phys2virt_adr(args_dest, tcb); // argument of thread entry function
    tcb->context.sp = sp;

    /* prepare rest of registers */
    tcb->context.pc = (uint32_t) func;
    tcb->context.lr = (uint32_t) &exit;
    tcb->context.cpsr = USR_DEFAULT_CPSR;
    tcb->state = READY;

    /* schedule thread */
    add_tcb_to_runqueue(tcb);
    if (reg != NO_REGISTERS)
        scheduler(reg);
}

void store_context(struct registers_t * reg)
{
    struct tcb_t *current_thread = get_current_thread();
    if (current_thread != NO_TCB) {
        struct mode_registers usr_registers;
        uint32_t spsr_irq, cpsr;
        _get_regs_usr(&usr_registers);
        _get_cpsr_spsr(&cpsr, &spsr_irq);

        current_thread->context.sp = usr_registers.sp;
        current_thread->context.lr = usr_registers.lr;
        current_thread->context.pc = reg->lr;
        for (uint32_t i=0; i<NUM_REGISTERS; i++)
            current_thread->context.base_registers[i] = reg->base_registers[i];
        current_thread->context.cpsr = spsr_irq;

        current_thread->state = READY;
    }
}

void load_context(struct registers_t * reg, volatile struct tcb_t * tcb)
{
    _set_usr_sp_lr(tcb->context.sp, tcb->context.lr);
    reg->lr = tcb->context.pc;
    for (uint32_t i=0; i<NUM_REGISTERS; i++)
        reg->base_registers[i] = tcb->context.base_registers[i];
    L1_table_update(L2_Tables[tcb->L2_table_i]);
}

void reset_scheduler_timer()
{
    /* scheduler timing rule:
    - if thread is ready or no thread is sleeping: use usual timer interval
    - if no thread is ready: start when next thread will wake up
    */
    if ((runqueue != NO_THREAD) || (sleepqueue == NO_TCB)) {
        setup_timer(SCHEDULER_TIMER, TIMER_INTERVAL, &scheduler);
    }
    else {
        uint32_t soonest = sleepqueue->wake_at;
        volatile struct tcb_t *tcb = sleepqueue->next_sleeping; // traverse tcb

        while (tcb != NO_TCB) {
            if (tcb->wake_at < soonest)
                soonest = tcb->wake_at;
            tcb = tcb->next_sleeping;
        }

        uint32_t sleep_time = soonest - get_current_time();
        setup_timer(SCHEDULER_TIMER, sleep_time, &scheduler);
    }
        
}

void scheduler(void * arg)
{
    struct registers_t * reg = (struct registers_t *) arg;
    wake_threads();
    uint8_t only_one_ready_thread_exists = 0;
    if (runqueue != NO_THREAD)
        only_one_ready_thread_exists = (runqueue->next == runqueue);
    struct tcb_t *current_thread;
    current_thread = get_current_thread();

    if (runqueue == NO_THREAD) {
        /* "Idle Thread" */

        reg->lr = (uint32_t) &_infinite_loop;
    }
    else if(!only_one_ready_thread_exists || (current_thread->state == READY)) {
        /* store context */
        if (current_thread->state == RUNNING) {
            store_context(reg);
        }

        /* go to next task */
        runqueue = runqueue->next;

        /* load new context */
        current_thread = get_current_thread();
        load_context(reg, current_thread);
        current_thread->state = RUNNING;
    }

    reset_scheduler_timer();
}

uint8_t thread_wait_for_char(struct registers_t * reg)
{
    if (char_thread == NO_TCB) {
        store_context(reg);
        char_thread = get_current_thread();
        remove_tcb_from_runqueue(char_thread);
        char_thread->state = WAITING;
        scheduler(reg);
        
        return 0;
    }
    else {
        return 1;
    }
}

void thread_process_char_received(struct registers_t * reg)
{
    if (char_thread != NO_TCB) {
        store_context(reg);        

        // write character to desired memory location
        char *ret_addr_virt = (char*) char_thread->context.base_registers[0];
        char *ret_addr_phy = (char*) virt2phys_adr((uint32_t) ret_addr_virt, char_thread);
        *ret_addr_phy = uart_get_char();
        // L2 table of current thread does not need to be loaded because char_thread is
        // instantly scheduled (see below)

        // return 0 which means successful read
        char_thread->context.base_registers[0] = 0;

        // reschedule thread
        char_thread->state = RUNNING;
        add_tcb_to_runqueue(char_thread);
        load_context(reg, char_thread);
        reset_scheduler_timer();

        char_thread = NO_TCB;
    }
}

void thread_make_sleep_current(struct registers_t * reg, uint32_t millis)
{
    if (millis == 0) {
        return;
    }
    else {
        store_context(reg);

        struct tcb_t *current_thread = get_current_thread();
        current_thread->state = WAITING;
        current_thread->wake_at = get_current_time() + millis*1000;  // timer works on microseconds
        remove_tcb_from_runqueue(current_thread);
        // add to sleepqueue
        current_thread->next_sleeping = sleepqueue;
        sleepqueue = current_thread;

        scheduler(reg);
    }
}

void wake_threads()
{
    volatile struct tcb_t *tcb = sleepqueue; // traverse tcb
    volatile struct tcb_t *prior_tcb = NO_TCB; // tcb that is prior to tcb in the sleepqueue
    time_t current_time = get_current_time();

    while (tcb != NO_TCB) {
        if (tcb->wake_at <= current_time) {
            // remove from sleepqueue
            if (prior_tcb == NO_TCB) // true if this is first tcb in queue
                sleepqueue = tcb->next_sleeping;
            else
                prior_tcb->next_sleeping = tcb->next_sleeping;

            tcb->state = READY;
            tcb->wake_at = 0;
            add_tcb_to_runqueue(tcb);

            tcb = tcb->next_sleeping;
        }
        else {
            prior_tcb = tcb;
            tcb = tcb->next_sleeping;
        }

        if (tcb == sleepqueue) {
            /* Something went wrong, so the sleepqueue contains a loop.
            This appeared multiple times during testing. In this block,
            the queue is repaired and prepared to be traversed again */
            sleepqueue = NO_TCB;
            tcb = NO_TCB;

            for (uint32_t i=0; i<MAX_THREADS; i++) {
                volatile struct tcb_t *repair_tcb = &(tcbs[i]);
                if ((repair_tcb->state == WAITING) && (repair_tcb->wake_at > 0)) {
                    if (sleepqueue == NO_TCB) {
                        sleepqueue = repair_tcb;
                        tcb = repair_tcb;
                    }
                    else {
                        tcb->next_sleeping = repair_tcb;
                        repair_tcb->next_sleeping = NO_TCB;
                    }
                }
            }
            tcb = sleepqueue;
        }
    }
}