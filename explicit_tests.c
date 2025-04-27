/**
 * @file    explicit_tests.c
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Pseudo-tests for explicit free list implementation
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

void init_test();
void malloc_test();
void malloc_test2();

int main()
{
   // puts( "\n------------------------  init  --------------------\n" );
   // init_test();

   puts( "\n------------------------ malloc --------------------\n" );
   malloc_test();

   puts( "\n------------------------ malloc2 --------------------\n" );
   malloc_test2();

   return EXIT_SUCCESS;
}

void init_test()
{
   mem_init();

   mm_init();

   mm_check_heap( 1 );
   mem_deinit();
}

void malloc_test()
{
   mem_init();
   mm_init();
   mm_check_heap( 1 );

   char* cp = mm_malloc( 64 );
   mm_check_heap( 1 );

   mm_free( cp );
   mm_check_heap( 1 );

   cp = mm_malloc( 256 );
   mm_check_heap( 1 );

   char* cp2 = mm_malloc( 128 );
   mm_check_heap( 1 );

   char* cp3 = mm_malloc( 512 );
   // mm_check_heap( 1 );
   char* cp4 = mm_malloc( 1024 );
   char* cp5 = mm_malloc( 32 );
   mm_free( cp2 );
   mm_free( cp4 );

   char* cp6 = mm_malloc( 4092 );
   mm_free( cp );
   mm_free( cp3 );
   mm_check_heap( 1 );

   mm_free( cp5 );
   mm_check_heap( 1 );

   mm_free( cp6 );
   mm_check_heap( 1 );
   mem_deinit();
}

void malloc_test2()
{
   mem_init();
   mm_init();
   mm_check_heap( 1 );  

   char* cp = mm_malloc( 4096 );

   // mm_check_heap( 1 );

   char* cp2 = mm_malloc( 4096 );
   // mm_check_heap( 1 );

   char* cp3 = mm_malloc( 4000 );
   // mm_check_heap( 1 );

   char* cp4 = mm_malloc( 512 );
   // mm_check_heap( 1 );

   char* cp5 = mm_malloc( 128 );
   // mm_check_heap( 1 );

   char* cp6 = mm_malloc( 576 );
   // mm_check_heap( 1 );
   
   char* cp7 = mm_malloc( 256 );
   // mm_check_heap( 1 );

   char* cp8 = mm_malloc( 8192 );
   // mm_check_heap( 1 );

   char* cp9 = mm_malloc( 2500 );
   char* cp10 = mm_malloc( 64 );
   char* cp11 = mm_malloc( 10000 );
   char* cp12 = mm_malloc( 7000 );

   mm_free( cp3 );
   mm_check_heap( 1 );

   mm_free( cp );
   mm_check_heap( 1 );

   mm_free( cp4 );
   mm_check_heap( 1 );

   mm_free( cp6 );
   mm_check_heap( 1 );

   mm_free( cp9 );
   mm_check_heap( 1 );

   mm_free( cp12 );
   mm_check_heap( 1 );

   mm_free( cp5 );
   mm_check_heap( 1 );

   mm_free( cp7 );
   mm_check_heap( 1 );

   mm_free( cp8 );
   mm_check_heap( 1 );

   mm_free( cp10 );
   mm_check_heap( 1 );
   
   mm_free( cp2 );
   mm_check_heap( 1 );

   mm_free( cp11 );
   mm_check_heap( 1 );
   

   mem_deinit();
}