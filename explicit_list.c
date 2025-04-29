/**
 * @file    explicit_list.c
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Source file for mm.h that uses explicit free list implementation
 * @version 0.1
 * @date    2025-04-22
 * 
 * @copyright Copyright (c) 2025
 * 
 * Implementation of malloc-like allocator using explicit free list
 * 
 * 16 byte aligned
 * 32 byte minimum block size
 * 
 * free_listp ----------|
 *                      |
 * heap_listp-----------|                      
 *                      |                     
 *    start             |                    
 *      ^               |                       
 *      |               |         
 *      |               |               |                          |               |               |
 *      |               |       |       |       |                  |       |       |       |       |
 *      |--------------------------------------------          -------------------------------------
 *      |   |hdr|ftr|hdr| next  |  prev |       |ftr|   ...    |hdr|       |       |       |   |epi|
 *      |--------------------------------------------          -------------------------------------
 *       \           /\                            /            \                             /
 *        \prologue /  \      free block          /              \         allocated         /
 *             
 *         
 * 
 * Block Format:    
 *                               ---- unused
 *                              / --- previous block's allocation status
 *                             / / -- current block's allocation status
 *                            / / /
 *    31           ...       2 1 0 
 *    ----------------------------
 *    |          Size       |    |     Header    
 *    |--------------------------| 
 *    |          Next*           | <-- bp
 *    |     (free block only)    |
 *    |--------------------------|
 *    |          Prev*           |
 *    |     (free block only)    |
 *    |                          |
 *    |--------------------------|
 *    |                          |
 *    |        Payload           |
 *    |  (allocated block only)  |
 *    |                          |
 *    |                          |
 *    |--------------------------|
 *    |        Padding           |
 *    |       (Optional)         |
 *    |--------------------------|
 *    |          Size       |    |     Footer (Free Block Only)
 *    ----------------------------
 * 
 * 
 * 
 *     
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
#define MIN_BLOCK_SIZE               32          // Minimum Block Size


// =====================================
// Macros
// =====================================

#define MAX( x, y )                  ( ( x ) > ( y ) ? ( x ) : ( y ) )  
 
#define GET( p )                     ( *( uint32_t* )( p ) )                          // read a word at address p
#define PUT( p, val )                ( *( uint32_t* )( p ) = val )                    // write a word at address p

#define PACK( size, prev, alloc )    ( ( size ) | ( prev << 1 ) | ( alloc ) )              // pack a size (in bytes), the previous block's allocation status and the current block's allocation status into a word

#define GET_SIZE( p )                ( GET( p ) & ( ~0x7 ) )                          // get the size from a packed word
#define GET_ALLOC( p )               ( GET( p ) & ( 0x1 ) )                           // get the allocated bit from a packed word
#define GET_PREV_ALLOC( p )          ( ( GET( p ) & ( 0x2 ) ) >> 1 )                  // get the previous allocated bit from packed word
#define SET_PREV_ALLOC( p )          ( PUT( ( p ), GET( ( p ) ) | 0x2 ) )             // set the previous allocated bit
#define CLEAR_PREV_ALLOC( p )        ( PUT( ( p ), GET( ( p ) ) & ~0x2 ) )            // clear the previous allocated bit

#define PUT_PTR( p, ptr )            ( *( uintptr_t* )( p ) = ( uintptr_t )( ptr ) )  // write a pointer at address p
#define GET_PTR( p )                 ( *( uintptr_t* )( p ) )                         // read a pointer at address p

#define PUT_NEXT_PTR( bp, ptr )      ( PUT_PTR( ( bp ), ( ptr ) ) )
#define PUT_PREV_PTR( bp, ptr )      ( PUT_PTR( ( bp + DSIZE ), ( ptr ) ) )
#define GET_NEXT_PTR( bp )           ( ( byte* )GET_PTR( bp ) )
#define GET_PREV_PTR( bp )           ( ( byte* )GET_PTR( bp + DSIZE ) )

// given a pointer to a block, compute the address of its header or footer
#define HDRP( bp )                   ( ( byte* )( bp ) - WSIZE )     
#define FTRP( bp )                   ( ( byte* )( bp ) + ( GET_SIZE( HDRP( bp ) ) - DSIZE ) )

// given a pointer to a block payload (bp), compute the address of the next or previous block payload pointer
#define NEXT_BLKP( bp )              ( ( byte* )( bp ) + ( GET_SIZE( HDRP( bp ) ) ) ) 
#define PREV_BLKP( bp )              ( ( byte* )( bp ) - ( GET_SIZE( ( bp ) - DSIZE ) ) )

// round size up to the nearest alignment
#define ALIGN( size )                ( ( ( size ) + ALIGNMENT - 1 ) & ~( ALIGNMENT - 1 ) ) 

// Compute the block size required to satisfy an allocation request
#define BLOCK_SIZE( size )           ( MAX( MIN_BLOCK_SIZE, ( ALIGN( size + WSIZE ) ) ) )      


// =====================================
// Private Global Variables
// =====================================

static byte* heap_listp = NULL;            // pointer to first block past prologue in heap
static byte* free_listp = NULL;


// =====================================
// Private Function Prototypes
// =====================================


static void* extend_heap( size_t size );      // extend the heap by size bytes
static void* coalesce( void* bp );            // coalesce adjacent free blocks
static void  free_list_insert( void* bp );    // insert into free list
static void  free_list_remove( void* bp );
static void* find_block( size_t block_size ); // find block on free list
static void  place_allocation( void* bp, size_t size );
static void  heapcheck( int verbose );
static void  blockcheck( void* bp );
static void  prologuecheck( void* bp );
static void  free_list_check( int verbose );
static void  printblock( void* bp );


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
   heap_listp = NULL;
   free_listp = NULL;

   // create initial heap
   if ( ( heap_listp = mem_sbrk( 4 * WSIZE ) ) == ( void* )-1 )
      return -1;
   
   // create prologue and epilogue
   PUT( heap_listp, 0 );                               // padding
   PUT( heap_listp + WSIZE, PACK( DSIZE, 1, 1 ) );     // prologue: header
   PUT( heap_listp + DSIZE, PACK( DSIZE, 1, 1) );      // prologue: footer
   PUT( heap_listp + 3 * WSIZE, PACK( 0, 1, 1 ) );     // epilogue

   if ( extend_heap( CHUNKSIZE ) == NULL )
      return -1;
   
   heap_listp += 4 * WSIZE;
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

   size_t const block_size = BLOCK_SIZE( size );

   void* bp = find_block( block_size );

   if ( bp == NULL )
   {
      size_t const extend = MAX( block_size, CHUNKSIZE );
      if ( ( bp = extend_heap( extend ) ) == NULL )
         return NULL;
   }

   place_allocation( bp, block_size );

   return bp;
}



/**
 * @brief      Free a block of allocated memory
 * 
 * @param ptr  Pointer to allocated block of memory to be freed
 */
