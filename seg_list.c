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

#include <assert.h>
#include <inttypes.h>                 // fixed width format macros for printf
#include <stdint.h>                   // uint32_t, uintptr_t
#include <stdio.h>                    // printf
#include <string.h>                   // memset

#include <stdlib.h>

// =====================================
// Typedefs
// =====================================

typedef unsigned char byte;
typedef uint64_t      bitvector[4];

struct seg_list_header;
typedef struct seg_list_header seg_list_header_t;

// =====================================
// Types
// =====================================

struct seg_list_header
{
   seg_list_header_t* next;
   uint64_t           vector[4];
   uint32_t           size;
   uint32_t           min;
};


// =====================================
// Constants
// =====================================

#define WSIZE                        4              // Word size (bytes)
#define DSIZE                        8              // Double word size (bytes)
#define CHUNKSIZE                    ( 1u << 12u )  // Extend heap by this amount (bytes)
#define ALIGNMENT                    16             // Align on 16 byte boundaries
#define MIN_BIG_BLOCK                144            // Minimum, 16 byte aligned, block size of allocation on explicit free list

#define SEG16_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 16u  )
#define SEG32_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 32u  )
#define SEG48_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 48u  )
#define SEG64_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 64u  )
#define SEG128_ENTRIES               ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 128u )


// =====================================
// Macros
// =====================================

#define MAX( x, y )                  ( ( x ) > ( y ) ? ( x ) : ( y ) )  

#define SET_BIT( word64, bit )       ( word64 |=  ( 1ull << bit ) )
#define CLEAR_BIT( word64, bit )     ( word64 &= ~( 1ull << bit ) )


// ---------------------------------------
// Macros to implement explicit free list
// ---------------------------------------

#define GET( p )                     ( *( uint32_t* )( p ) )                          // read a word at address p
#define PUT( p, val )                ( *( uint32_t* )( p ) = val )                    // write a word at address p

#define PACK( size, prev, alloc )    ( ( size ) | ( prev << 1u ) | ( alloc ) )         // pack a size (in bytes), the previous block's allocation status and the current block's allocation status into a word

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
#define BLOCK_SIZE( size )           ( MAX( MIN_BIG_BLOCK, ( ALIGN( size + WSIZE ) ) ) )      
// Compute chunk size rounded up to the nearest 4K 
#define CHUNK_SIZE( size )           ( ( ( ( uint64_t )( size ) >> 12 ) + 1 ) << 12 ) 


// =====================================
// Private Global Variables
// =====================================

static byte* free_list_16   = NULL;  // sizes 1 to 16
static byte* free_list_32   = NULL;  // sizes 17 to 32
static byte* free_list_48   = NULL;  // sizes 33 to 48
static byte* free_list_64   = NULL;  // sizes 49 to 64
static byte* free_list_128  = NULL;  // sizes 65 to 128
static byte* free_list_big  = NULL;  // sizes > 128      - implemented as explicit free list
static byte* explicit_chunk = NULL;  // pointer to chunks of memory used for explicit free list


// =====================================
// Private Function Prototypes
// =====================================

void* create_new_seglist( byte** free_list, uint32_t size );
void  init_seglist_header( void* ptr, uint32_t size );
void  insert_new_seglist( byte** free_list, void* entry );
int   find_free_offset( bitvector* bv, uint32_t num_entries );

seg_list_header_t* get_seg_list_header( void* ptr );

void* do_malloc( byte** seg_list, uint32_t size, uint32_t capacity );
void* do_malloc_big( size_t size );
void  do_free_big( void* ptr );
void* do_big_realloc( void* ptr, size_t size );

inline int      seg_list_capacity( uint32_t size );
inline uint32_t seg_list_min_size( uint32_t size );

void  print_seglist_headers( void* ptr );

// ----------------------------------
// Functions for explicit free list
// ----------------------------------

