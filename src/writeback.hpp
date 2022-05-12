/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Header file for writeback stage.

	@note Changes from HL5

		- Use of HLSLibs connections for communication with the rest of the processor.

		- Memory is outside of the processor

*/

#ifndef __WRITEBACK__H
#define __WRITEBACK__H

#include <systemc.h>

#include <mc_connections.h>
#include "defines.hpp"
#include "globals.hpp"
#include "drim4hls_datatypes.hpp"


SC_MODULE(writeback)
{
	// FlexChannel initiators
	Connections::In< exe_out_t > din;
	Connections::In< dmem_out_t > dmem_out;

	Connections::Out< mem_out_t > dout;
	Connections::Out< dmem_in_t > dmem_in;

	// Clock and reset signals
	sc_in_clk clk;
	sc_in<bool> rst;

	// Thread prototype
	void writeback_th(void);
	// Function prototypes.
	sc_bv<XLEN> ext_sign_byte(sc_bv<BYTE> read_data);           // Sign extend byte read from memory. For LB
	sc_bv<XLEN> ext_unsign_byte(sc_bv<BYTE> read_data);         // Zero extend byte read from memory. For LBU
	sc_bv<XLEN> ext_sign_halfword(sc_bv<BYTE*2> read_data);     // Sign extend half-word read from memory. For LH
	sc_bv<XLEN> ext_unsign_halfword(sc_bv<BYTE*2> read_data);   // Zero extend half-word read from memory. For LHU

	// Constructor
	SC_HAS_PROCESS(writeback);
	writeback(sc_module_name name)
		: din("din")
		, dout("dout")
		, dmem_in("dmem_in")
		, dmem_out("dmem_out")
		, clk("clk")
		, rst("rst")
	{
		SC_CTHREAD(writeback_th, clk.pos());
		async_reset_signal_is(rst, false);
	}

	// Member variables
	exe_out_t input;
	exe_out_t input1;
	exe_out_t input2;
	exe_out_t temp_input;
	
	mem_out_t output;
	sc_bv<DATA_SIZE> mem_dout;  // Temporarily stores the value read from memory with a load instruction

	dmem_in_t		  dmem_dout;
	dmem_out_t		  dmem_din;

	bool dmem_freeze;
	bool dmem_data_valid;
	
	bool freeze;
	bool loading_data;
	bool storing_data;

};


#endif
