/* Copyright 2017 Columbia University, SLD Group */

//
// tb.h - Robert Margelli
// Testbench header file.
//

#ifndef __IMEM_INT__H
#define __IMEM_INT__H

#include <systemc.h>
#include "../src/hl5_datatypes.hpp"

#include "../src/globals.hpp"
#include "../src/defines.hpp"

SC_MODULE(imem_interface)
{
public:
	// Declaration of clock and reset parameters
	sc_in < bool > clk;
	sc_in < bool > rst;
	// TODO: removeme
	// sc_in < bool > main_start;
	// sc_in < bool > main_end;

	// Instruction counters
	//sc_in < unsigned int > instr_addr; 
	//sc_in < bool > valid; 
	//sc_out < sc_uint<XLEN> > instr_data; 

	Connections::Out< imem_out_t > imem_out; 
	Connections::In< imem_in_t > imem_in;

	sc_uint<XLEN> *imem;

	imem_out_t imem_dout;
	imem_in_t imem_din;
	
	void fetch_instr();

	SC_HAS_PROCESS(imem_interface);
	imem_interface(sc_module_name name, sc_uint<XLEN> imem[ICACHE_SIZE])
		: clk("clk")
		, rst("rst")
		// , main_start("main_start")
		// , main_end("main_end")
		//, instr_addr("instr_addr")
		//, valid("valid")
		//, instr_data("instr_data")
		, imem_out("imem_out")
		, imem_in("imem_in")
		, imem(imem)
	{
		SC_CTHREAD(fetch_instr, clk.pos());
		reset_signal_is(rst, 0);
	}

};

#endif
