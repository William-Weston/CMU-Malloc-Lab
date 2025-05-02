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
typedef uint64_t bitvector[4];

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
   byte               padding_[4];
};


// =====================================
// Constants
// =====================================

#define WSIZE                        4             // Word size (bytes)
#define DSIZE                        8             // Double word size (bytes)
#define CHUNKSIZE                    ( 1 << 12 )   // Extend heap by this amount (bytes)
#define ALIGNMENT                    16            // Align on 16 byte boundaries

#define SEG16_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 16 )
#define SEG32_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 32 )
#define SEG48_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 48 )
#define SEG64_ENTRIES                ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 64 )
#define SEG128_ENTRIES               ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 128 )
#define SEG269_ENTRIES               ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 269 )
#define SEG578_ENTRIES               ( ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / 578 )

// =====================================
// Macros
// =====================================

#define MAX( x, y )                  ( ( x ) > ( y ) ? ( x ) : ( y ) )  


#define PUT_PTR( p, ptr )            ( *( uintptr_t* )( p ) = ( uintptr_t )( ptr ) )  // write a pointer at address p
#define GET_PTR( p )                 ( *( uintptr_t* )( p ) )   

#define SET_BIT( word64, bit )       ( word64 |=  ( 1ull << bit ) )
#define CLEAR_BIT( word64, bit )     ( word64 &= ~( 1ull << bit ) )


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

void* do_malloc_16();
void* do_malloc_32();
void* do_malloc_48();
void* do_malloc_64();
void* do_malloc_128();
void* do_malloc_269();
void* do_malloc_578();
void* do_malloc_big( size_t size );

int do_free_16( void* ptr );
int do_free_32( void* ptr );
int do_free_48( void* ptr );
int do_free_64( void* ptr );
int do_free_128( void* ptr );
int do_free_269( void* ptr );
int do_free_578( void* ptr );
int do_free_big( void* ptr );

inline 
int   seg_list_capacity( int size );
void  print_seglist_headers( void* ptr );



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
   assert( sizeof( seg_list_header_t) == 48 );

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
   // to determine how to free the ptr we must determine to which free list it belongs

   if ( do_free_16( ptr ) )
      return;
   
   if ( do_free_32( ptr ) )
      return;
   
   if ( do_free_48( ptr ) )
      return;

   if ( do_free_64( ptr ) )
      return;
   
   if ( do_free_128( ptr ) )
      return;

   if ( do_free_269( ptr ) )
      return;

   if ( do_free_578( ptr ) )
      return;

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
   seg_list_header_t tmp;

   tmp.next      = NULL;
   tmp.vector[0] = 0;
   tmp.vector[1] = 0;
   tmp.vector[2] = 0;
   tmp.vector[3] = 0;
   tmp.size      = size;

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
   seg_list_header_t* header = entry;
   
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


void* do_malloc_big( size_t size )
{
   return NULL;
}


int do_free_16( void* ptr )
{
   byte* searcher  = free_list_16;
   byte* const tmp = ptr;

   while ( searcher )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )searcher;

      if ( tmp > searcher && tmp < searcher + CHUNKSIZE )
      {
         int   const offset = ( tmp - ( searcher + sizeof( seg_list_header_t ) ) ) / 16;
         int   const word64 = offset / 64;
         int   const bit    = offset % 64;
         
         CLEAR_BIT( pheader->vector[word64], bit );
         return 1;
      }
      searcher = ( byte* )pheader->next;
   }

   return 0;
}


int do_free_32( void* ptr )
{
   byte* searcher  = free_list_32;
   byte* const tmp = ptr;

   while ( searcher )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )searcher;

      if ( tmp > searcher && tmp < searcher + CHUNKSIZE )
      {
         int   const offset = ( tmp - ( searcher + sizeof( seg_list_header_t ) ) ) / 32;
         int   const word64 = offset / 64;
         int   const bit    = offset % 64;
         
         CLEAR_BIT( pheader->vector[word64], bit );
         return 1;
      }
      searcher = ( byte* )pheader->next;
   }

   return 0;
}


