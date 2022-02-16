/* Copyright 2017 Columbia University, SLD Group */

//
// fedec.h - Robert Margelli
// fetch + decoding logic header file.
//

#ifndef __FEDEC__H
#define __FEDEC__H

#include <systemc.h>
#include <bitset>
#include <mc_connections.h>

#include "defines.hpp"
#include "globals.hpp"
#include "hl5_datatypes.hpp"

SC_MODULE(fedec)
{
public:
	// FlexChannel initiators
	Connections::Out< de_out_t > CCS_INIT_S1(dout);
	Connections::In< mem_out_t > CCS_INIT_S1(feed_from_wb);

	Connections::Out< imem_in_t > CCS_INIT_S1(imem_in);
	Connections::In< imem_out_t > CCS_INIT_S1(imem_out);

	// Forward
	sc_in< reg_forward_t > CCS_INIT_S1(fwd_exe);

	// End of simulation signal.
	sc_out < bool > CCS_INIT_S1(program_end);

	// Fetch enable signal.
	sc_in < bool > CCS_INIT_S1(fetch_en);

	// Entry point
	sc_in < unsigned > CCS_INIT_S1(entry_point);

	// Clock and reset signals
	sc_in_clk CCS_INIT_S1(clk);
	sc_in< bool > CCS_INIT_S1(rst);

	// Instruction counters
	sc_out < long int > icount; 
	sc_out < long int > j_icount; 
	sc_out < long int > b_icount; 
	sc_out < long int > m_icount; 
	sc_out < long int > o_icount; 

	// Trap signals. TODO: not used. Left for future implementations.
	sc_signal< bool > CCS_INIT_S1(trap); //sc_out
	sc_signal< sc_uint<LOG2_NUM_CAUSES> > CCS_INIT_S1(trap_cause); //sc_out

	// Thread prototype
	void fedec_th(void);

	// Function prototypes.
	sc_bv<PC_LEN> sign_extend_jump(sc_bv<21> imm);
	sc_bv<PC_LEN> sign_extend_branch(sc_bv<13> imm);

	SC_HAS_PROCESS(fedec);
	fedec(sc_module_name name)
		: dout("dout")
		, feed_from_wb("feed_from_wb")
		, fwd_exe("fwd_exe")
		, program_end("program_end")
		, fetch_en("fetch_en")
		, entry_point("entry_point")
		, j_icount("j_icount")
		, b_icount("b_icount")
		, m_icount("m_icount")
		, o_icount("o_icount")
		, clk("clk")
		, rst("rst")
		, trap("trap")
		, trap_cause("trap_cause")
		, imem_out("imem_out")
		, imem_in("imem_in")
	{
		SC_CTHREAD(fedec_th, clk.pos());
		reset_signal_is(rst, false);

	}


	sc_uint<PC_LEN>   pc;           // Init. to -4, then before first insn fetch it will be updated to 0.
	sc_bv<INSN_LEN>   insn;         // Contains full instruction fetched from IMEM. Used in decoding.
	fe_in_t           self_feed;    // Contains branch and jump data.
	unsigned int      imem_data;	// Contains instruction data		 

	imem_in_t		  imem_dout;
	imem_out_t		  imem_din;
	// Member variables (DECODE)
	mem_out_t   feedinput;
	de_out_t    output;

	sc_uint<XLEN> instr_data; 
	bool instr_valid; 
	unsigned int instr_addr;
	// NB. x0 is included in this regfile so it is not a real hardcoded 0
	// constant. The writeback section of fedec has a guard fro writes on
	// x0. For double protection, some instructions that want to write into
	// x0 will have their regwrite signal forced to false.
	sc_bv<XLEN> regfile[REG_NUM];
	// Keeps track of in-flight instructions that are going to overwrite a
	// register. Implements a primitive stall mechanism for RAW hazards.
	sc_uint<TAG_WIDTH>  sentinel[REG_NUM];
	// TODO: Used for synchronizing with the wb stage and avoid
	// 'hiccuping'. It magically works.
	sc_uint<2>  position;
	bool freeze;
	// Sychronize PC when jump or branch taken occurs
	bool sync_pc;
	sc_uint<TAG_WIDTH> tag;

	sc_uint<PC_LEN> return_address;
};

#endif
