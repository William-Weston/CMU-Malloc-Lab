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


int main()
{
   mem_init();

   mm_init();

   mm_check_heap( 1 );
   mem_deinit();
   return EXIT_SUCCESS;
}