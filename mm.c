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
 *   |   |8/1|8/1|hdr|        |ftr|hdr|        |ftr|  ...  |hdr|        |        |ftr|0/1|
 *   |---------------------------------------------         ------------------------------
 *   |       |       |        |       |        |               |        |        |       |
 *   |       |       |        |       |        |               |        |        |       |
 *    \     /        |
 *     \   /         |
 *      \ /      heaplistp   
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
 *      |                          | <- bp
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
 * Block Pointers (bp): point to the first byte of the payload 
 * 
 */
#include "mm.h"
#include "memlib.h"           // mem_sbrk

#include <stdint.h>           // intptr_t, uint32_t, uintptr_t
#include <stdio.h>            // printf
#include <string.h>           // memset

// =====================================
// Constants
// =====================================

#define WSIZE          4          // Word and header/footer size (bytes)
#define DSIZE          8          // Double word size (bytes)
#define ALIGNMENT      8          // Align on 8 byte boundaries
#define CHUNKSIZE      ( 1<<12 )  // Extend heap by this amount (bytes)
#define MIN_BLOCK_SIZE 16         // The minimum block size, 8 byte payload and 8 byte header/footer


// =====================================
// Macros
//=====================================

#define MAX( x, y )            ( ( x ) > ( y ) ? ( x ) : ( y ) )  

#define GET( p )               ( *( uint32_t* )( p ) )         // read a word at address ptr
#define PUT( p, val )          ( *( uint32_t* )( p ) = val )   // write a word at address ptr 

#define PACK( size, alloc )    ( ( size ) | ( alloc ) )        // pack a size, in bytes, and allocated bit into a word
#define GET_SIZE( p )          ( GET( p ) & ( ~0x7 ) )         // get the size from a packed word
#define GET_ALLOC( p )         ( GET( p ) & ( 0x1 ) )          // get the allocated bit from a packed word

// given a pointer to a block, compute the address of its header or footer
#define HDRP( bp )             ( ( char* )( bp ) - WSIZE )     
#define FTRP( bp )             ( ( char* )( bp ) + ( GET_SIZE( HDRP( bp ) ) - DSIZE ) )

// given a pointer to a block (bp), compute the address of the next or previous block pointer
#define NEXT_BLKP( bp )        ( ( char* )( bp ) + ( GET_SIZE( HDRP( bp ) ) ) ) 
#define PREV_BLKP( bp )        ( ( char* )( bp ) - ( GET_SIZE( ( bp ) - DSIZE ) ) )

// round size up to the nearest alignment
#define ALIGN( size )          ( ( ( size ) + ALIGNMENT - 1 ) & ~( ALIGNMENT - 1  ) )


// =====================================
// Private Global Variables
// =====================================

static char* heap_listp = NULL;            // pointer to first block past prologue in heap


// =====================================
// Private Function Prototypes
// =====================================

static void* extend_heap( size_t words );
static void* coalesce( void* bp );
static void* find_block( size_t block_size );
static void  place( void* bp, size_t size );
static void  heapcheck( int verbose );
static void  blockcheck( void* bp );
static void  prologuecheck( void* bp );
static void  printblock( void* bp );


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
   PUT( heap_listp, 0 );                          // padding
   PUT( heap_listp + WSIZE, PACK( 8, 1 ) );       // prologue header
   PUT( heap_listp + 2 * WSIZE, PACK( 8, 1 ) );   // prologue footer
   PUT( heap_listp + 3 * WSIZE, PACK( 0, 1 ) );   // epilogue header

   heap_listp += 4 * WSIZE;
   
   if ( extend_heap( CHUNKSIZE / WSIZE ) == NULL )
      return -1;

   return 0;
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
   if ( size == 0 )
      return NULL;
   
   size_t const block_size = ALIGN( size ) + DSIZE;

   void* bp = find_block( block_size );

   if ( bp == NULL )
   {
      // What is our coalescence policy?  Depending on the policy, should we coalesce before extending the heap?
      size_t const extend = MAX( block_size, CHUNKSIZE );
      if ( ( bp = extend_heap( extend / WSIZE ) ) == NULL )
         return NULL;
   }
   
   place( bp, block_size );
   
   return bp;
}


/**
 * @brief      Free a block of allocated memory
 * 
 * @param ptr  Pointer to allocated block of memory to be freed
 */
