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
#define GET_NEXT_PTR( bp )           ( GET_PTR( bp ) )
#define GET_PREV_PTR( bp )           ( GET_PTR( bp + DSIZE ) )

// given a pointer to a block, compute the address of its header or footer
#define HDRP( bp )                   ( ( byte* )( bp ) - WSIZE )     
#define FTRP( bp )                   ( ( byte* )( bp ) + ( GET_SIZE( HDRP( bp ) ) - DSIZE ) )

// given a pointer to a block payload (bp), compute the address of the next or previous block payload pointer
#define NEXT_BLKP( bp )              ( ( byte* )( bp ) + ( GET_SIZE( HDRP( bp ) ) ) ) 
#define PREV_BLKP( bp )              ( ( byte* )( bp ) - ( GET_SIZE( ( bp ) - DSIZE ) ) )

// round size up to the nearest alignment
#define ALIGN( size )                ( ( ( size ) + ALIGNMENT - 1 ) & ~( ALIGNMENT - 1 ) ) 

// Compute the block size required to satisfy an allocation request
#define BLOCK_SIZE( size )           ( MAX( MIN_BLOCK_SIZE, ( ALIGN( size ) ) ) )      


// =====================================
// Private Global Variables
// =====================================

static byte* heap_listp = NULL;            // pointer to first block past prologue in heap
static byte* free_listp = NULL;


// =====================================
// Private Function Prototypes
// =====================================


static void* extend_heap( size_t size );   // extend the heap by size bytes
static void* coalesce( void* bp );         // coalesce adjacent free blocks
static void  free_list_insert( void* bp ); // insert into free list
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
int mm_init( void )
{
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
   size_t const block_size = BLOCK_SIZE( size );

   byte* old_brk;
   
   if ( ( intptr_t )( old_brk = mem_sbrk( block_size ) ) == -1 )
   {
      return NULL;
   }

   uint32_t const prev_alloc = GET_PREV_ALLOC( old_brk - WSIZE );

   PUT( old_brk - WSIZE, PACK( block_size , prev_alloc, 0 ) );                // free block header
   PUT( old_brk + block_size - DSIZE, PACK( block_size, prev_alloc, 0 ) );    // free block footer
   PUT( old_brk + block_size - WSIZE, PACK( 0, 0, 1 ) );                      // new epilogue

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
 */
static void* coalesce( void* bp )
{
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

}



/**
 * @brief Check prologue block for appropriate format
 * 
 * @param bp Block payload pointer to prologue
 */
static void prologuecheck( void* bp )
{

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
      byte*  const next_ptr     = ( byte* )GET_NEXT_PTR( bp );
      byte*  const prev_ptr     = ( byte* )GET_PREV_PTR( bp );

      printf( "%p: header: [%zu:%c%c] | next: %p | prev: %p | footer: [%zu:%c%c]\n", bp, 
               h_size, ( h_prev_alloc ? 'a' : 'f' ), ( h_alloc ? 'a' : 'f' ),
               next_ptr, prev_ptr, 
               f_size, ( f_prev_alloc ? 'a' : 'f' ), ( f_alloc ? 'a' : 'f' )
            );
   }
}