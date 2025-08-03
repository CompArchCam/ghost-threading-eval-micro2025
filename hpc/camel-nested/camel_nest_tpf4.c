
#include <stdio.h>
#include <time.h>
#include <stdatomic.h>

#include "../thpool/thpool.h"

#define MAX_KEY (33554432 >> 2)
#define MAX_ITER (33554432 << 2)
#define STEP 128


#ifndef PREFETCH
#define PREFETCH 1
#endif

#ifndef NUMHASH
#define NUMHASH 10
#endif

atomic_size_t main_iter; 

/* Code from http://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key#12996028 */
unsigned int
hash(unsigned int x) {
  x = x * 0x45d9f3b;
  /* Yes, it's going to mess up the distribution by taking an & */
  return x & (MAX_KEY - 1);
}

    int array1 [MAX_KEY];
    int* array2 [MAX_KEY];
    int* array3 [MAX_KEY/STEP][STEP];
    int histogram_array[MAX_KEY]; 


#define MEMBW
void PrefetchThread() {
    volatile int res; 
    char serialize_flag = 0; 
    for(int x=0; x<MAX_ITER/STEP; x++) {
        int index = x % (MAX_KEY/STEP); 
        for (int y = 0; y < STEP; y++) {
                __builtin_prefetch(array3[index][y]);

            /*dummy operations to slow pf thread down*/
            // res = atomic_load_explicit(&main_iter, memory_order_seq_cst); 
            #ifdef MEMBW
            if (serialize_flag) 
                __asm__ volatile ("serialize\n\t"); 

            if ((x*STEP+y) % 30 == 0 || serialize_flag == 1) {
                int main_iter_ = (int) atomic_load_explicit(&main_iter, memory_order_relaxed); 
                #ifdef PRINT_HISTOGRAM
                histogram_array[count] = i - main_iter_; 
                count++; 
                #endif 

                if ((x*STEP+y) - main_iter_ >= 75) {
                    serialize_flag = 1; 
                } else if ((x*STEP+y) - main_iter_ <= 63) {
                    serialize_flag = 0; 
                } else if ((x*STEP+y) - main_iter_ <= 0) {
                    serialize_flag = 0; 
                    y = main_iter_ + 10; 
                }
            }
            #else
            // res = atomic_load_explicit(&main_iter, memory_order_seq_cst); 
            #endif 
        }
    }
}