void mm_free( void* ptr )
{
   if ( ptr == NULL )
      return;

   // change our allocation status
   byte*  const bp         = ( byte* )ptr;
   size_t const size       = GET_SIZE( HDRP( bp ) );
   int    const prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );

   PUT( HDRP( bp ), PACK( size, prev_alloc, 0 ) ); 
   PUT( FTRP( bp ), PACK( size, prev_alloc, 0 ) );

   // change the allocation bit in the next block's header and footer (if it exists)
   byte* const next_bp = NEXT_BLKP( bp );
   CLEAR_PREV_ALLOC( HDRP( next_bp ) );

   if ( !GET_ALLOC( HDRP( next_bp ) ) )
   {
      CLEAR_PREV_ALLOC( FTRP( next_bp ) );
   }

   free_list_insert( bp );
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

   size_t const block_size = BLOCK_SIZE( size );
   size_t const old_size   = GET_SIZE( HDRP( ptr ) );

   if ( block_size == old_size )
      return ptr;
   
   if ( block_size < old_size )
   {
      place_allocation( ptr, block_size );
      return ptr;
   }

   // block_size > old_size
   byte*  const next_bp    = NEXT_BLKP( ptr );
   size_t const next_size  = GET_SIZE( HDRP( next_bp ) );
   size_t const total_size = old_size + next_size;

   if ( !GET_ALLOC( HDRP( next_bp ) ) && block_size <= total_size )
   {
      int const prev_alloc = GET_PREV_ALLOC( HDRP( ptr ) );
      
      PUT( HDRP( ptr ), PACK( block_size, prev_alloc, 1 ) );

      if ( total_size - block_size >= MIN_BLOCK_SIZE )  // we can split the following block
      {
         byte*  const split_bp  = NEXT_BLKP( ptr );
         size_t const next_size = total_size - block_size;

         PUT( HDRP( split_bp ), PACK( next_size, 1, 0 ) );
         PUT( FTRP( split_bp ), PACK( next_size, 1, 0 ) );

         free_list_insert( split_bp );
      }

      free_list_remove( next_bp );
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

   byte* ptr = mm_malloc( bytes );

   if ( ptr == NULL )
      return NULL;
      
   memset( ptr, 0, bytes );

   return ptr;
}