void  mm_free( void* ptr )
{
   if ( ptr == NULL )
      return;

   char*  const bp   = ( char* )ptr;
   size_t const size = GET_SIZE( HDRP( bp ) );

   PUT( HDRP( bp ), PACK( size, 0 ) ); 
   PUT( FTRP( bp ), PACK( size, 0 ) );

   coalesce( bp );
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
   if ( size == 0 )
   {
      mm_free( ptr );
      return ptr;
   }

   if ( ptr == NULL )
      return mm_malloc( size );

   size_t const block_size = ALIGN( size ) + DSIZE;
   size_t const old_size   = GET_SIZE( HDRP( ptr ) );

   if ( block_size == old_size )   // nothing to do
      return ptr;
   
   if ( block_size < old_size )
   {
      place( ptr, block_size );
      return ptr;
   }
   
   // block_size > old_size
   char*  const next_block = NEXT_BLKP( ptr );
   size_t const next_size  = GET_SIZE( HDRP( next_block ) );
   size_t const total_size = old_size + next_size;

   // is the next block free and of sufficient size?
   if ( !GET_ALLOC( HDRP( next_block ) ) &&  block_size <= total_size )
   { 
      if ( total_size - block_size >= MIN_BLOCK_SIZE )  // we can split
      {
         PUT( HDRP( ptr ), PACK( block_size, 1 ) );
         PUT( FTRP( ptr ), PACK( block_size, 1 ) );
         
         char*  const next_bp   = NEXT_BLKP( ptr );
         size_t const next_size = total_size - block_size;

         PUT( HDRP( next_bp ), PACK( next_size, 0 ) );
         PUT( FTRP( next_bp ), PACK( next_size, 0 ) );

         coalesce( next_bp );
      }
      else
      {
         PUT( HDRP( ptr ), PACK( total_size, 1 ) );
         PUT( FTRP( ptr ), PACK( total_size, 1 ) );
      }
      return ptr;
   }

   // must allocate and copy
   void* new_ptr = mm_malloc( size );
   memcpy( new_ptr, ptr, old_size - DSIZE );
   mm_free( ptr );
   return new_ptr;
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
   size_t const bytes = num * size;

   char* ptr = mm_malloc( bytes );

   if ( ptr == NULL )
      return NULL;
      
   memset( ptr, 0, bytes );

   return ptr;
}


void mm_check_heap( int verbose )
{
   heapcheck( verbose );
}


// ================================================================================================
//                                         Private
// ================================================================================================


/**
 * @brief  Extend heap with free block and return its block pointer
 * 
 * @param words    Number of words by which to extend the heap
 * @return void*   On success, pointer to new free block
 *                 On error, null pointer
 */
static void* extend_heap( size_t words )
{
   size_t const size = ALIGN( words * WSIZE );   // aligned size in bytes

   char* old_brk;
   
   if ( ( intptr_t )( old_brk = mem_sbrk( size ) ) == -1 )
   {
      return NULL;
   }

   PUT( old_brk - WSIZE, PACK( size , 0 ) );          // free block header
   PUT( old_brk + size - DSIZE, PACK( size, 0 ) );    // free block footer
   PUT( old_brk + size - WSIZE, PACK( 0, 1 ) );       // new epilogue

   return old_brk;
}


/**
 * @brief Boundary tag coalescing
 * 
 * @param bp      Block pointer
 * @return void*  Pointer to coalesced block
 * 
 * Cases:
 *         1. The previous and next blocks are both allocated.
 *         2. The previous block is allocated and the next block is free.
 *         3. The previous block is free and the next block is allocated.
 *         4. The previous and next blocks are both free.
 */
static void* coalesce( void* bp )
{
   size_t const bp_size = GET_SIZE( HDRP( bp ) );
   int const prev_alloc = GET_ALLOC( bp - DSIZE );
   int const next_alloc = GET_ALLOC( HDRP( bp + bp_size ) );

   if ( prev_alloc && next_alloc )                     // Case 1
      return bp;

   if ( prev_alloc && !next_alloc )                    // Case 2
   {
      size_t const next_size = GET_SIZE( HDRP( bp + bp_size ) );
      size_t const new_size  = bp_size + next_size;

      PUT( HDRP( bp ), PACK( new_size, 0 ) );          // create new header
      PUT( FTRP( bp ), PACK( new_size, 0 ) );          // create new footer

      return bp;
   }

   if ( !prev_alloc && next_alloc )                    // Case 3 
   {
      size_t const prev_size = GET_SIZE( bp - DSIZE );
      size_t const new_size  = bp_size + prev_size;
      char*  const prev_bp   = PREV_BLKP( bp );

      PUT( HDRP( prev_bp ), PACK( new_size, 0 ) );    // create new header
      PUT( FTRP( bp ), PACK( new_size, 0 ) );         // create new footer

      return prev_bp;
   }

   if ( !prev_alloc && !next_alloc )                 // Case 4
   {
      size_t const prev_size = GET_SIZE( bp - DSIZE );
      size_t const next_size = GET_SIZE( HDRP( bp + bp_size ) );
      size_t const new_size  = prev_size + bp_size + next_size;
      char*  const prev_bp   = PREV_BLKP( bp );

      PUT( HDRP( prev_bp ), PACK( new_size, 0 ) );          // create header
      PUT( FTRP( NEXT_BLKP( bp ) ), PACK( new_size, 0 ) );  // create footer

      return prev_bp;
   }

   return NULL;
}


/**
 * @brief Locate block on free list
 * 
 * @param block_size  number of bytes required 
 * @return void*      On success, block pointer to free block of at least block_size bytes
 *                    On error, null pointer indicates request can not be satisfied
 * 
 * Implements a naive first fit algorithm that starts at heaplistp and ends when either
 * a free block of sufficient size is found or we encounter the epilogue
 */
