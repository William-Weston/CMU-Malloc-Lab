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
void init_test();

int main()
{
   puts( "\n---------------------- init -----------------------\n" );
   init_test();

   puts( "\n-------------------- mm_malloc --------------------\n" );
   malloc_test();

   puts( "\n------------------- mm_realloc --------------------\n" );
   realloc_test();

   puts( "\n-------------------- mm_calloc --------------------\n" );
   calloc_test();

   return EXIT_SUCCESS;
}


void malloc_test()
{
   mem_init();
   mm_init();

   mm_check_heap( 1 );

   char* ptr = mm_malloc( 64 );

   mm_check_heap( 1 );

   mm_free( ptr );
   mm_check_heap( 1 );

   ptr = mm_malloc( 64 );

   char* ptr2 = mm_malloc( 28 );

   mm_check_heap( 1 );

   mm_free( ptr );

   mm_check_heap( 1 );

   mm_free( ptr2 );

   mm_check_heap( 1 );

   ptr = mm_malloc( 4096 );

   mm_check_heap( 1 );

   if ( ptr == NULL )
   {
      printf( "Error: null pointer\n" );
   }
   mm_free( ptr );

   mm_check_heap( 1 );

   mem_deinit();
}


void realloc_test()
{
   mem_init();
   mm_init();

   mm_check_heap( 1 );

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

   mem_deinit();
}


void calloc_test()
{
   mem_init();
   mm_init();

   mm_check_heap( 1 );

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

   ptr = mm_calloc( 4086, 1 );
   mm_check_heap( 1 );
   
   int* ip = mm_calloc( 1000, sizeof( int ) );

   for ( int idx = 0; idx < 1000; ++idx )
   {
      if ( *ip != 0 )
      {
         printf( "Error: mm_calloc failed to zero initialize integers\n" );
         break;
      }
   }

   mm_check_heap( 1 );
   mm_free( ptr );
   mm_check_heap( 1 );

   mm_free( ip );
   mm_check_heap( 1 );

   mem_deinit();
}


void init_test()
{
   mem_init();
   mm_init();

   mm_check_heap( 1 );
   mem_deinit();
}
