/**
 * @file    mmtester.c
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Simple tester and main
 * @version 0.1
 * @date    2025-04-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "memlib.h"    // memory system
#include "mm.h"        // allocator

#include <stddef.h>     // EXIT_SUCCESS
#include <stdio.h>      // printf

void malloc_test();
void realloc_test();
void calloc_test();
void test();

int main()
{
   mem_init();
   mm_init();

   mm_check_heap( 1 );

   realloc_test();
  
   mem_deinit();

   return EXIT_SUCCESS;
}


void malloc_test()
{
   char* ptr = mm_malloc( 64 );

   mm_check_heap( 1 );

   mm_free( ptr );
   mm_check_heap( 1 );

   ptr = mm_malloc( 4096 );

   mm_check_heap( 1 );

   if ( ptr == NULL )
   {
      printf( "Error: null pointer\n" );
   }
   mm_free( ptr );

   mm_check_heap( 1 );
}


void realloc_test()
{
   // ptr is NULL
   char* ptr = mm_realloc( NULL, 64 );

   mm_check_heap( 1 );

   // size is zero
   ptr = mm_realloc( ptr, 0 );

   mm_check_heap( 1 );

   ptr = mm_realloc( ptr, 64 );
   
   mm_check_heap( 1 );

   // size is < old size
   ptr = mm_realloc( ptr, 32 );

   mm_check_heap( 1 );

   mm_free( ptr );

   mm_check_heap( 1 );

   ptr = mm_realloc( NULL, 64 );

   mm_check_heap( 1 );

   ptr = mm_realloc( ptr, 128 );

   mm_check_heap( 1 );

   mm_free( ptr );

   mm_check_heap( 1 );
}


void calloc_test()
{
   char* ptr = mm_calloc( 32, 1 );

   for ( int idx = 0; idx < 32; ++idx )
   {
      if ( *ptr != 0 )
      {
         printf( "Calloc did not zero initialize\n" );
         break;
      }
   }
   mm_check_heap( 1 );
   mm_free( ptr );
   mm_check_heap( 1 );
}


void test()
{

}
