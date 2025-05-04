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
#define CHUNKSIZE                    ( 1u << 12 )   // Extend heap by this amount (bytes)
#define ALIGNMENT                    16             // Align on 16 byte boundaries
#define MIN_BIG_BLOCK                592            // Minimum block size of allocation on explicit free list

#define SEG16_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 16u  )
#define SEG32_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 32u  )
#define SEG48_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 48u  )
#define SEG64_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 64u  )
#define SEG128_ENTRIES               ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 128u )
#define SEG269_ENTRIES               ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 269u )
#define SEG578_ENTRIES               ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 578u )


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



// =====================================
// Private Global Variables
// =====================================

static byte* free_list_16  = NULL;               // sizes 1 to 16
static byte* free_list_32  = NULL;               // sizes 17 to 32
static byte* free_list_48  = NULL;               // sizes 33 to 48
static byte* free_list_64  = NULL;               // sizes 49 to 64
static byte* free_list_128 = NULL;               // sizes 65 to 128
static byte* free_list_269 = NULL;               // sizes 129 to 269
static byte* free_list_578 = NULL;               // sizes 257 to 578
static byte* free_list_big = NULL;               // sizes > 578      - implemented as explicit free list


// =====================================
// Private Function Prototypes
// =====================================

void* create_new_seglist( byte** free_list, int size );
void  init_seglist_header( void* ptr, int size );
void  insert_new_seglist( byte** free_list, void* entry );
int   find_free_offset( bitvector* bv, int num_entries );

seg_list_header_t* get_seg_list_header( void* ptr );

