/**
 * @file    std_wrappers.c
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Source file for std_wrappers.h
 * @version 0.1
 * @date    2025-04-15
 * 
 * @copyright Copyright (c) 2025
 * 
 *    Adapted from CSAPP.
 */

#include "std_wrappers.h"

#include <errno.h>          // errno
#include <stdlib.h>         // exit, EXIT_FAILURE, malloc
#include <stdio.h>          // fprintf, stderr
#include <string.h>         // strerror

void* Malloc( size_t size )
{
   void* ptr;

   if ( ( ptr = malloc( size ) ) == NULL )
      unix_error( "Malloc Error" );
   
   return ptr;
}


// ==============================
// Error Handling Functions
// ==============================

// Unix-style error
void unix_error( char* msg )
{
   fprintf( stderr, "%s: %s\n", msg, strerror( errno ) );
   exit( EXIT_FAILURE );
}