static void  add_explicit_chunk( size_t size );
static void* coalesce( void* bp );                        // coalesce adjacent free blocks
static void  free_list_insert( void* bp );                // insert into free list
static void  free_list_remove( void* bp );
static void* find_block( size_t block_size );             // find block on free list
static void  place_allocation( void* bp, size_t size );
static void  heapcheck( int verbose );
static void  check_chunk( byte* chunk, int verbose );
static void  check_prologue( byte* bp, int verbose );
static void  free_list_check( int verbose );
static void  blockcheck( void* bp );
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
   assert( sizeof( seg_list_header_t ) == 48u );

   free_list_16   = NULL;
   free_list_32   = NULL;
   free_list_48   = NULL;
   free_list_64   = NULL;
   free_list_128  = NULL;
   free_list_big  = NULL;
   explicit_chunk = NULL;

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

   if ( size <= 16 )
   {
      return do_malloc( &free_list_16, 16u, SEG16_ENTRIES );
   }
   else if( size <= 32 )
   {
      return do_malloc( &free_list_32, 32u, SEG32_ENTRIES );
   }
   else if ( size <= 48 )
   {
      return do_malloc( &free_list_48, 48u, SEG48_ENTRIES );
   }
   else if( size <= 64 )
   {
      return do_malloc( &free_list_64, 64u, SEG64_ENTRIES );
   }
   else if( size <= 128 )
   {
      return do_malloc( &free_list_128, 128u, SEG128_ENTRIES );
   }
   else   // size > 128
   {
      return do_malloc_big( size );
   }

   return NULL;
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

   // to determine how to free the ptr we must determine to which free list it belongs
   seg_list_header_t* const seg_list = get_seg_list_header( ptr );

   if ( seg_list )
   {
      uintptr_t const ptr_addr = ( uintptr_t )ptr;
      uintptr_t const seg_addr = ( uintptr_t )seg_list;
      int       const offset   = ( ptr_addr - ( seg_addr + sizeof( seg_list_header_t ) ) ) / seg_list->size;
      int       const word64   = offset / 64;
      int       const bit      = offset % 64;
      
      CLEAR_BIT( seg_list->vector[word64], bit );
      return;
   }

   do_free_big( ptr );
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

   // we must allocate and copy except under the following conditions
   // - the previous allocation was not on a seg list and the following block is free and 
   //   of sufficient size
   // - the previous allocation was on a seg list and the requested size is within the same
   //   seg list
   //   - ie:  the previous allocation was for 64 bytes and the requested realloc was for 90 bytes

   seg_list_header_t* const seg_listp = get_seg_list_header( ptr );

   if ( seg_listp )
   {
      if ( size <= seg_listp->size )
      {
         return ptr;
      }
      else
      {
         void* new_ptr = mm_malloc( size );
         memcpy( new_ptr, ptr, seg_listp->size );
         return new_ptr;
      }
   }
      
   // ptr was allocated on the explicit free list
   // implementation is the same as explicit free list mm_realloc
   return do_big_realloc( ptr, size );
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

   void* ptr = mm_malloc( bytes );

   if ( ptr == NULL )
      return NULL;

   memset( ptr, 0 , bytes );
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
   if ( verbose )
   {
      print_seglist_headers( free_list_16 );
      print_seglist_headers( free_list_32 );
      print_seglist_headers( free_list_48 );
      print_seglist_headers( free_list_64 );
      print_seglist_headers( free_list_128 );
  }

  heapcheck( verbose );
}


// ================================================================================================
//                                         Private
// ================================================================================================



/**
 * @brief Create a new seglist object
 * 
 * @param free_list 
 * @return void*      On success, pointer to newly allocated 4K block initialized as a seg list
 *                    On error, NULL
 */
void* create_new_seglist( byte** free_list, uint32_t size )
{
   void* new_chunk;  
   
   if ( ( intptr_t )( new_chunk = mem_sbrk( CHUNKSIZE ) ) == -1 )
   {
      return NULL;
   }

   init_seglist_header( new_chunk, size );
   insert_new_seglist( free_list, new_chunk );

   return new_chunk;
}


