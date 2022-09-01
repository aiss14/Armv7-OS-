#include <stdint.h>
#include <user/userthread.h>
#include <user/sys.h>
#include <kernel/kprintf.h>
#include <config.h>
#include <lib/math.h>

#define N_REPEAT_CHAR       10
#define BUSY_WAIT_SCALER    50

volatile uint32_t global = 100;
char c_print;

void demo_thread(void *x){
    uint8_t local=0;
    uint8_t id=*((uint8_t*)x);
    while(global<117) {
        local++;
        global++;

        write_char(c_print);
        write_char(':');
        write_char((char)(global/100)+'0');
        write_char((char)((global/10)%10)+'0');
        write_char((char)(global%10)+'0');
        write_char(' ');
        write_char('(');
        write_char((char) id+'0');
        write_char(':');
        if (local > 9)
            write_char((char) local/10+'0');
        write_char((char) local%10+'0');
        write_char(')');
        write_char('\n');
        sleep(500);
    }
}

void demo_process(void* x)
{
    c_print = *((char*)x);

    if (((c_print >= 'a') && (c_print <='z')) || ((c_print >= 'A') && (c_print <='Z'))) {
        uint8_t id1=1;
        uint8_t id2=2;
        thread_create(demo_thread, &id1, sizeof(id1), 0);
        sleep(166);
        thread_create(demo_thread, &id2, sizeof(id2), 0);
        sleep(166);
    }
    uint8_t id3=3;
    demo_thread(&id3);
}


void main(void *x) {
    (void) x;

    while (1)
    {
        char c;
        uint8_t ret_val = 1;
        while (ret_val) {
            ret_val = read_char(&c);
            thread_create(demo_process, &c, sizeof(c), 1);
        }

    }
}
