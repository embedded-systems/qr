#include <x32.h>

int i = 0;

void isr_hoppa() {
    i += 1;
    printf("hophop\n");
}

void hop_loop() {
    int j = 0;
    while(1) {
        j += 1; // break here, remove break, 'n' // 's' should not leave break here
        j += 1;
    }
}

int main() {
    unsigned int k = 0;
    char c = 1;
    unsigned char d = 'c';

    i = 0;

    peripherals[PERIPHERAL_TIMER2_PERIOD] = 1000 * CLOCKS_PER_MS;
    INTERRUPT_VECTOR(INTERRUPT_TIMER2) = &isr_hoppa;
    INTERRUPT_PRIORITY(INTERRUPT_TIMER2) = 1;

    ENABLE_INTERRUPT(INTERRUPT_TIMER2);
    ENABLE_INTERRUPT(INTERRUPT_GLOBAL);

    hop_loop(); 

    while(i < 5) {
        printf("%d ", i);
        i += 1;
    }

    //while(1);

 /*   while(i > -1) {
        i -= 1;
    }

    while(k < 5) {
        k += 1;
    }

    if(c < 4) {
        c = 4;
    }

    if(d == 'c') {
        d = 5;
    }

    if((short)i < (short)k) {
        d = 6;
    } */

    return 0;

}

