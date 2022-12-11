/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief 
    This file contains several defines. Some of which must be
    commented/uncommented correctly before running.

	@note No changes from HL5

*/

#ifndef DEFINES_H
#define DEFINES_H

// Enable/disable multiplier, divider, CSR.

#define MUL32       1 // Enable 32x32 multiplier for MUL
#define MUL64       1 // Enable 64x64 multiplier for MULH, MULHSU, MULHU
#define DIV         1 // Enable division operations DIV, DIVU
#define REM         1 // Enable remainder operations REM, REMU
#define CSR_LOGIC   1 // Enable CSR logic in exe stage.


// Cache size
#define ICACHE_SIZE 51200
#define DCACHE_SIZE 51200

#define DATA_WIDTH 32 
#define ADDR_WIDTH 32
#define TAG_WIDTH 4
#define SENTINEL_INIT (1 << (TAG_WIDTH - 1))
#define FWD_ENABLE

// Cache size
#define DCACHE_WAYS 2 // Number of ways
#define DCACHE_ENTRIES 32 // Number of blocks per way
#define BLOCK_WIDTH 64 // Number of bits per block

// ( (int) log2( DCACHE_ENTRIES ) )
#define DCACHE_INDEX_WIDTH 5
// ( (int) log2( BLOCK_WIDTH / DATA_WIDTH) )
#define DCACHE_OFFSET_WIDTH 1
// ( ADDR_WIDTH - DCACHE_INDEX_WIDTH - DCACHE_OFFSET_WIDTH )
#define DCACHE_TAG_WIDTH 26

#define DCACHE_DATA_SIZE ( DCACHE_WAYS * BLOCK_WIDTH )
#define DCACHE_TAGS_SIZE ( DCACHE_WAYS * DCACHE_TAG_WIDTH + 2 * DCACHE_WAYS) // Contais the tags and the valid/dirty bits

// Dbg directives.

#define INTERNAL_PROG // When on specifies the program to execute as an array in the fetch stage (not for production).

#define VERBOSE

#endif // DEFINES_H
