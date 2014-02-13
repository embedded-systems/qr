    #include <x32.h>

    void run_out_of_memory() {
        run_out_of_memory();
    }

    void out_of_memory_isr() {
        printf("Out of memory, exitting!\r\n");
        DISABLE_INTERRUPT(INTERRUPT_OUT_OF_MEMORY);
        exit(1);
    }

    int main() {
        INTERRUPT_VECTOR(INTERRUPT_OUT_OF_MEMORY) = &out_of_memory_isr;
        INTERRUPT_PRIORITY(INTERRUPT_OUT_OF_MEMORY) = 1000;
        ENABLE_INTERRUPT(INTERRUPT_OUT_OF_MEMORY);
        ENABLE_INTERRUPT(INTERRUPT_GLOBAL);

        printf("Running out of memory...\r\n");
        run_out_of_memory();
        return 0;
    }
