/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Header file for fetch stage

	@note Changes from HL5
		- Implements the logic only for the fetch part from fedec.hpp.

		- Use of HLSLibs connections for communication with the rest of the processor.

		- Increment program counter based on new stall functionality.


*/

#ifndef __FETCH__H
#define __FETCH__H

#include <systemc.h>
#include <bitset>
#include <mc_connections.h>

#include "defines.hpp"
#include "globals.hpp"
#include "drim4hls_datatypes.hpp"

SC_MODULE(fetch)
{
public:
	// Channel ports
	Connections::In< fe_in_t > fetch_din;
	Connections::In< imem_out_t > imem_dout;
	Connections::Out< imem_in_t > imem_din;
	Connections::Out< fe_out_t > dout;
	Connections::Out< imem_out_t > imem_de;

	// Entry point
	sc_in < unsigned > entry_point;

	// Clock and reset signals
	sc_in_clk clk;
	sc_in< bool > rst;

	// Trap signals. TODO: not used. Left for future implementations.
	sc_signal< bool > trap; //sc_out
	sc_signal< sc_uint<LOG2_NUM_CAUSES> > trap_cause; //sc_out

	// Thread prototype
	void fetch_th(void);

	SC_HAS_PROCESS(fetch);
	fetch(sc_module_name name)
		: imem_din("imem_din")
		, fetch_din("fetch_din")
		, dout("dout")
		, imem_dout("imem_dout")
		, imem_de("imem_de")
		, entry_point("entry_point")
		, clk("clk")
		, rst("rst")
	{
		SC_CTHREAD(fetch_th, clk.pos());
		async_reset_signal_is(rst, false);

	}

	// *** Internal variables
	sc_uint<PC_LEN>   pc;           // Init. to -4, then before first insn fetch it will be updated to 0.	 
	sc_uint<PC_LEN>   imem_pc;		// Used in fetching from instruction memory

	// Custom datatypes used for retrieving and sending data through the channels
	imem_in_t		  imem_in;		// Contains data for fetching from the instruction memory
	fe_out_t		  fe_out;		// Contains data for the decode stage
	fe_in_t			  fetch_in;		// Contains data from the decode stage used in incrementing the PC
	imem_out_t 		  imem_out;
	
	bool redirect;
	sc_uint<PC_LEN>   redirect_addr;

	bool freeze;
};

#endif
