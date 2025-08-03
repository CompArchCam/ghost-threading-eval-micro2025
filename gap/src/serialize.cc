#include <iostream>
#include <cstdlib>
#include <atomic>

#include <sched.h>
#include <pthread.h>

#define MEM_SIZE 5*1024*1024

int main() {
    volatile float dummy = 1234234232.484848; 
    while (true) {
        // for (int i = 0; i < 32; i++) {
            // dummy /= 1.1; 
            // dummy /= 0.9; 
            // dummy /= 1.2; 
            // dummy /= 0.8; 
            // dummy /= 1.3; 
            // dummy /= 0.7; 
            // dummy /= 1.4; 
            // dummy /= 0.6; 
        // } 

        asm volatile ("serialize\n\t"); 
    }
    
    return 0; 
}