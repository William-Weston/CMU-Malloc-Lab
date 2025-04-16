/**
 * @file    memlib.c
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Source file for memlib.h
 * @version 0.1
 * @date    2025-04-15
 * 
 * @copyright Copyright (c) 2025
 * 
 *
 * A module that simulates the memory system.
 * Needed because it allows us to interleave calls from the student's
 * malloc package with the system's malloc package in libc.
 * 
 * The mem_init function models the virtual memory available to the heap as a
 * large double-word aligned array of bytes. 
 * 
 * The bytes between mem_heap and mem_brk represent allocated virtual memory. 
 * The bytes following mem_brk represent unallocated virtual memory. 
 * 
 * The allocator requests additional heap memory by calling the mem_sbrk function, 
 * which has the same interface as the system’s sbrk function, as well as the same semantics, 
 * except that it rejects requests to shrink the heap.
 *
 */
#include "memlib.h"
#include "std_wrappers.h"

#include <errno.h>          // ENOMEM, errno
#include <stdio.h>          // fprintf, stderr
#include <stdlib.h>         // free

#include <unistd.h>         // getpagesize


// =======================
// Constants and Macros  
// =======================

#define MAX_HEAP ( 20 * ( 1 << 20 ) )      /* 20 MB */


// ==========================
// Private Global Variables
// ==========================

static char* mem_heap;     /* Points to first byte of heap          */
static char* mem_brk;      /* Points to last byte of heap plus 1    */
static char* mem_max_addr; /* Max legal heap addr plus 1            */


/**
 * mem_init - Initialize the memory system model
 *
 */
void mem_init( void )
{
   mem_heap     = ( char* )Malloc( MAX_HEAP );
   mem_brk      = ( char* )mem_heap;
   mem_max_addr = ( char* )( mem_heap + MAX_HEAP );
}


/*
 * mem_sbrk - Simple model of the sbrk function.
 *            Extends the heap by incr bytes and returns the start address of the new area.
 *            In this model, the heap cannot be shrunk.
 *
 * Return: On success, returns the previous program break.
 *         On error, (void *) -1 is returned, and errno is set to ENOMEM
 */
void* mem_sbrk( int incr )
{
   char* old_brk = mem_brk;

   if ( ( incr < 0 ) || ( mem_brk > ( mem_max_addr - incr ) ) )
   {
      errno = ENOMEM;
      fprintf( stderr, "ERROR: mem_sbrk failed - Ran out of memory...\n" );
      return ( void* )-1;
   }

   mem_brk += incr;
   return ( void* )old_brk;
}


/*
 * mem_deinit - free the storage used by the memory system model
 */
void mem_deinit( void )
{
   free( mem_heap );
}


/*
 * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 */
void mem_reset_brk()
{
   mem_brk = mem_heap;
}


/*
 * mem_heap_lo - return address of the first heap byte
 */
void* mem_heap_lo()
{
   return ( void* )mem_heap;
}


/*
 * mem_heap_hi - return address of last heap byte
 */
void* mem_heap_hi()
{
   return ( void* )( mem_brk - 1 );
}


/*
 * mem_heapsize() - returns the heap size in bytes
 */
size_t mem_heapsize()
{
   return mem_brk - mem_heap;
}


/* 
 * mem_pagesize() - returns the page size of the system
 */
size_t mem_pagesize()
{
   return getpagesize();
}