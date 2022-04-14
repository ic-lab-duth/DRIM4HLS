/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Header file for data instruction stage.

*/

#ifndef __IMEM_INT__H
#define __IMEM_INT__H

#include <systemc.h>
#include "../src/drim4hls_datatypes.hpp"

#include "../src/globals.hpp"
#include "../src/defines.hpp"

SC_MODULE(imem_interface)
{
public:
	// Declaration of clock and reset parameters
	sc_in < bool > clk;
	sc_in < bool > rst;
 
	// Instruction memory ports
	Connections::Out< imem_out_t > imem_out; 
	Connections::In< imem_in_t > imem_in;
	Connections::In< imem_in_t > stall_in;
	Connections::In< stall_t > dmem_stall_in;

	Connections::Out< stall_t > stall_fe;
	Connections::Out< stall_t > stall_de; 

	sc_uint<XLEN> *imem;

	imem_out_t imem_dout;
	imem_in_t imem_din;
	imem_in_t stall_din;
	stall_t dmem_stall_din;

	stall_t stall_fe_d;
	stall_t stall_de_d;
	
	void fetch_instr();

	SC_HAS_PROCESS(imem_interface);
	imem_interface(sc_module_name name, sc_uint<XLEN> imem[ICACHE_SIZE])
		: clk("clk")
		, rst("rst")
		, imem_out("imem_out")
		, imem_in("imem_in")
		, stall_in("stall_in")
		, imem(imem)
	{
		SC_CTHREAD(fetch_instr, clk.pos());
		reset_signal_is(rst, 0);
	}

	bool dmem_stall;
	bool valid;

};

#endif
