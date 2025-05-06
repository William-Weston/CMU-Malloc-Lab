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

#include <assert.h>
#include <stdlib.h>    // EXIT_SUCCESS
#include <stdint.h>
#include <stdio.h>     // puts

void malloc_test();
void malloc_test1();
void malloc_test2();
void malloc_test3();
void malloc_test4();
void malloc_test5();
void malloc_test6();   // big malloc
void malloc_align_test();

void realloc_test0();
void realloc_test1();

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

   // puts( "\n----------------------- malloc5 --------------------\n" );
   // malloc_test5();

   // puts( "\n----------------------- malloc6 --------------------\n" );
   // malloc_test6();

   // puts( "\n-------------------- malloc_align ------------------\n" );
   // malloc_align_test();

   // puts( "\n--------------------- realloc0 ---------------------\n" );
   // realloc_test0();

   puts( "\n--------------------- realloc1 ---------------------\n" );
   realloc_test1();

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

   char* cp  = mm_malloc( 512 );
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
   char* ptrs3[578];

   for ( int count = 0; count < 578; ++count )
   {
      ptrs[count] = mm_malloc( 16 );
   }

   for ( int count = 0; count < 578; ++count )
   {
      ptrs2[count] = mm_malloc( 32 );
   }

   for ( int count = 1; count < 578; ++count )
   {
      ptrs3[count] = mm_malloc( count );
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

   for ( int count = 1; count < 578; ++count )
   {
      mm_free( ptrs3[count] );
   }


   mm_check_heap( 1 );

   mem_deinit();
}


void malloc_test6()
{
   mem_init();
   mm_init();

   char* cp = mm_malloc( 3000 );

   printf( "address: %p\n", cp );

   mm_check_heap( 1 );

   char* cp2 = mm_malloc( 3000 );

   printf( "address2: %p\n", cp2 );

   mm_check_heap( 1 );

   char* cp3 = mm_malloc( 6000 );

   printf( "address3: %p\n", cp3 );

   mm_check_heap( 1 );

   char* cp4 = mm_malloc( 1100 );

   printf( "address4: %p\n", cp4 );

   mm_check_heap( 1 );

   puts( "\nFree" );
   mm_free( cp  );
   mm_free( cp2 );
   mm_free( cp3 );
   mm_free( cp4 );

   mm_check_heap( 1 );

   mem_deinit();
}


void malloc_align_test()
{
   mem_init();
   mm_init();

   char* cp[100];

   for ( int idx = 0, size = 16; idx < 100; ++idx, size += 16 )
   {
      cp[idx] = mm_malloc( size );
   }

   mm_check_heap( 1 );

   for ( int idx = 0; idx < 100; ++idx )
   {
      assert( ( uintptr_t )(cp[idx] ) % 16 == 0 );
   }

   for ( int idx = 0; idx < 100; ++idx )
   {
      mm_free( cp[idx] );
   }

   mm_check_heap( 1 );

   char* cp2 = mm_malloc( 129 );
   assert( ( uintptr_t )cp2 % 16 == 0 );

   char* cp3 = mm_malloc( 129 );
   assert( ( uintptr_t )cp3 % 16 == 0 );

   mm_check_heap( 1 );

   mm_free( cp2 );
   mm_free( cp3 );

   mem_deinit();
}


void realloc_test0()
{
   mem_init();
   mm_init();

   char* cp0 = mm_realloc( NULL, 129 );
   assert( ( uintptr_t )cp0 % 16 == 0 );

   char* cp1 = mm_realloc( cp0, 8 );
   assert( ( uintptr_t )cp1 % 16 == 0 );
   assert( cp1 == cp0 );

   char* cp2 = mm_realloc( NULL, 1 );
   assert( ( uintptr_t )cp2 % 16 == 0 );

   char* cp3 = mm_realloc( cp2, 15 );
   assert( ( uintptr_t )cp3 % 16 == 0 );
   assert( cp3 == cp2 );

   char* cp4 = mm_realloc( NULL, 16 );
   assert( ( uintptr_t )cp4 % 16 == 0 );

   cp4 = mm_realloc( cp4, 32 );
   assert( ( uintptr_t )cp4 % 16 == 0 );

   cp4 = mm_realloc( cp4, 256 );
   assert( ( uintptr_t )cp4 % 16 == 0 );

   char* cp5 = mm_realloc( NULL, 252 );
   assert( ( uintptr_t )cp5 % 16 == 0 );

   mm_check_heap( 1 );

   char* cp6 = mm_realloc( cp5, 256 );
   assert( ( uintptr_t )cp6 % 16 == 0 );
   assert( cp6 == cp5 );

   char* cp7 = mm_realloc( NULL, 144 );
   assert( ( uintptr_t )cp7 % 16 == 0 );

   mm_free( cp6 );

   cp5 = mm_realloc( NULL, 272 );
   assert( ( uintptr_t )cp5 % 16 == 0 );
  
   mm_check_heap( 1 );
   mem_deinit();
}


void realloc_test1()
{
   mem_init();
   mm_init();

   char* cp0 = mm_realloc( NULL, 256 );
   char* cp1 = mm_realloc( NULL, 256 );
   char* cp2 = mm_realloc( NULL, 256 );
   char* cp3 = mm_realloc( NULL, 256 );
   char* cp4 = mm_realloc( NULL, 172 );
   char* cp5 = mm_realloc( NULL, 256 );
   char* cp6 = mm_realloc( NULL, 256 );

   assert( ( uintptr_t )cp0 % 16 == 0 );
   assert( ( uintptr_t )cp1 % 16 == 0 );
   assert( ( uintptr_t )cp2 % 16 == 0 );
   assert( ( uintptr_t )cp3 % 16 == 0 );
   assert( ( uintptr_t )cp4 % 16 == 0 );
   assert( ( uintptr_t )cp5 % 16 == 0 );
   assert( ( uintptr_t )cp6 % 16 == 0 );

   mm_free( cp4 );

   mm_check_heap( 1 );

   cp3 = mm_realloc( cp3, 270 );
   assert( ( uintptr_t )cp3 % 16 == 0 );

   mm_check_heap( 1 );

   cp3 = mm_realloc( cp3, 300 );
   assert( ( uintptr_t )cp3 % 16 == 0 );

   mm_check_heap( 1 );


   cp3 = mm_realloc( cp3, 304 );
   assert( ( uintptr_t )cp3 % 16 == 0 );

   mm_check_heap( 1 );

   mem_deinit();
}