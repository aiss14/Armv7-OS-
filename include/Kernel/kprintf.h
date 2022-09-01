#ifndef KPRINTF_H
#define KPRINTF_H

void kprintf(char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif // KPRINTF_H