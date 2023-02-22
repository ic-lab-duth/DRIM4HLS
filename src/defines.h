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

// Data cache directives
#define DCACHE_WAYS 2 // Number of ways
#define DCACHE_ENTRIES 16 // Number of blocks per way
#define DCACHE_LINE 32 // Number of bits per block

// ( (int) log2( DCACHE_ENTRIES ) )
#define DCACHE_INDEX_WIDTH 4
// ( (int) log2( DCACHE_LINE / DATA_WIDTH) )
#define DCACHE_OFFSET_WIDTH 0
// ( ADDR_WIDTH - DCACHE_INDEX_WIDTH - DCACHE_OFFSET_WIDTH )
#define DCACHE_TAG_WIDTH 28

#define DCACHE_DATA_SIZE ( DCACHE_WAYS * BLOCK_WIDTH )
#define DCACHE_TAGS_SIZE ( DCACHE_WAYS * DCACHE_TAG_WIDTH + 2 * DCACHE_WAYS) // Contais the tags and the valid/dirty bits

// Instruction Cache directives
#define ICACHE_WAYS 2 // Number of ways
#define ICACHE_ENTRIES 8 // Number of blocks per way
#define ICACHE_LINE 64 // Number of bits per block

// ( (int) log2( ICACHE_ENTRIES ) )
#define ICACHE_INDEX_WIDTH 3
// ( (int) log2( ICACHE_LINE / DATA_WIDTH) )
#define ICACHE_OFFSET_WIDTH 1
// ( ADDR_WIDTH - ICACHE_INDEX_WIDTH - ICACHE_OFFSET_WIDTH )
#define ICACHE_TAG_WIDTH 28

#define ICACHE_DATA_SIZE ( ICACHE_WAYS * BLOCK_WIDTH )
#define ICACHE_TAGS_SIZE ( ICACHE_WAYS * ICACHE_TAG_WIDTH + 2 * ICACHE_WAYS) // Contais the tags and the valid/dirty bits
#define ICACHE_BUFFER_SIZE ( ICACHE_LINE / ADDR_WIDTH  + 1)

// Branch predictor directives

#define BTB_ENTRIES 32
// ( (int) log2( BTB_ENTRIES ) )
#define BTB_INDEX_WIDTH 5
// ( ADDR_WIDTH - BTB_INDEX_WIDTH )
#define BTB_TAG_WIDTH 27
#define BTB_PREDICTION_BITS_WIDTH 2 // Number of prediction bits used
// (2^BTB_PREDICTION_BITS_WIDTH / 2) - 1
#define WEAK_NON_TAKEN 1 // Branches with certainty of <= WEAK_NON_TAKEN are not taken
#define STRONG_TAKEN 3 // Maximum value for prediction bits

// RAS directives
#define RAS_ENTRIES 4
// ( (int) log2( RAS_ENTRIES ) )
#define RAS_POINTER_SIZE 2
// Dbg directives.

#define INTERNAL_PROG // When on specifies the program to execute as an array in the fetch stage (not for production).

#define VERBOSE

#endif // DEFINES_H