int do_free_48( void* ptr )
{
   byte* searcher  = free_list_48;
   byte* const tmp = ptr;

   while ( searcher )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )searcher;

      if ( tmp > searcher && tmp < searcher + CHUNKSIZE )
      {
         int   const offset = ( tmp - ( searcher + sizeof( seg_list_header_t ) ) ) / 48;
         int   const word64 = offset / 64;
         int   const bit    = offset % 64;
         
         CLEAR_BIT( pheader->vector[word64], bit );
         return 1;
      }
      searcher = ( byte* )pheader->next;
   }
   return 0;
}


int do_free_64( void* ptr )
{
   byte* searcher  = free_list_64;
   byte* const tmp = ptr;

   while ( searcher )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )searcher;

      if ( tmp > searcher && tmp < searcher + CHUNKSIZE )
      {
         int   const offset = ( tmp - ( searcher + sizeof( seg_list_header_t ) ) ) / 64;
         int   const word64 = offset / 64;
         int   const bit    = offset % 64;
         
         CLEAR_BIT( pheader->vector[word64], bit );
         return 1;
      }
      searcher = ( byte* )pheader->next;
   }
   return 0;
}


int do_free_128( void* ptr )
{
   byte* searcher  = free_list_128;
   byte* const tmp = ptr;

   while ( searcher )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )searcher;

      if ( tmp > searcher && tmp < searcher + CHUNKSIZE )
      {
         int   const offset = ( tmp - ( searcher + sizeof( seg_list_header_t ) ) ) / 128;
         int   const word64 = offset / 64;
         int   const bit    = offset % 64;
         
         CLEAR_BIT( pheader->vector[word64], bit );
         return 1;
      }
      searcher = ( byte* )pheader->next;
   }

   return 0;
}


int do_free_269( void* ptr )
{
   byte* searcher  = free_list_269;
   byte* const tmp = ptr;

   while ( searcher )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )searcher;

      if ( tmp > searcher && tmp < searcher + CHUNKSIZE )
      {
         int   const offset = ( tmp - ( searcher + sizeof( seg_list_header_t ) ) ) / 269;
         int   const word64 = offset / 64;
         int   const bit    = offset % 64;
         
         CLEAR_BIT( pheader->vector[word64], bit );
         return 1;
      }
      searcher = ( byte* )pheader->next;
   }
   return 0;
}


int do_free_578( void* ptr )
{
   byte* searcher  = free_list_578;
   byte* const tmp = ptr;

   while ( searcher )
   {
      seg_list_header_t* const pheader = ( seg_list_header_t* )searcher;

      if ( tmp > searcher && tmp < searcher + CHUNKSIZE )
      {
         int   const offset = ( tmp - ( searcher + sizeof( seg_list_header_t ) ) ) / 578;
         int   const word64 = offset / 64;
         int   const bit    = offset % 64;
         
         CLEAR_BIT( pheader->vector[word64], bit );
         return 1;
      }
      searcher = ( byte* )pheader->next;
   }
   return 0;
}


int do_free_big( void* ptr )
{
   return 0;
}


inline int seg_list_capacity( int size )
{
   return ( CHUNKSIZE - sizeof( seg_list_header_t ) ) / size;
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
      printf( "(%p)  |  Size: %-5" PRIu32 "  |  Next: %-18p  |  Capacity: %-d\n", 
               ptr, pheader->size, pheader->next, seg_list_capacity( pheader->size ) );
      printf( "Status: [0x%016" PRIx64 ":0x%016" PRIx64 ":0x%016" PRIx64 ":0x%016" PRIx64 "]\n", 
               pheader->vector[3], pheader->vector[2], pheader->vector[1], pheader->vector[0] );
      pheader = pheader->next;
   }
}