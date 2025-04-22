/**
 * @file    mm.h
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Implementation of general purpose allocator
 * @version 0.1
 * @date    2025-04-15
 * 
 * @copyright Copyright (c) 2025
 * 
 * @note    Adapted from CSAPP
 */
#ifndef __2025_04_15_MM_H__
#define __2025_04_15_MM_H__

#include <stddef.h>           // size_t

int   mm_init( void );

void* mm_malloc( size_t size );
void* mm_realloc( void* ptr, size_t size );
void* mm_calloc( size_t num, size_t size );
void  mm_free( void* ptr );

void  mm_check_heap( int verbose );

#endif   // __2025_04_15_MM_H__