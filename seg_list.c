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

#include <stdlib.h>

// =====================================
// Typedefs
// =====================================

typedef unsigned char byte;
typedef uint64_t bitvector[4];

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


#define PUT_PTR( p, ptr )            ( *( uintptr_t* )( p ) = ( uintptr_t )( ptr ) )  // write a pointer at address p
#define GET_PTR( p )                 ( *( uintptr_t* )( p ) )   

#define SET_BIT( word64, bit )       ( word64 |=  ( 1 << bit ) )
#define CLEAR_BIT( word64, bit )     ( word64 &= ~( 1 << bit ) )


// =====================================
// Private Global Variables
// =====================================

static byte* free_list_16  = NULL;               // sizes 1 to 16
static byte* free_list_32  = NULL;               // sizes 17 to 32
static byte* free_list_64  = NULL;               // sizes 33 to 64
static byte* free_list_128 = NULL;               // sizes 65 to 128
static byte* free_list_256 = NULL;               // sizes 129 to 256
static byte* free_list_512 = NULL;               // sizes 257 to 512
static byte* free_list_big = NULL;               // sizes > 512      - implemented as explicit free list


// =====================================
// Private Function Prototypes
// =====================================

void  init_seglist_header( void* ptr );
void* create_new_seglist( byte** free_list );
void  insert_new_seglist( byte** free_list, void* entry );

int   find_free_offset( bitvector* bv, int num_entries );

void* do_malloc_16( );
void* do_malloc_32();
void* do_malloc_64();
void* do_malloc_128();
void* do_malloc_256();
void* do_malloc_512();
void* do_malloc_big( size_t size );


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
   else if( size <= 64 )
   {
      return do_malloc_64();
   }
   else if( size <= 128 )
   {
      return do_malloc_128();
   }
   else if( size <= 256 )
   {
      return do_malloc_256();
   }
   else if( size <= 512 )
   {
      return do_malloc_512();
   }
   else   // size > 512
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
   print_seglist_headers( free_list_16 );
   print_seglist_headers( free_list_32 );
   print_seglist_headers( free_list_64 );
   print_seglist_headers( free_list_128 );
   print_seglist_headers( free_list_256 );
   print_seglist_headers( free_list_512 );
  
}


// ================================================================================================
//                                         Private
// ================================================================================================



/**
 * @brief Given a pointer to a 4K block, initialize the seg list
 * 
 * @param ptr Pointer to 4K block for seg list
 */
void init_seglist_header( void* ptr )
{
   PUT_PTR( ptr, NULL );

   bitvector* bv = ( bitvector* )( ptr + DSIZE );
   ( *bv )[0] = 0;
   ( *bv )[1] = 0;
   ( *bv )[2] = 0;
   ( *bv )[3] = 0;
}


/**
 * @brief Create a new seglist object
 * 
 * @param free_list 
 * @return void*      On success, pointer to newly allocated 4K block initialized as a seg list
 *                    On error, NULL
 */
void* create_new_seglist( byte** free_list )
{
   byte* old_brk;  
   
   if ( ( intptr_t )( old_brk = mem_sbrk( CHUNKSIZE ) ) == -1 )
   {
      return NULL;
   }

   init_seglist_header( old_brk );
   insert_new_seglist( free_list, old_brk );

   return old_brk;
}


/**
 * @brief Insert new seg list block into start of free list
 * 
 * @param free_list The free list to insert into
 * @param entry     The new seg list to insert
 */