static void* find_block( size_t block_size )
{
   char* bp = heap_listp;

   size_t size;

   while ( ( size = GET_SIZE( HDRP( bp ) ) ) != 0 )   // until we reach epilogue
   {
      if ( ( GET_ALLOC( HDRP( bp ) ) == 0 ) && size >= block_size )
      {
         return bp;
      }

      bp += size;
   }

   return NULL;
}


/**
 * @brief Place a block of size bytes at the start of the free block with the block pointer bp
 *        and split it if the excess would be at least equal to the minimum block size
 * 
 * @param bp    block pointer to the free block
 * @param size  number of bytes to place in the free block
 */
static void place( void* bp, size_t size )
{
   size_t const block_size = GET_SIZE( HDRP( bp ) );

   if ( block_size - size >= MIN_BLOCK_SIZE )   // split block
   {
      PUT( HDRP( bp ), PACK( size, 1 ) );
      PUT( FTRP( bp ), PACK( size, 1 ) );
      
      char*  const next_bp   = NEXT_BLKP( bp );
      size_t const next_size = block_size - size;

      PUT( HDRP( next_bp ), PACK( next_size, 0 ) );
      PUT( FTRP( next_bp ), PACK( next_size, 0 ) );

      coalesce( next_bp );
   }
   else
   {
      PUT( HDRP( bp ), PACK( block_size, 1 ) );
      PUT( FTRP( bp ), PACK( block_size, 1 ) );
   }
}


/**
 * @brief Check heap for consistency
 * 
 * @param verbose Print extra information
 */
static void heapcheck( int verbose )
{
   {  // check prologue
      void* const prologuebp = (char*)heap_listp - DSIZE;
      if ( verbose )
         printblock( prologuebp );

      prologuecheck( prologuebp );
   }

   char* bp = NULL;

   for ( bp = heap_listp; GET_SIZE( HDRP( bp ) ) > 0; bp = NEXT_BLKP( bp ) )
   {
      if ( verbose )
         printblock( bp );

      blockcheck( bp );
   }

   // epilogue
   if ( verbose )
      printblock( bp );

   if ( GET_SIZE( HDRP( bp ) ) != 0 && GET_ALLOC( HDRP( bp ) ) != 0x1 )
   {
      printf( "Error: Bad epilogue\n");
   }
}


/**
 * @brief Check a given block for alignment and consistency between header and footer
 * 
 * @param bp Block pointer to block we will check
 * 
 * 
 */
static void blockcheck( void* bp )
{
   // address of bp should be aligned
   if( ( (uintptr_t)bp % ALIGNMENT ) != 0 )
   {
      printf( "Error: %p is not %d byte aligned\n", bp, ALIGNMENT );
   }

   size_t const h_size = GET_SIZE( HDRP( bp ) );

   if ( h_size < MIN_BLOCK_SIZE )
   {
      printf( "Error: Block size (%zu) is less than the minimum block size (%d)", h_size, MIN_BLOCK_SIZE );
   }

   if ( h_size % ALIGNMENT != 0 )
   {
      printf( "Error: Block size (%zu) is not %d byte aligned\n", h_size, ALIGNMENT );
   }

   if ( GET( HDRP( bp ) ) != GET( FTRP( bp ) ) )
   {
      printf( "Error: header does not match footer\n" );
   }
}


/**
 * @brief Check prologue block for appropriate format
 * 
 * @param bp Block pointer to prologue
 */
static void prologuecheck( void* bp )
{
   if ( ( (uintptr_t)bp % ALIGNMENT ) != 0 )
   {
      printf( "Error: Bad Prologue - %p is not %d byte aligned\n", bp, ALIGNMENT );
      return;
   }

   size_t const h_size  = GET_SIZE( HDRP( bp ) );
   size_t const f_size  = GET_SIZE( FTRP( bp ) );
   size_t const h_alloc = GET_ALLOC( HDRP( bp ) );
   size_t const f_alloc = GET_ALLOC( FTRP( bp ) );

   if ( h_size != DSIZE || f_size != DSIZE || h_alloc != 0x1 || f_alloc != 0x1 )
   {
      printf( "Error: Bad Prologue\n");
      printblock( bp );
   }
}


/**
 * @brief Print header and footer contents of a given block
 * 
 * @param bp Block pointer for whose contents we will print
 */
static void printblock( void* bp )
{
   size_t const h_size  = GET_SIZE( HDRP( bp ) );

   if ( h_size == 0 )
   {
      printf( "%p: EOL\n", bp );       // epilogue 
      return;
   }

   size_t const h_alloc = GET_ALLOC( HDRP( bp ) );
   size_t const f_size  = GET_SIZE( FTRP( bp ) );
   size_t const f_alloc = GET_ALLOC( FTRP( bp ) );

   printf( "%p: header: [%zu:%c] - footer: [%zu:%c]\n", bp, 
            h_size, ( h_alloc ? 'a' : 'f' ),
            f_size, ( f_alloc ? 'a' : 'f' ) );
}

