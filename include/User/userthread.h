#ifndef USERTHREAD_H
#define USERTHREAD_H

void main(void *x) __attribute__((weak));
void _infinite_loop(void) __attribute__((weak));

#endif // USERTHREAD_H