int main()
{
    threadpool thpool = thpool_init(1); 
    main_iter = 0; 
    
    for(int x=0; x<MAX_KEY; x++) {
    	array1[x] = x;
    	array2[x] = &(array1[hash(x)]);
    }
    for(int x=0; x<MAX_KEY/STEP; x++) {
        for (int y = 0; y < STEP; y++)
    	    array3[x][y] = array2[x*STEP+y]; 
    }
    
    long int sum = 0;
    
    struct timespec start, finish; 
    
    clock_gettime(CLOCK_REALTIME, &start); 
    thpool_add_work(thpool, (void*)PrefetchThread, NULL); // issue pf thread 
    for(int x=0; x<MAX_ITER/STEP; x++) {
        int index = x % (MAX_KEY/STEP); 
        for (int y = 0; y < STEP; y++) {
    	sum += 	
#if NUMHASH>50   	    										
hash( 
#endif
#if NUMHASH>49   	    										
hash( 
#endif
#if NUMHASH>48   	    										
hash( 
#endif
#if NUMHASH>47   	    										
hash( 
#endif
#if NUMHASH>46   	    										
hash( 
#endif
#if NUMHASH>45   	    										
hash( 
#endif
#if NUMHASH>44   	    										
hash( 
#endif
#if NUMHASH>43   	    										
hash( 
#endif
#if NUMHASH>42   	    										
hash( 
#endif
#if NUMHASH>41   	    										
hash( 
#endif
#if NUMHASH>40   	    										
hash( 
#endif
#if NUMHASH>39   	    										
hash( 
#endif
#if NUMHASH>38   	    										
hash( 
#endif
#if NUMHASH>37   	    										
hash( 
#endif
#if NUMHASH>36   	    										
hash( 
#endif
#if NUMHASH>35   	    										
hash( 
#endif
#if NUMHASH>34   	    										
hash( 
#endif
#if NUMHASH>33   	    										
hash( 
#endif
#if NUMHASH>32   	    										
hash( 
#endif
#if NUMHASH>31   	    										
hash( 
#endif
#if NUMHASH>30   	    										
hash( 
#endif
#if NUMHASH>29   	    										
hash( 
#endif
#if NUMHASH>28   	    										
hash( 
#endif
#if NUMHASH>27   	    										
hash( 
#endif
#if NUMHASH>26   	    										
hash( 
#endif
#if NUMHASH>25   	    										
hash( 
#endif
#if NUMHASH>24   	    										
hash( 
#endif
#if NUMHASH>23   	    										
hash( 
#endif
#if NUMHASH>22   	    										
hash( 
#endif
#if NUMHASH>21   	    										
hash( 
#endif
#if NUMHASH>20   	    										
hash( 
#endif
#if NUMHASH>19    	    										
hash( 		
#endif
#if NUMHASH>18    	    										
hash( 
#endif
#if NUMHASH>17    	    										
hash( 
#endif
#if NUMHASH>16    	    										
hash( 
#endif
#if NUMHASH>15    	    										
hash( 
#endif
#if NUMHASH>14    	    										
hash( 
#endif
#if NUMHASH>13    	    										
hash( 
#endif
#if NUMHASH>12    	    										
hash( 
#endif
#if NUMHASH>11    	    										
hash( 
#endif
#if NUMHASH>10    	    										
hash(     		
#endif
#if NUMHASH>9    	    										
hash( 
#endif
#if NUMHASH>8    	    										
hash( 
#endif
#if NUMHASH>7    	    										
hash( 
#endif
#if NUMHASH>6   	    										
hash( 
#endif
#if NUMHASH>5    	    										
hash( 
#endif
#if NUMHASH>4    	    										
hash( 
#endif
#if NUMHASH>3    	    										
hash( 
#endif
#if NUMHASH>2    	    										
hash( 
#endif
#if NUMHASH>1    	    										
hash( 
#endif
#if NUMHASH>0   	    										
hash( 
#endif
    	*array3[index][y]
#if NUMHASH>0   	    										
) 
#endif
#if NUMHASH>1   	    										
) 
#endif
#if NUMHASH>2   	    										
) 
#endif
#if NUMHASH>3   	    										
) 
#endif
#if NUMHASH>4  	    											
) 
#endif
#if NUMHASH>5	 	    										
) 
#endif
#if NUMHASH>6	  	    										
) 
#endif
#if NUMHASH>7	 	    										
) 
#endif
#if NUMHASH>8  	    										
) 
#endif
#if NUMHASH>9   	    										
) 
#endif
#if NUMHASH>10   	    										
) 
#endif
#if NUMHASH>11   	    										
) 
#endif
#if NUMHASH>12   	    										
) 
#endif
#if NUMHASH>13  	    										
) 
#endif
#if NUMHASH>14  	    										
) 
#endif
#if NUMHASH>15   	    										
) 
#endif
#if NUMHASH>16   	    										
) 
#endif
#if NUMHASH>17   	    										
) 
#endif
#if NUMHASH>18   	    										
) 
#endif
#if NUMHASH>19   	    										
) 
#endif
#if NUMHASH>20   	    										
) 
#endif
#if NUMHASH>21  	    										
) 
#endif
#if NUMHASH>22   	    										
) 
#endif
#if NUMHASH>23   	    										
) 
#endif
#if NUMHASH>24   	    										
) 
#endif
#if NUMHASH>25   	    										
) 
#endif
#if NUMHASH>26 	    										
) 
#endif
#if NUMHASH>27  	    										
) 
#endif
#if NUMHASH>28   	    										
) 
#endif
#if NUMHASH>29   	    										
) 
#endif
#if NUMHASH>30   	    										
) 
#endif
#if NUMHASH>31   	    										
) 
#endif
#if NUMHASH>32   	    										
) 
#endif
#if NUMHASH>33   	    										
) 
#endif
#if NUMHASH>34   	    										
) 
#endif
#if NUMHASH>35   	    										
) 
#endif
#if NUMHASH>36   	    										
) 
#endif
#if NUMHASH>37   	    										
) 
#endif
#if NUMHASH>38   	    										
) 
#endif
#if NUMHASH>39   	    										
) 
#endif
#if NUMHASH>40   	    										
) 
#endif
#if NUMHASH>41   	    										
) 
#endif
#if NUMHASH>42   	    										
) 
#endif
#if NUMHASH>43   	    										
) 
#endif
#if NUMHASH>44   	    										
) 
#endif
#if NUMHASH>45   	    										
) 
#endif
#if NUMHASH>46   	    										
) 
#endif
#if NUMHASH>47   	    										
) 
#endif
#if NUMHASH>48   	    										
) 
#endif
#if NUMHASH>49   	    										
) 
#endif
#if NUMHASH>50   	    										
) 
#endif
;
    #ifdef MEMBW
    // if (x % 20 == 0)
    #endif 
        atomic_store_explicit(&main_iter, (x*STEP+y), memory_order_relaxed); 
        }
    }
    thpool_wait(thpool); 
    clock_gettime(CLOCK_REALTIME, &finish);

    /*print histogram*/
    // int histogram[32] = {0}; 
    // for (int i = 0; i < MAX_KEY; i++) {
    //     if (histogram_array[i] <= 0)
    //         histogram[0]++; 
    //     else if (histogram_array[i] >= 31)
    //         histogram[31]++; 
    //     else 
    //         histogram[histogram_array[i]]++; 
    // }
    // for (int i = 0; i < 32; i++)
    //     printf("%d\n", histogram[i]); 

    /*print execute time*/
    my_timespec_ms(start, finish); 
    
    printf("Result %ld\n", sum);
    

    return 0;
}
