/**
 * @file    std_wrappers.h
 * @author  William Weston (wjtWeston@protonmail.com)
 * @brief   Error-Handling Wrappers for common standard C functions
 * @version 0.1
 * @date 2025-04-15
 * 
 * @copyright Copyright (c) 2025
 * 
 * @note Adapted from CSAPP
 */
#ifndef __2025_04_15_STD_WRAPPERS_H
#define __2025_04_15_STD_WRAPPERS_H

#include <stddef.h>       // size_t

void* Malloc( size_t size );

void  unix_error( char* msg );

#endif  // __2025_04_15_STD_WRAPPERS_H