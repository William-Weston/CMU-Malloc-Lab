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

 