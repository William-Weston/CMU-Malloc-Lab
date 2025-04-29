/**
 * @file    seg_tests.c
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Pseudo-tests for segregated free list implementation
 * @version 0.1
 * @date    2025-04-22
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "memlib.h"
#include "mm.h"

#include <stdlib.h>    // EXIT_SUCCESS
#include <stdio.h>     // puts

void malloc_test();
void malloc_test1();

int main()
{
   malloc_test1();
   return EXIT_SUCCESS;
}

void malloc_test()
{
   mem_init();   
   mm_check_heap( 1 );
   char* cp = mm_malloc( 16 );
   char* cp1 = mm_malloc( 16 );
   char* cp2 = mm_malloc( 16 );
   char* cp3 = mm_malloc( 16 );
   char* cp4 = mm_malloc( 16 );
   char* cp5 = mm_malloc( 16 );
   char* cp6 = mm_malloc( 16 );
   char* cp7 = mm_malloc( 16 );
   
   mm_check_heap( 1 );
   printf( "%p | %p | %p | %p\n", cp, cp1, cp2, cp3 );
   printf( "%p | %p | %p | %p\n", cp4, cp5, cp6, cp7 );

   char* cp8 = mm_malloc( 32 );
   char* cp9 = mm_malloc( 32 );
   char* cp10 = mm_malloc( 32 );
   char* cp11 = mm_malloc( 32 );
   mm_check_heap( 1 );

   printf( "%p | %p | %p | %p\n", cp8, cp9, cp10, cp11 );
   mem_deinit();
}


void malloc_test1()
{
   mem_init();   
   mm_check_heap( 1 );
   char* cp = mm_malloc( 512 );
   char* cp1 = mm_malloc( 512 );
   char* cp2 = mm_malloc( 512 );
   char* cp3 = mm_malloc( 512 );
   char* cp4 = mm_malloc( 512 );
   char* cp5 = mm_malloc( 512 );
   char* cp6 = mm_malloc( 512 );
   char* cp7 = mm_malloc( 512 );
   
   mm_check_heap( 1 );
   printf( "%p | %p | %p | %p\n", cp, cp1, cp2, cp3 );
   printf( "%p | %p | %p | %p\n", cp4, cp5, cp6, cp7 );

   mem_deinit();
}