/**
 * @brief Check the heap for consistency
 * 
 * @param verbose  If 0, don't print verbose output
 *                 
 */
void mm_check_heap( int verbose )
{
   heapcheck( verbose );
}



// ================================================================================================
//                                         Private
// ================================================================================================


/**
 * @brief  Extend heap with free block and return its block payload pointer
 * 
 * @param size     Number of bytes by which to extend the heap
 * @return void*   On success, pointer to new free block
 *                 On error, null pointer
 */
static void* extend_heap( size_t size )
{
   byte* old_brk;
   
   if ( ( intptr_t )( old_brk = mem_sbrk( size ) ) == -1 )
   {
      return NULL;
   }

   uint32_t const prev_alloc = GET_PREV_ALLOC( old_brk - WSIZE );

   PUT( old_brk - WSIZE, PACK( size , prev_alloc, 0 ) );          // free block header
   PUT( old_brk + size - DSIZE, PACK( size, prev_alloc, 0 ) );    // free block footer
   PUT( old_brk + size - WSIZE, PACK( 0, 0, 1 ) );                // new epilogue

   free_list_insert( old_brk );

   return coalesce( old_brk );
}



/**
 * @brief Boundary tag coalescing
 * 
 * @param bp      Block payload pointer to recently freed block
 * @return void*  Pointer to coalesced block
 * 
 * Cases:
 *         1. The previous and next blocks are both allocated.
 *         2. The previous block is allocated and the next block is free.
 *         3. The previous block is free and the next block is allocated.
 *         4. The previous and next blocks are both free.
 * 
 */
static void* coalesce( void* bp )
{
   size_t const bp_size    = GET_SIZE( HDRP( bp ) );
   int    const prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );
   int    const next_alloc = GET_ALLOC( HDRP( bp + bp_size ) );

   if ( prev_alloc && next_alloc )                       // Case 1
   {
      printf( "coalesce: Case 1\n" );
      return bp;
   }

   if ( prev_alloc && !next_alloc )                      // Case 2
   {
      printf( "coalesce: Case 2\n" );
      byte*  const next_bp   = bp + bp_size;
      size_t const next_size = GET_SIZE( HDRP( next_bp ) );
      size_t const new_size  = bp_size + next_size;

      PUT( HDRP( bp ), PACK( new_size, 1, 0 ) );          // create new header
      PUT( FTRP( bp ), PACK( new_size, 1, 0 ) );          // create new footer

      // adjust free list pointers
      free_list_remove( next_bp );

      return bp;
   }

   if ( !prev_alloc && next_alloc )                     // Case 3
   {
      printf( "coalesce: Case 3\n" );
      size_t const prev_size = GET_SIZE( bp - DSIZE );
      size_t const new_size  = bp_size + prev_size;
      byte*  const prev_bp   = PREV_BLKP( bp );

      // we know that the block before the previous must be allocated because
      // it is one of our invariants
      PUT( HDRP( prev_bp ), PACK( new_size, 1, 0 ) );   // create new header
      PUT( FTRP( prev_bp ), PACK( new_size, 1, 0 ) );   // create new footer
      
      // adjust free list pointers
      free_list_remove( bp );

      return prev_bp;
   }

   if ( !prev_alloc && !next_alloc )                  // Case 4
   {
      printf( "coalesce: Case 4\n" );
      byte*  const prev_bp   = PREV_BLKP( bp );
      byte*  const next_bp   = NEXT_BLKP( bp );
      size_t const prev_size = GET_SIZE( bp - DSIZE );
      size_t const next_size = GET_SIZE( HDRP( next_bp ) );
      size_t const new_size  = prev_size + bp_size + next_size;
      
      // we know that the block before the previous and after the next must 
      // be allocated because it is one of our invariants
      PUT( HDRP( prev_bp ), PACK( new_size, 1, 0 ) );  // create header
      PUT( FTRP( prev_bp ), PACK( new_size, 1, 0 ) );  // create footer

      // adjust free list pointers
      free_list_remove( bp );
      free_list_remove( next_bp );

      return prev_bp;
   }

   return bp;      // dummy line
}


