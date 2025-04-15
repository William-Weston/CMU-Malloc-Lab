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

int main()
{
   mem_init();

   return EXIT_SUCCESS;
}