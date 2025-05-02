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
void malloc_test2();
void malloc_test3();
void malloc_test4();
void malloc_test5();

int main()
{
   // puts( "\n------------------------ malloc0 --------------------\n" );
   // malloc_test();

   // puts( "\n----------------------- malloc1 --------------------\n" );
   // malloc_test1();

   // puts( "\n----------------------- malloc2 --------------------\n" );
   // malloc_test2();

   // puts( "\n----------------------- malloc3 --------------------\n" );
   // malloc_test3();

   // puts( "\n----------------------- malloc4 --------------------\n" );
   // malloc_test4();

   puts( "\n----------------------- malloc5 --------------------\n" );
   malloc_test5();

   return EXIT_SUCCESS;
}

void malloc_test()
{
   mem_init();
   mm_init();
   mm_check_heap( 1 );

   char* cp  = mm_malloc( 16 );
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

   char* cp8  = mm_malloc( 32 );
   char* cp9  = mm_malloc( 32 );
   char* cp10 = mm_malloc( 32 );
   char* cp11 = mm_malloc( 32 );
   mm_check_heap( 1 );

   printf( "%p | %p | %p | %p\n", cp8, cp9, cp10, cp11 );

   puts( "48's" );
   char* cp12 = mm_malloc( 48 );
   char* cp13 = mm_malloc( 48 );
   char* cp14 = mm_malloc( 48 );
   char* cp15 = mm_malloc( 48 );
   
   mm_check_heap( 1 );

   printf( "%p | %p | %p | %p\n", cp12, cp13, cp14, cp15 );

   puts( "free 0 - 10" );
   mm_free( cp );
   mm_free( cp1 );
   mm_free( cp2 );
   mm_free( cp3 );
   mm_free( cp4 );
   mm_free( cp5 );
   mm_free( cp6 );
   mm_free( cp7 );
   mm_free( cp8 );
   mm_free( cp9 );
   mm_free( cp10 );
   mm_check_heap( 1 );

   puts( "free 11 - 15" );
   mm_free( cp11 );
   mm_free( cp12 );
   mm_free( cp13 );
   mm_free( cp14 );
   mm_free( cp15 );
   mm_check_heap( 1 );

   mem_deinit();
}


void malloc_test1()
{
   mem_init();
   mm_init();
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

   mm_free( cp );
   mm_free( cp1 );
   mm_free( cp2 );
   mm_free( cp3 );
   mm_free( cp4 );
   mm_free( cp5 );
   mm_free( cp6 );
   mm_free( cp7 );
   mm_check_heap( 1 );

   mem_deinit();
}


void malloc_test2()
{
   mem_init();
   mm_init();
   char* cpa[1000];

   for ( int count = 0; count < 1000; ++count )
   {
      cpa[count] = mm_malloc( 16 );
   }
   mm_check_heap( 1 );

   puts( "after free" );

   for ( int count = 0; count < 1000; ++count )
   {
      mm_free( cpa[count] );
   }
   mm_check_heap( 1 );

   mem_deinit();
}


void malloc_test3()
{
   mem_init();
   mm_init();

   char* cp = mm_malloc( 16 );
   char* cp1 = mm_malloc( 16 );
   char* cp2 = mm_malloc( 16 );
   char* cp3 = mm_malloc( 16 );
   char* cp4 = mm_malloc( 16 );
   char* cp5 = mm_malloc( 16 );
   char* cp6 = mm_malloc( 16 );
   char* cp7 = mm_malloc( 16 );

   printf( "%p | %p | %p | %p\n", cp, cp1, cp2, cp3 );
   printf( "%p | %p | %p | %p\n", cp4, cp5, cp6, cp7 );

   mm_check_heap( 1 );

   mm_free( cp3 );

   mm_check_heap( 1 );

   cp3 = mm_malloc( 16 );
   mm_check_heap( 1 );

   mm_free( cp7 );
   mm_free( cp6 );
   mm_free( cp5 );
   mm_free( cp4 );
   mm_free( cp3 );
   mm_free( cp2 );
   mm_free( cp1 );
   mm_free( cp );
   mm_check_heap( 1 );

   cp = mm_malloc( 16 );
   mm_check_heap( 1 );
   mm_free( cp );

   cp = mm_malloc( 512 );
   mm_check_heap( 1 );
   mm_free( cp );
   mm_check_heap( 1 );
   mem_deinit();
}


void malloc_test4()
{
   mem_init();
   mm_init();

   mm_check_heap( 1 );
   puts( "after init" );

   char* cp = mm_malloc( 32 );
   mm_check_heap( 1 );
   puts( "" );

   mm_free( cp );
   mm_check_heap( 1 );
   puts( "" );

   char* cp1 = mm_malloc( 32 );
   char* cp2 = mm_malloc( 32 );

   mm_check_heap( 1 );
   puts( "" );

   
   mm_free( cp1 );
   mm_free( cp2 );
   mm_check_heap( 1 );

   mem_deinit();
}


void malloc_test5()
{
   mem_init();
   mm_init();

   char* ptrs[578];
   char* ptrs2[578];

   for ( int count = 0; count < 578; ++count )
   {
      ptrs[count] = mm_malloc( 16 );
   }

   for ( int count = 0; count < 578; ++count )
   {
      ptrs2[count] = mm_malloc( 32 );
   }

   mm_check_heap( 1 );

   puts( "after free" );

   for ( int count = 0; count < 578; ++count )
   {
      mm_free( ptrs[count] );
   }

   for ( int count = 0; count < 578; ++count )
   {
      mm_free( ptrs2[count] );
   }
   mm_check_heap( 1 );

   mem_deinit();
}