/**
 * @brief Insert block payload pointer into the start of free list
 * 
 * @param bp Block payload pointer to insert
 */
static void free_list_insert( void* bp )
{
   byte* const old_start = free_listp;
   byte* const new_bp    = bp;

   free_listp = new_bp;

   if ( old_start )
   {
      PUT_NEXT_PTR( new_bp, old_start );
      PUT_PREV_PTR( new_bp, NULL );
      PUT_PREV_PTR( old_start, new_bp );
   }
   else  // NULL: free list is empty
   {
      PUT_NEXT_PTR( new_bp, NULL );
      PUT_PREV_PTR( new_bp, NULL );
   }
}


/**
 * @brief Remove a block payload pointer from the free list
 * 
 * @param bp  Block payload pointer on the free list to remove
 */
static void free_list_remove( void* bp )
{
   // adjust free list pointers
   byte* const fl_prev_bp = GET_PREV_PTR( bp );
   byte* const fl_next_bp = GET_NEXT_PTR( bp );

   if ( fl_prev_bp )
      PUT_NEXT_PTR( fl_prev_bp, fl_next_bp );
   else
      free_listp = fl_next_bp;
   if ( fl_next_bp )
      PUT_PREV_PTR( fl_next_bp, fl_prev_bp );
}

/**
 * @brief Locate block on free list
 * 
 * @param block_size  number of bytes required 
 * @return void*      On success, block payload pointer to free block of at least block_size bytes
 *                    On error, null pointer indicates request can not be satisfied
 * 
 * Implements a naive first fit algorithm that searches the free list for
 * a free block of sufficient size
 */
static void* find_block( size_t block_size )
{
   byte* bp = free_listp;

   while ( bp )
   {
      if ( !GET_ALLOC( HDRP( bp ) ) && GET_SIZE( HDRP( bp ) ) >= block_size )
      {
         return bp;
      }
      bp = GET_NEXT_PTR( bp );
   }

   return NULL;
}


/**
 * @brief Place an allocated block of size bytes at the start of the free block
 *        with the block payload pointer bp and split it if the excess would be
 *        at least equal to the minimum free block size
 * 
 * @param bp   Block payload pointer 
 * @param size Size of allocation
 */
static void place_allocation( void* bp, size_t size )
{
   size_t const block_size = GET_SIZE( HDRP( bp ) );
   int    const prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );
   byte*  const next_bp    = NEXT_BLKP( bp );

   if ( block_size - size >= MIN_BLOCK_SIZE )    // split block
   {
      PUT( HDRP( bp ), PACK( size, prev_alloc, 1 ) );

      byte*  const next_bp   = NEXT_BLKP( bp );
      size_t const next_size = block_size - size;

      PUT( HDRP( next_bp ), PACK( next_size, prev_alloc, 0 ) );
      PUT( FTRP( next_bp ), PACK( next_size, prev_alloc, 0 ) );

      free_list_insert( next_bp );
      free_list_remove( bp );
   }
   else
   {
      PUT( HDRP( bp ), PACK( block_size, prev_alloc, 1 ) );

      SET_PREV_ALLOC( HDRP( next_bp ) );
      if ( !GET_ALLOC( HDRP( next_bp ) ) )
      {
         SET_PREV_ALLOC( FTRP( next_bp ) );
      }

      free_list_remove( bp );
   }
}


/**
 * @brief Check heap for consistency
 * 
 * @param verbose Print extra information
 */
static void heapcheck( int verbose )
{
   printf( "%p: heap_listp\n", heap_listp );
   printf( "%p: free_listp\n", free_listp );

   // check prologue
   {
      byte* const prologuebp = heap_listp - DSIZE;

      if ( verbose )
         printblock( prologuebp );
      
      prologuecheck( prologuebp );
   }

   byte* bp = NULL;

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

   if ( verbose )
   {
      puts( "Free list check:" );
   }
   
   free_list_check( verbose );
}