void* do_malloc_16();
void* do_malloc_32();
void* do_malloc_48();
void* do_malloc_64();
void* do_malloc_128();
void* do_malloc_269();
void* do_malloc_578();
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
   assert( sizeof( seg_list_header_t) == 48u );

   free_list_16  = NULL;
   free_list_32  = NULL;
   free_list_48  = NULL;
   free_list_64  = NULL;
   free_list_128 = NULL;
   free_list_578 = NULL;
   free_list_big = NULL;

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
      return do_malloc_16();
   }
   else if( size <= 32 )
   {
      return do_malloc_32();
   }
   else if ( size <= 48 )
   {
      return do_malloc_48();
   }
   else if( size <= 64 )
   {
      return do_malloc_64();
   }
   else if( size <= 128 )
   {
      return do_malloc_128();
   }
   else if( size <= 269 )
   {
      return do_malloc_269();
   }
   else if( size <= 578 )
   {
      return do_malloc_578();
   }
   else   // size > 578
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
      print_seglist_headers( free_list_269 );
      print_seglist_headers( free_list_578 );
  }
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
void* create_new_seglist( byte** free_list, int size )
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
void init_seglist_header( void* ptr, int size )
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
int find_free_offset( bitvector* bv, int num_entries )
{
   for ( int v = 0; v < 4; ++v )
   {
      uint64_t mask = 1;

      for ( int bit = 0; bit < 64; ++bit )
      {
         if ( 64 * v + bit >= num_entries )
            return -1;

         int const is_free = !( ( *bv )[v] & mask );

         if ( is_free )
         {
            SET_BIT( ( *bv )[v], bit );
            return v * 64 + bit;
         }
         mask = ( mask << 1 );
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
   byte* const seg_lists[7] = { free_list_16, free_list_32, free_list_48, free_list_64, free_list_128, free_list_269, free_list_578 };
 
   for ( unsigned idx = 0u; idx < sizeof( seg_lists ); ++idx )
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


/**
 * @brief Handle allocations of size 16 bytes and less
 * 
 * @param size    Requested size
 * @return void*  Pointer to allocated memory
 */
void* do_malloc_16()
{
   if ( free_list_16 == NULL )
   {
      if ( create_new_seglist( &free_list_16, 16 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block16 = free_list_16;

   while ( 1 )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )block16;
      int                const offset  = find_free_offset( &pheader->vector, SEG16_ENTRIES );  

      if ( offset == -1 )
      {
         if ( pheader->next )
         {
            block16 = ( byte* )pheader->next;
         }
         else
         {
            if ( ( block16 = create_new_seglist( &free_list_16, 16 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block16 + offset * 16 + sizeof( seg_list_header_t );
      }
   }
}


void* do_malloc_32()
{
   if ( free_list_32 == NULL )
   {
      if ( create_new_seglist( &free_list_32, 32 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block32 = free_list_32;

   while ( 1 )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )block32;
      int                const offset  = find_free_offset( &pheader->vector, SEG32_ENTRIES );  

      if ( offset == -1 )
      {
         if ( pheader->next )
         {
            block32 = ( byte* )pheader->next;
         }
         else
         {
            if ( ( block32 = create_new_seglist( &free_list_32, 32 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block32 + offset * 32 + sizeof( seg_list_header_t );
      }
   }
}


void* do_malloc_48()
{
   if ( free_list_48 == NULL )
   {
      if ( create_new_seglist( &free_list_48, 48 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block48 = free_list_48;

   while ( 1 )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )block48;
      int                const offset  = find_free_offset( &pheader->vector, SEG48_ENTRIES );  

      if ( offset == -1 )
      {
         if ( pheader->next )
         {
            block48 = ( byte* )pheader->next;
         }
         else
         {
            if ( ( block48 = create_new_seglist( &free_list_48, 48 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block48 + offset * 48 + sizeof( seg_list_header_t );
      }
   }
}


void* do_malloc_64()
{
   if ( free_list_64 == NULL )
   {
      if ( create_new_seglist( &free_list_64, 64 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block64 = free_list_64;

   while ( 1 )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )block64;
      int                const offset  = find_free_offset( &pheader->vector, SEG64_ENTRIES );  

      if ( offset == -1 )
      {
         if ( pheader->next )
         {
            block64 = ( byte* )pheader->next;
         }
         else
         {
            if ( ( block64 = create_new_seglist( &free_list_64, 64 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block64 + offset * 64 + sizeof( seg_list_header_t );
      }
   }
}


void* do_malloc_128()
{
   if ( free_list_128 == NULL )
   {
      if ( create_new_seglist( &free_list_128, 128 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block128 = free_list_128;

   while ( 1 )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )block128;
      int                const offset  = find_free_offset( &pheader->vector, SEG128_ENTRIES );  

      if ( offset == -1 )
      {
         if ( pheader->next )
         {
            block128 = ( byte* )pheader->next;
         }
         else
         {
            if ( ( block128 = create_new_seglist( &free_list_128, 128 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block128 + offset * 128 + sizeof( seg_list_header_t );
      }
   }
}


void* do_malloc_269()
{
   if ( free_list_269 == NULL )
   {
      if ( create_new_seglist( &free_list_269, 269 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block269 = free_list_269;

   while ( 1 )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )block269;
      int                const offset  = find_free_offset( &pheader->vector, SEG269_ENTRIES );  

      if ( offset == -1 )
      {
         if ( pheader->next )
         {
            block269 = ( byte* )pheader->next;
         }
         else
         {
            if ( ( block269 = create_new_seglist( &free_list_269, 269 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block269 + offset * 269 + sizeof( seg_list_header_t );
      }
   }
}


void* do_malloc_578()
{
   if ( free_list_578 == NULL )
   {
      if ( create_new_seglist( &free_list_578, 578 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block578 = free_list_578;

   while ( 1 )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )block578;
      int                const offset  = find_free_offset( &pheader->vector, SEG578_ENTRIES );  

      if ( offset == -1 )
      {
         if ( pheader->next )
         {
            block578 = ( byte* )pheader->next;
         }
         else
         {
            if ( ( block578 = create_new_seglist( &free_list_578, 578 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block578 + offset * 578 + sizeof( seg_list_header_t );
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
      add_explicit_chunk( size );
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
 * @brief Handle reallocations of ptrs allocated using the explicit free list
 * 
 * @param ptr     Pointer to memory allocated by mm_malloc, mm_realloc or mm_calloc 
 * @param size    number of bytes to allocate
 * @return void*  On success, returns the pointer to the beginning of newly allocated memory.
 *                On error, returns a null pointer
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
   
   case 269:
      return 129u;

   case 578:
      return 270u;
      
   default:
      return 579u;
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

   seg_list_header_t* pheader = ptr;

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
   size_t const chunk_size = MAX( CHUNKSIZE, size );
   size_t const free_size  = chunk_size - ALIGNMENT;

   void* chunk;

   if ( (intptr_t )( chunk = mem_sbrk( chunk_size ) ) == -1 )
   {
      return;
   }

   void* const free_bp = chunk + ALIGNMENT;
   
   PUT( chunk, 0 );                                      // padding
   PUT( chunk + WSIZE, PACK( DSIZE, 1, 1) );             // prologue: header
   PUT( chunk + DSIZE, PACK( DSIZE, 1, 1 ) );            // prologue: footer
   PUT( HDRP( free_bp ), PACK( free_size, 1, 0 ) );      // free block header
   PUT( FTRP( free_bp ), PACK( free_size, 1, 0 ) );      // free block footer
   PUT( chunk + CHUNKSIZE - WSIZE, PACK( 0, 1, 1 ) );    // epilogue
   
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
   size_t const block_size = GET_SIZE( HDRP( bp ) );
   int    const prev_alloc = GET_PREV_ALLOC( HDRP( bp ) );
   byte*  const next_bp    = NEXT_BLKP( bp );

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