/**
 * @brief Given a pointer to a 4K block, initialize the seg list
 * 
 * @param ptr Pointer to 4K block for seg list
 */
void init_seglist_header( void* ptr, uint32_t size )
{
   seg_list_header_t const tmp = { .next = NULL, .vector = { 0, 0, 0, 0 }, 
                                   .size = size, .min    = seg_list_min_size( size ) };

   memcpy( ptr, &tmp, sizeof( tmp ) );
}


/**
 * @brief Insert new seg list block into start of free list
 * 
 * @param free_list The free list to insert into
 * @param entry     The new seg list to insert
 */
void insert_new_seglist( byte** free_list, void* entry )
{
   seg_list_header_t* const header = entry;
   
   header->next = (seg_list_header_t*)*free_list;
   *free_list   = entry;
}


/**
 * @brief Find the first zero bit in the bitvector and then set it to 1
 * 
 * @param bv           Pointer to bitvector 
 * @param num_entries  Number of valid entries in the bit vector
 * @return int         On success, offset in free list that is free 
 *                     On error, -1
 * 
 * 
 */
int find_free_offset( bitvector* bv, uint32_t num_entries )
{
   for ( uint32_t v = 0u; v < 4u; ++v )
   {
      uint64_t mask = 1u;

      for ( uint32_t bit = 0u; bit < 64u; ++bit )
      {
         if ( 64u * v + bit >= num_entries )
            return -1;

         uint32_t const is_free = !( ( *bv )[v] & mask );

         if ( is_free )
         {
            SET_BIT( ( *bv )[v], bit );
            return v * 64u + bit;
         }
         mask = ( mask << 1u );
      }
   }
   return -1;
}


/**
 * @brief Get the seg list that contains a given pointer
 * 
 * @param ptr                  Pointer to memory allocated by mm_malloc, mm_realloc or mm_calloc
 * @return seg_list_header_t*  On success, pointer to seg list
 *                             On failure, NULL
 */
seg_list_header_t* get_seg_list_header( void* ptr )
{
   byte* const tmp          = ptr;
   byte* const seg_lists[] = { free_list_16, free_list_32, free_list_48, free_list_64, free_list_128 };
   
   for ( unsigned idx = 0u; idx < sizeof( seg_lists ) / sizeof( seg_lists[0] ); ++idx )
   {
      byte* searcher = seg_lists[idx];
      
      while ( searcher )
      {
         seg_list_header_t* pheader = ( seg_list_header_t* )searcher;

         if ( tmp > searcher && tmp < searcher + CHUNKSIZE )
         {
            return pheader;
         }
         searcher = ( byte* )pheader->next;
      }
   }

   return NULL;
}


void* do_malloc( byte** seg_list, uint32_t size, uint32_t capacity )
{
   if ( *seg_list == NULL )
   {
      if ( create_new_seglist( seg_list, size ) == NULL )
      {
         return NULL;
      }
   }

   byte* blockp = *seg_list;

   while ( 1 )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )blockp;
      int                const offset  = find_free_offset( &pheader->vector, capacity );

      if ( offset == -1 )
      {
         if ( pheader->next )
         {
            blockp = ( byte* )pheader->next;
         }
         else
         {
            if ( ( blockp = create_new_seglist( seg_list, size ) ) == NULL )
            {
               return NULL;
            }
         }
      }
      else
      {
         return blockp + offset * size + sizeof( seg_list_header_t );
      }
   }
}


/**
 * @brief Allocate on the explicit free list
 * 
 * @param size    Size to allocate
 * @return void*  On success, returns a pointer to begining of allocated memory
 *                On error, returns a null pointer
 */
