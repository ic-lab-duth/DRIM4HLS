/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Header file for decode stage

	@note Changes from HL5
		- Implements the logic only for the decode part from fedec.hpp.

		- Use of HLSLibs connections for communication with the rest of the processor.

		- Stall mechanism manages data dependencies, dynamic load/write memory stalls
		  and change of program direction.


*/

#ifndef __DEC__H
#define __DEC__H

#include <systemc.h>
#include <bitset>
#include <mc_connections.h>

#include "defines.hpp"
#include "globals.hpp"
#include "drim4hls_datatypes.hpp"

SC_MODULE(decode)
{
public:
	// FlexChannel initiators
	Connections::Out< de_out_t > dout;
    Connections::Out< fe_in_t > fetch_dout;

	Connections::In< mem_out_t > feed_from_wb;
	Connections::In< imem_out_t > imem_out;
    Connections::In< fe_out_t > fetch_din;
	Connections::In< reg_forward_t > fwd_exe;
	// End of simulation signal.
	sc_out < bool > program_end;

	// Clock and reset signals
	sc_in_clk clk;
	sc_in< bool > rst;

	// Instruction counters
	sc_out < long int > icount; 
	sc_out < long int > j_icount; 
	sc_out < long int > b_icount; 
	sc_out < long int > m_icount; 
	sc_out < long int > o_icount;

	// Thread prototype
	void decode_th(void);

	// Function prototypes.
	sc_bv<PC_LEN> sign_extend_jump(sc_bv<21> imm);
	sc_bv<PC_LEN> sign_extend_branch(sc_bv<13> imm);

	SC_HAS_PROCESS(decode);
	decode(sc_module_name name)
    	: clk("clk")
		, rst("rst")
		, dout("dout")
		, feed_from_wb("feed_from_wb")
        , fetch_din("fetch_din")
        , fetch_dout("fetch_dout")
        , program_end("program_end")
		, fwd_exe("fwd_exe")
        , icount("icount")
		, j_icount("j_icount")
		, b_icount("b_icount")
		, m_icount("m_icount")
		, o_icount("o_icount")
		, imem_out("imem_out")
	{
		SC_CTHREAD(decode_th, clk.pos());
		async_reset_signal_is(rst, false);

	}

	// Member variables (DECODE)
	de_in_t           self_feed;    	// Contains branch and jump data		 
	imem_out_t		  imem_din;			// Contains data from instruction memory
	mem_out_t  	      feedinput;		// Contains data from writeback stage
	de_out_t          output;			// Contains data for the execute stage
    fe_out_t          input;			// Contains data from the fetch stage
    fe_in_t           fetch_out;		// Contains data for the fetch stage about processor stalls

	reg_forward_t fwd;
	reg_forward_t temp_fwd;  
	
	sc_bv<INSN_LEN>   insn;         	// Contains full instruction fetched from IMEM. Used in decoding.
	unsigned int      imem_data;		// Contains instruction data

	sc_uint< PC_LEN > pc;				// Contains PC for the current instruction that is decoded
	sc_uint< PC_LEN > next_pc;			// Contains PC for the next instruction that will be decoded

    fe_out_t buffer_fe_out[2];			// Buffer for the data coming from the fetch stage
	imem_out_t buffer_imem_out[2];
    
    // Trap signals. TODO: not used. Left for future implementations.
	sc_signal< bool > trap; //sc_out
	sc_signal< sc_uint<LOG2_NUM_CAUSES> > trap_cause; //sc_out
	// NB. x0 is included in this regfile so it is not a real hardcoded 0
	// constant. The writeback section of fedec has a guard fro writes on
	// x0. For double protection, some instructions that want to write into
	// x0 will have their regwrite signal forced to false.
	sc_bv<XLEN> regfile[REG_NUM];
	// Keeps track of in-flight instructions that are going to overwrite a
	// register. Implements a primitive stall mechanism for RAW hazards.
	sc_bv<XLEN + 1> sentinel[REG_NUM];
	sc_uint<TAG_WIDTH> previous_tag;
	sc_uint<TAG_WIDTH> tag;
	// Stalls processor and sends a nop operation to the execute stage
	bool freeze;
	// Flushes current instruction in order to sychronize processor with a
	// change of direction in the execution
	bool flush;

	bool forward_success_rs1;
	bool forward_success_rs2;

	sc_uint<OPCODE_SIZE> opcode;
};

#endif
