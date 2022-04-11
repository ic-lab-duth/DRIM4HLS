/* Copyright 2017 Columbia University, SLD Group */

// memwb.h - Robert Margelli
// memory+writeback stage header file.

#ifndef __MEMWB__H
#define __MEMWB__H

#include <systemc.h>

#include <mc_connections.h>
#include "defines.hpp"
#include "globals.hpp"
#include "hl5_datatypes.hpp"


SC_MODULE(memwb)
{
	// FlexChannel initiators
	Connections::In< exe_out_t > din;
	Connections::Out< mem_out_t > dout;

	Connections::Out< dmem_in_t > dmem_in;
	Connections::In< dmem_out_t > dmem_out;

	Connections::In< stall_t > dmem_stall;

	// Clock and reset signals
	sc_in_clk clk;
	sc_in<bool> rst;

	// Enable fetch
	sc_in<bool> fetch_en;   // Used to synchronize writeback with fetch at reset.

	// Thread prototype
	void memwb_th(void);
	// Function prototypes.
	sc_bv<XLEN> ext_sign_byte(sc_bv<BYTE> read_data);           // Sign extend byte read from memory. For LB
	sc_bv<XLEN> ext_unsign_byte(sc_bv<BYTE> read_data);         // Zero extend byte read from memory. For LBU
	sc_bv<XLEN> ext_sign_halfword(sc_bv<BYTE*2> read_data);     // Sign extend half-word read from memory. For LH
	sc_bv<XLEN> ext_unsign_halfword(sc_bv<BYTE*2> read_data);   // Zero extend half-word read from memory. For LHU

	// Constructor
	SC_HAS_PROCESS(memwb);
	memwb(sc_module_name name)
		: din("din")
		, dout("dout")
		, dmem_stall("dmem_stall")
		, clk("clk")
		, rst("rst")
		, fetch_en("fetch_en")
	{
		SC_CTHREAD(memwb_th, clk.pos());
		reset_signal_is(rst, false);
	}

	// Member variables
	exe_out_t input;
	exe_out_t input1;
	exe_out_t input2;
	exe_out_t temp_input;
	//exe_out_t buffer[2];
	
	mem_out_t output;
	sc_bv<DATA_SIZE> mem_dout;  // Temporarily stores the value read from memory with a load instruction

	dmem_in_t		  dmem_dout;
	dmem_out_t		  dmem_din;

	stall_t dmem_stall_d;
	bool dmem_freeze;
	bool dmem_data_valid;

};


#endif
