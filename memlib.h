/**
 * @file    memlib.h
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Provides a model of the memory system for an allocator to use
 *          without interfering with the existing system-level malloc package.
 * @version 0.1
 * @date    2025-04-15
 * 
 * @copyright Copyright (c) 2025
 * 
 * Source:  Adapted from CSAPP
 * 
 * Any application wishing to use the allocator must first call mem_init() to initialize 
 * the memory system.
 */
#ifndef __2025_04_15_MEMLIB_H__
#define __2025_04_15_MEMLIB_H__

#include <stddef.h>            // size_t

void   mem_init( void );
void*  mem_sbrk( int incr );

void   mem_deinit( void );
void   mem_reset_brk( void );
void*  mem_heap_lo( void );
void*  mem_heap_hi( void );
size_t mem_heapsize( void );
size_t mem_pagesize( void );


#endif  // __2025_04_15_MEMLIB_H__