#ifndef MY_RECLAIMER_DEF_H
#define MY_RECLAIMER_DEF_H

#include <cmath>
#include <iostream>
#include <assert.h>

//The size of the smallest buffer
#define BASE_SIZE 100
//The growth rate
#define FACTOR  1.5
//Maximum number of increases
#define MAX_ORDER 24

#define MAX_UNIF_SIZE BASE_SIZE*pow(FACTOR,MAX_ORDER-1)


#ifndef SOFTWARE_BARRIER
#   define SOFTWARE_BARRIER asm volatile("": : :"memory")
#endif


#define CAT2(x, y) x##y
#define CAT(x, y) CAT2(x, y)

#define PAD volatile char CAT(___padding, __COUNTER__)[128]

#define MAX_THREADS_POW2 512

#endif //MY_RECLAIMER_DEF_H