void* do_malloc_big( size_t size )
{
   if ( size == 0 )
      return NULL;

   size_t const block_size = BLOCK_SIZE( size );

   void* bp = find_block( block_size );

   if ( bp == NULL )
   {
      add_explicit_chunk( block_size );
      bp = find_block( block_size );
      if ( bp == NULL )               // must be out of memory
         return NULL;
   }

   place_allocation( bp, block_size );

   return bp;
}


/**
 * @brief Free allocations that use explicit free lists
 * 
 * @param ptr   Pointer to free
 */
void do_free_big( void* ptr )
{
  // change our allocation status
   byte*    const bp         = ( byte* )ptr;
   size_t   const size       = GET_SIZE( HDRP( bp ) );
   uint32_t const prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );

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
 * @brief Handle reallocations of ptrs allocated using the explicit free list
 * 
 * @param ptr     Pointer to memory allocated by mm_malloc, mm_realloc or mm_calloc 
 * @param size    number of bytes to allocate
 * @return void*  On success, returns the pointer to the beginning of newly allocated memory.
 *                On error, returns a null pointer
 * 
 * TODO: fix bugs
 *         - case where new size is less than min size of big allocations
 *         - alignment issues?
 */
void* do_big_realloc( void* ptr, size_t size )
{
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

      if ( total_size - block_size >= MIN_BIG_BLOCK )  // we can split the following block
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
 * @brief The number of allocations stored in a given sized seg list 
 * 
 * @param size   The maximum size stored on the seg list
 * @return int   The maximum number allocations the seg list can hold
 */
inline int seg_list_capacity( uint32_t size )
{
   return ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / size;
}


/**
 * @brief The minimum allocation size stored in a given size seg list
 * 
 * @param size       The maximum size held in the seg list
 * @return uint32_t  The minimum size held in the seg list
 */
inline uint32_t seg_list_min_size( uint32_t size )
{
   switch ( size )
   {
   case 16:
      return 1u;
   
   case 32:
      return 17u;

   case 48:
      return 33u;

   case 64:
      return 49u;

   case 128:
      return 65u;
      
   default:
      return 129u;
   }
}


/**
 * @brief Print header information for a given seg list
 * 
 * @param ptr Pointer to start of seg list
 */
void print_seglist_headers( void* ptr )
{
   if ( ptr == NULL )
      return;

   seg_list_header_t const* pheader = ptr;

   while ( pheader )
   {
      printf( "(%p)  |  Size: %3" PRIu32 " - %-3" PRIu32"  |  Next: %-18p  |  Capacity: %-d\n", 
               ptr, pheader->min, pheader->size, pheader->next, seg_list_capacity( pheader->size ) );
      printf( "Status: [0x%016" PRIx64 ":0x%016" PRIx64 ":0x%016" PRIx64 ":0x%016" PRIx64 "]\n", 
               pheader->vector[3], pheader->vector[2], pheader->vector[1], pheader->vector[0] );
      pheader = pheader->next;
   }
}


/**
 * @brief Add a new chunk of memory to the explicit list
 * 
 * @param size Size of requested allocation
 */
static void add_explicit_chunk( size_t size )
{
   static size_t const overhead = 2 * ALIGNMENT;

   size_t const chunk_size = CHUNK_SIZE( size );
   size_t const free_size  = chunk_size - overhead;

   void* chunk;
   if ( (intptr_t )( chunk = mem_sbrk( chunk_size ) ) == -1 )
   {
      return;
   }

   void* const free_bp    = chunk + 2 * ALIGNMENT;
   void* const next_chunk = explicit_chunk;

   explicit_chunk = chunk;

   PUT_PTR( chunk, next_chunk );                         // pointer to start of next chunk of explicit free list memory; used to validate heap
   PUT( chunk + DSIZE, chunk_size );                     // chunk size
   PUT( chunk + 12, 0 );                                 // padding
   PUT( chunk + 16, 0 );                                 // padding
   PUT( chunk + 20, PACK( 8, 1, 1 ) );                   // prologue header
   PUT( chunk + 24, PACK( 8, 1, 1 ) );                   // prologue footer
   PUT( HDRP( free_bp ), PACK( free_size, 1, 0 ) );      // free block header
   PUT( FTRP( free_bp ), PACK( free_size, 1, 0 ) );      // free block footer
   PUT( chunk + chunk_size - WSIZE, PACK( 0, 1, 1 ) );   // epilogue
   
   free_list_insert( free_bp );
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
   size_t   const bp_size    = GET_SIZE( HDRP( bp ) );
   uint32_t const prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );
   uint32_t const next_alloc = GET_ALLOC( HDRP( bp + bp_size ) );

   if ( prev_alloc && next_alloc )                       // Case 1
   {
      return bp;
   }

   if ( prev_alloc && !next_alloc )                      // Case 2
   {
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
   byte* const old_start = free_list_big;
   byte* const new_bp    = bp;

   free_list_big = new_bp;

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
      free_list_big = fl_next_bp;
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
   byte* bp = free_list_big;

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
   size_t   const block_size = GET_SIZE( HDRP( bp ) );
   uint32_t const prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );
   byte*    const next_bp    = NEXT_BLKP( bp );

   if ( block_size - size >= MIN_BIG_BLOCK )    // split block
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
 * @brief Check explicit heap and explicit free list for consistency
 * 
 * @param verbose Print extra information
 */
static void heapcheck( int verbose )
{
   if ( explicit_chunk == NULL )
      return;

   byte* chunk = explicit_chunk;

   while( chunk )
   {
      check_chunk( chunk, verbose );
      chunk = ( byte* )GET_PTR( chunk );
   }

   free_list_check( verbose );
}

/**
 * @brief Check the chunk for consistency
 * 
 * @param chunk 
 * @param verbose 
 */
static void check_chunk( byte* chunk, int verbose )
{
   size_t const size = GET( chunk + DSIZE );
   size_t total_size = 0ul;

   if ( verbose )
      printf( "%p : Chunk Size: %zu \n", chunk, size );
   
   byte* bp = chunk + 24;

   // check prologue
   check_prologue( bp, verbose );
   total_size += 28;
   
   size_t block_size;
   for ( bp = chunk + 32; ( block_size = GET_SIZE( HDRP( bp ) ) ) > 0; bp = NEXT_BLKP( bp ) )
   {
      total_size += block_size;
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

   total_size += WSIZE;

   if ( total_size != size )
   {
      printf( "Error: Declared chunk size of %zu not equal to actual size of %zu\n", size, total_size );
   }
}


/**
 * @brief Check Explicit lists prologues for consistency
 * 
 * @param bp     Pointer to prologue block payload pointer
 * @param verbose 
 */
static void check_prologue( byte* bp, int verbose )
{
   if ( GET( HDRP( bp ) ) != GET( FTRP( bp ) ) )
      puts( "Error: Bad prologue" );
     
   if ( verbose )
   {
      size_t   const h_size       = GET_SIZE( HDRP( bp ) );
      size_t   const f_size       = GET_SIZE( FTRP( bp ) );
      uint32_t const h_prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );
      uint32_t const f_prev_alloc = GET_PREV_ALLOC( FTRP( bp ) );
      uint32_t const h_alloc      = GET_ALLOC( HDRP( bp ) );
      uint32_t const f_alloc      = GET_ALLOC( FTRP( bp ) );

      printf( "%p : Prologue: header: [%zu:%c%c] | footer: [%zu:%c%c]\n", bp, 
         h_size, ( h_prev_alloc ? 'a' : 'f' ), ( h_alloc ? 'a' : 'f' ),
         f_size, ( f_prev_alloc ? 'a' : 'f' ), ( f_alloc ? 'a' : 'f' ) 
      );
   }
}

/**
 * @brief Consistency check of free_list
 * 
 * @param verbose Print verbose output
 */
static void free_list_check( int verbose )
{
   byte const* bp   = free_list_big;
   byte const* prev = NULL;

   if ( verbose )
      puts( "Explicit Free List:" );

   while ( bp )
   {
      byte const* const next_bp = GET_NEXT_PTR( bp );
      byte const* const prev_bp = GET_PREV_PTR( bp );

      if ( verbose )
      {
         printf( "\t%p | prev: %-14p | next: %p\n", bp, prev_bp, next_bp );
      }

      if ( prev != prev_bp )
      {
         puts( "Error: Bad free list pointers" );
      }

      prev = bp;
      bp   = next_bp;
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
   // address of bp should be aligned
   if( ( (uintptr_t)bp % ALIGNMENT ) != 0 )
   {
      printf( "Error: %p is not %d byte aligned\n", bp, ALIGNMENT );
   }

   size_t const h_size = GET_SIZE( HDRP( bp ) );

   if ( h_size < MIN_BIG_BLOCK )
   {
      printf( "Error: Block size (%zu) is less than the minimum block size (%d)", h_size, MIN_BIG_BLOCK );
   }

   if ( h_size % ALIGNMENT != 0 )
   {
      printf( "Error: Block size (%zu) is not %d byte aligned\n", h_size, ALIGNMENT );
   }

   uint32_t const is_allocated = GET_ALLOC( HDRP( bp ) );

   if ( !is_allocated && GET( HDRP( bp ) ) != GET( FTRP( bp ) ) )
   {
      printf( "Error: header does not match footer\n" );
   }

   if ( !GET_PREV_ALLOC( HDRP( bp ) ) )
   {
      byte const* const prev_bp = PREV_BLKP( bp );

      if ( GET_ALLOC( prev_bp ) )
      {
         printf( "Error: Previous block is allocated when current block's header indicates that it is free\n" );
      }
   }
}



/**
 * @brief Print header and footer (optional) contents of a given block
 * 
 * @param bp Block payload pointer for whose contents we will print
 */
static void printblock( void* bp )
{
   size_t   const h_size       = GET_SIZE( HDRP( bp ) );
   uint32_t const is_allocated = GET_ALLOC( HDRP( bp ) );

   if ( h_size == 0 )     // epilogue
   {
      printf( "%p : Epilogue: [%zu:%c%c]\n", bp, 
         h_size, ( GET_PREV_ALLOC( HDRP( bp ) ) ? 'a' : 'f' ), ( is_allocated ? 'a' : 'f' ) );
      return;
   }

   if ( is_allocated )
   {
      uint32_t const h_alloc      = GET_ALLOC( HDRP( bp ) );
      uint32_t const h_prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );

      printf( "%p : header: [%zu:%c%c]\n", bp, 
               h_size, ( h_prev_alloc ? 'a' : 'f' ), ( h_alloc ? 'a' : 'f' ) );
   }
   else   // free block
   {
      size_t   const f_size       = GET_SIZE( FTRP( bp ) );
      uint32_t const h_prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );
      uint32_t const f_prev_alloc = GET_PREV_ALLOC( FTRP( bp ) );
      uint32_t const h_alloc      = is_allocated;
      uint32_t const f_alloc      = GET_ALLOC( FTRP( bp ) );
      
      byte const* const next_ptr = GET_NEXT_PTR( bp );
      byte const* const prev_ptr = GET_PREV_PTR( bp );

      printf( "%p : header: [%zu:%c%c] | next: %p | prev: %p | footer: [%zu:%c%c]\n", bp, 
               h_size, ( h_prev_alloc ? 'a' : 'f' ), ( h_alloc ? 'a' : 'f' ),
               next_ptr, prev_ptr, 
               f_size, ( f_prev_alloc ? 'a' : 'f' ), ( f_alloc ? 'a' : 'f' )
            );
   }
}