/**
 * @brief Check a given block for alignment and consistency between header and footer
 * 
 * @param bp Block payload pointer to block we will check
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

   int const is_allocated = GET_ALLOC( HDRP( bp ) );

   if ( !is_allocated && GET( HDRP( bp ) ) != GET( FTRP( bp ) ) )
   {
      printf( "Error: header does not match footer\n" );
   }

   if ( !GET_PREV_ALLOC( HDRP( bp ) ) )
   {
      byte* const prev_bp = PREV_BLKP( bp );

      if ( GET_ALLOC( prev_bp ) )
      {
         printf( "Error: Previous block is allocated when current block's header indicates that it is free\n" );
      }
   }
}



/**
 * @brief Check prologue block for appropriate format
 * 
 * @param bp Block payload pointer to prologue
 */
static void prologuecheck( void* bp )
{
   if ( GET( bp - WSIZE ) != GET( bp ) )
   {
      printf( "Error: Bad Prologue - Header & Footer are NOT consistent\n" );
   }
}


/**
 * @brief Consistency check of free_list
 * 
 * @param verbose Print verbose output
 */
static void free_list_check( int verbose )
{
   byte* bp   = free_listp;
   byte* prev = NULL;

   while ( bp )
   {
      byte* const next_bp = GET_NEXT_PTR( bp );
      byte* const prev_bp = GET_PREV_PTR( bp );

      if ( verbose )
      {
         printf( "%p: next: %p, prev: %p\n", bp, next_bp, prev_bp );
      }

      if ( prev != prev_bp )
      {
         printf( "Error: Bad free list pointers\n" );
      }

      prev = bp;
      bp   = next_bp;
   }
   if ( verbose )
      puts( "" );
}


/**
 * @brief Print header and footer (optional) contents of a given block
 * 
 * @param bp Block payload pointer for whose contents we will print
 */
static void printblock( void* bp )
{
   size_t const h_size       = GET_SIZE( HDRP( bp ) );
   int    const is_allocated = GET_ALLOC( HDRP( bp ) );

   if ( h_size == 0 )     // epilogue
   {
      printf( "%p: Epilogue: [%zu:%c%c]\n", bp, 
         h_size, ( GET_PREV_ALLOC( HDRP( bp ) ) ? 'a' : 'f' ), ( is_allocated ? 'a' : 'f' ) );
      return;
   }

   if ( bp == heap_listp - DSIZE )
   {
      size_t const f_size       = GET_SIZE( FTRP( bp ) );
      int    const h_prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );
      int    const f_prev_alloc = GET_PREV_ALLOC( FTRP( bp ) );
      int    const h_alloc      = is_allocated;
      int    const f_alloc      = GET_ALLOC( FTRP( bp ) );

      printf( "%p: Prologue: header: [%zu:%c%c] | footer: [%zu:%c%c]\n", bp, 
         h_size, ( h_prev_alloc ? 'a' : 'f' ), ( h_alloc ? 'a' : 'f' ),
         f_size, ( f_prev_alloc ? 'a' : 'f' ), ( f_alloc ? 'a' : 'f' ) 
      );
      return;
   }

   if ( is_allocated )
   {
      int const h_alloc      = GET_ALLOC( HDRP( bp ) );
      int const h_prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );

      printf( "%p: header: [%zu:%c%c]\n", bp, 
               h_size, ( h_prev_alloc ? 'a' : 'f' ), ( h_alloc ? 'a' : 'f' ) );
   }
   else   // free block
   {
      size_t const f_size       = GET_SIZE( FTRP( bp ) );
      int    const h_prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );
      int    const f_prev_alloc = GET_PREV_ALLOC( FTRP( bp ) );
      int    const h_alloc      = is_allocated;
      int    const f_alloc      = GET_ALLOC( FTRP( bp ) );
      byte*  const next_ptr     = GET_NEXT_PTR( bp );
      byte*  const prev_ptr     = GET_PREV_PTR( bp );

      printf( "%p: header: [%zu:%c%c] | next: %p | prev: %p | footer: [%zu:%c%c]\n", bp, 
               h_size, ( h_prev_alloc ? 'a' : 'f' ), ( h_alloc ? 'a' : 'f' ),
               next_ptr, prev_ptr, 
               f_size, ( f_prev_alloc ? 'a' : 'f' ), ( f_alloc ? 'a' : 'f' )
            );
   }
}