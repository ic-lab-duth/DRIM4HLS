/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Header file for data memory stage.
*/

#ifndef __DMEM_INT__H
#define __DMEM_INT__H

#include <systemc.h>
#include "../src/drim4hls_datatypes.hpp"

#include "../src/globals.hpp"
#include "../src/defines.hpp"

SC_MODULE(dmem_interface)
{
public:
	// Declaration of clock and reset parameters
	sc_in < bool > clk;
	sc_in < bool > rst;

	Connections::Out< dmem_out_t > dmem_out; 
	Connections::In< dmem_in_t > dmem_in_read;
	Connections::In< dmem_in_t > dmem_in_write;

	Connections::Out< stall_t > stall_fe;
	Connections::Out< stall_t > stall_de; 
	Connections::Out< stall_t > stall_exe; 
	Connections::Out< stall_t > stall_memwb;
	Connections::Out< stall_t > stall_imem;

	sc_uint<XLEN> *dmem;

	dmem_out_t dmem_dout;
	dmem_in_t dmem_din_read;
	dmem_in_t dmem_din_write;
	stall_t imem_stall_din;

	stall_t stall_fe_d;
	stall_t stall_de_d;
	stall_t stall_exe_d;
	stall_t stall_memwb_d;
	stall_t stall_dmem_d;
	
	void dmem_th();

	SC_HAS_PROCESS(dmem_interface);
	dmem_interface(sc_module_name name, sc_uint<XLEN> dmem[ICACHE_SIZE])
		: clk("clk")
		, rst("rst")
		, dmem_out("dmem_out")
		, dmem_in_read("dmem_in_read")
		, dmem_in_write("dmem_in_write")
		, dmem(dmem)
	{
		SC_CTHREAD(dmem_th, clk.pos());
		reset_signal_is(rst, 0);
	}

};

#endif
