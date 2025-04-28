/**
 * @file    seg_list.c
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Source file for mm.h that uses segregated free list implementation
 * @version 0.1
 * @date    2025-04-22
 * 
 * @copyright Copyright (c) 2025
 * 
 * Segregated Free List Implementation
 */
#include "mm.h"
#include "memlib.h"                   // mem_sbrk

#include <stdint.h>                   // uint32_t, uintptr_t
#include <stdio.h>                    // printf
#include <string.h>                   // memset


// =====================================
// Typedefs
// =====================================

typedef unsigned char byte;


// =====================================
// Constants
// =====================================

#define WSIZE                        4           // Word size (bytes)
#define DSIZE                        8           // Double word size (bytes)
#define CHUNKSIZE                    ( 1<<12 )   // Extend heap by this amount (bytes)
#define ALIGNMENT                    16          // Align on 16 byte boundaries


// =====================================
// Macros
// =====================================

#define MAX( x, y )                  ( ( x ) > ( y ) ? ( x ) : ( y ) )  


// =====================================
// Private Global Variables
// =====================================


// =====================================
// Private Function Prototypes
// =====================================



// =====================================
// Public Function Definitions
// =====================================


/**
 * @brief  Initialize the memory manager
 * @return On success, returns 0
 *         On error, returns -1
 */
int mm_init( void )
{
   return -1;
}


/**
 * @brief Allocates size bytes of uninitialized storage
 * 
 * @param size    The number of bytes requested for allocation
 * @return void*  On success, returns a pointer to begining of allocated memory
 *                On error, returns a null pointer
 */
void* mm_malloc( size_t size )
{
   return NULL;
}


/**
 * @brief      Free a block of allocated memory
 * 
 * @param ptr  Pointer to allocated block of memory to be freed
 */
void mm_free( void* ptr )
{

}


/**
 * @brief  Reallocates the given area of memory.
 * 
 * @param ptr     if ptr is not NULL, it must be previously allocated by mm_malloc, mm_calloc 
 *                or mm_realloc and not yet freed
 * @param size    number of bytes to allocate
 * @return void*  On success, returns the pointer to the beginning of newly allocated memory.
 *                On error, returns a null pointer
 * 
 * The mm realloc routine returns a pointer to an allocated region of at least size bytes with 
 * the following constraints:
 * 
 *   – if ptr is NULL, the call is equivalent to mm_malloc(size);
 *   – if size is equal to zero, the call is equivalent to mm_free(ptr);
 *   – if ptr is not NULL, it must have been returned by an earlier call to mm_malloc, mm_calloc or mm_realloc
 *     the call to mm_realloc changes the size of the memory block pointed to by ptr (the old block) 
 *     to size bytes and returns the address of the new block.
 * 
 * Reallocation is done by either:
 * 
 *   a) expanding or contracting the existing area pointed to by ptr, if possible. 
 *      the contents of the area remain unchanged up to the lesser of the new and old sizes. 
 *      if the area is expanded, the contents of the new part of the array are undefined.
 *   b) allocating a new memory block of size new_size bytes, copying memory area with size equal 
 *      the lesser of the new and the old sizes, and freeing the old block.
 * 
 * If there is not enough memory, the old memory block is not freed and null pointer is returned.
 *         
 */
void* mm_realloc( void* ptr, size_t size )
{
   return NULL;
}


/**
 * @brief Allocates memory for an array of num objects of size and initializes all bytes in the allocated storage to zero. 
 * 
 * @param num     number of objects
 * @param size    size of each object
 * @return void*  On success, returns the pointer to the beginning of newly allocated memory.
 *                On failure, returns a null pointer. 
 */
void* mm_calloc( size_t num, size_t size )
{
   return NULL;
}


/**
 * @brief Check the heap for consistency
 * 
 * @param verbose  If 0, don't print verbose output
 *                 
 */
void mm_check_heap( int verbose )
{

}