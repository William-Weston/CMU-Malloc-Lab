/**
 * @file    mm.c
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Source file for mm.h
 * @version 0.1
 * @date    2025-04-15
 * 
 * @copyright Copyright (c) 2025
 * 
 * ------------------------------------------------------------------------------------------------
 * 
 * Implementation of allocator using an implicit free list
 * 
 *        prologue                                                                  epilogue  
 * start    / \     /  block 1 \     /  block 2 \             /    block n   \         |
 *   ^     /   \   /            \   /            \           /                \        |
 *   |    /     \ /              \ /              \         /                  \       |
 *   |   |       |                |                |       |                    \     / \
 *   |---------------------------------------------         ------------------------------
 *   |   |8/1|8/1|hdr|        |ftr|hdr|        |ftr|  ...  |hrd|        |        |ftr|0/1|
 *   |---------------------------------------------         ------------------------------
 *   |       |       |        |       |        |               |        |        |       |
 *   |       |       |        |       |        |               |        |        |       |
 *    \     /
 *     \   /
 *      \ /
 *     double                   
 *      word
 *     aligned
 *    (8 bytes)
 * 
 * 
 * Block Format:
 * 
 *       31       ...       3 2 1 0
 *      ----------------------------
 *      |    Block Size      | a/f |   Header    a = 001 : Allocated
 *      |--------------------------|             f = 000 : Free
 *      |                          |
 *      |         Payload          |
 *      |  (allocated block only)  |
 *      |                          |
 *      |                          |
 *      |--------------------------|
 *      |         Padding          |
 *      |        (Optional)        |
 *      |--------------------------|
 *      |    Block Size      | a/f |   Footer  
 *      ----------------------------
 * 
 */
#include "mm.h"
#include "memlib.h"      // mem_sbrk

#include <stdint.h>      // uint32_t, uintptr_t


// =====================================
// Constants
// =====================================

#define WSIZE      4          // Word and header/footer size (bytes)
#define DSIZE      8          // Double word size (bytes)
#define CHUNKSIZE  ( 1<<12 )  // Extend heap by this amount (bytes)



// =====================================
// Macros
//=====================================

#define GET( p )               ( *( uint32_t* )( p ) )         // read a word at address ptr
#define PUT( p, val )          ( *( uint32_t* )( p ) = val )   // write a word at address ptr 

#define PACK( size, alloc )    ( ( size ) | ( alloc ) )        // pack a size and allocated bit into a word
#define GET_SIZE( p )          ( GET( p ) & ( ~0x7 ) )         // get the size from a packed word
#define GET_ALLOC( p )         ( GET( p ) & ( 0x1 ) )          // get the packed allocated bit from a word



// =====================================
// Private Global Variables
// =====================================

static char* heap_listp = NULL;

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
int mm_init()
{
   // create initial heap
   if ( ( heap_listp = mem_sbrk( 4 * WSIZE ) ) == ( void* )-1 )
      return -1;

   // create prologe and epilogue

   
   return 0;
}