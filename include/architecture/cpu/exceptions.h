#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#define RW_OFFSET   11
#define STATUS_4BIT 10
#define IMP_EX_ABT  0b10110

#define DATA_ABT_LR_OFFSET  8
#define PREF_ABT_LR_OFFSET  4
#define UND_LR_OFFSET   4
#define SVC_LR_OFFSET   4
#define IRQ_LR_OFFSET   8
#define FIQ_LR_OFFSET   8

/* The CORRECTIONs define, where to jump 
after the exception is processed */
#define DATA_ABT_LR_CORRECTION  4
#define PREF_ABT_LR_CORRECTION  0
#define UND_LR_CORRECTION   0
#define SVC_LR_CORRECTION   0
#define IRQ_LR_CORRECTION   4
#define FIQ_LR_CORRECTION   4

#endif