void insert_new_seglist( byte** free_list, void* entry )
{
   byte* const seg_list = *free_list;
   *free_list = entry;
   PUT_PTR( entry, seg_list );
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
      if ( create_new_seglist( &free_list_16 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block16 = free_list_16;

   while ( 1 )
   {
      bitvector* const bv     = ( bitvector* )( block16 + DSIZE );
      int        const offset = find_free_offset( bv, 253 );

      if ( offset == -1 )
      {
         byte* next = ( byte* )GET_PTR( block16 );

         if ( next )
         {
            block16 = next;
         }
         else
         {
            if ( ( block16 = create_new_seglist( &free_list_16 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block16 + offset * 16 + 48;
      }
   }
}

void* do_malloc_32()
{
   if ( free_list_32 == NULL )
   {
      if ( create_new_seglist( &free_list_32 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block32 = free_list_32;

   while ( 1 )
   {
      bitvector* const bv     = ( bitvector* )( block32 + DSIZE );
      int        const offset = find_free_offset( bv, 126 );

      if ( offset == -1 )
      {
         byte* next = ( byte* )GET_PTR( block32 );

         if ( next )
         {
            block32 = next;
         }
         else
         {
            if ( ( block32 = create_new_seglist( &free_list_32 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block32 + offset * 32 + 48;
      }
   }
}


void* do_malloc_64()
{
   if ( free_list_64 == NULL )
   {
      if ( create_new_seglist( &free_list_64 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block64 = free_list_64;

   while ( 1 )
   {
      bitvector* const bv     = ( bitvector* )( block64 + DSIZE );
      int        const offset = find_free_offset( bv, 63 );

      if ( offset == -1 )
      {
         byte* next = ( byte* )GET_PTR( block64 );

         if ( next )
         {
            block64 = next;
         }
         else
         {
            if ( ( block64 = create_new_seglist( &free_list_64 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block64 + offset * 64 + 48;
      }
   }
}


void* do_malloc_128()
{
   if ( free_list_128 == NULL )
   {
      if ( create_new_seglist( &free_list_128 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block128 = free_list_128;

   while ( 1 )
   {
      bitvector* const bv     = ( bitvector* )( block128 + DSIZE );
      int        const offset = find_free_offset( bv, 31 );

      if ( offset == -1 )
      {
         byte* next = ( byte* )GET_PTR( block128 );

         if ( next )
         {
            block128 = next;
         }
         else
         {
            if ( ( block128 = create_new_seglist( &free_list_128 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block128 + offset * 128 + 48;
      }
   }
}


void* do_malloc_256()
{
   if ( free_list_256 == NULL )
   {
      if ( create_new_seglist( &free_list_256 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block256 = free_list_256;

   while ( 1 )
   {
      bitvector* const bv     = ( bitvector* )( block256 + DSIZE );
      int        const offset = find_free_offset( bv, 15 );

      if ( offset == -1 )
      {
         byte* next = ( byte* )GET_PTR( block256 );

         if ( next )
         {
            block256 = next;
         }
         else
         {
            if ( ( block256 = create_new_seglist( &free_list_256 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block256 + offset * 256 + 48;
      }
   }
}


void* do_malloc_512()
{
   if ( free_list_512 == NULL )
   {
      if ( create_new_seglist( &free_list_512 ) == NULL )
      {
         return NULL;   // can't allocate
      }
   }

   byte* block512 = free_list_512;

   while ( 1 )
   {
      bitvector* const bv     = ( bitvector* )( block512 + DSIZE );
      int        const offset = find_free_offset( bv, 7 );

      if ( offset == -1 )
      {
         byte* next = ( byte* )GET_PTR( block512 );

         if ( next )
         {
            block512 = next;
         }
         else
         {
            if ( ( block512 = create_new_seglist( &free_list_512 ) ) == NULL  )
               return NULL;
         }
      }
      else
      {
         return block512 + offset * 512 + 48;
      }
   }
}


void* do_malloc_big( size_t size )
{
   return NULL;
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

   uintptr_t const next = GET_PTR( ptr );

   printf( "(%p) Next: 0x%lx\n", ptr, next );

   bitvector* const bv = ( bitvector* )( ptr + DSIZE );

   printf( "[0x%016lx:0x%016lx:0x%016lx:0x%016lx]\n", (*bv)[0], (*bv)[1], (*bv)[2], (*bv)[3] );

   byte* nextp = (byte*)next;
   if ( nextp )
   {
      print_seglist_headers( nextp );
   }
}