/* Copyright 2017 Columbia University, SLD Group */

//
// tb.h - Robert Margelli
// Testbench header file.
//

#ifndef __DMEM_INT__H
#define __DMEM_INT__H

#include <systemc.h>
#include "../src/hl5_datatypes.hpp"

#include "../src/globals.hpp"
#include "../src/defines.hpp"

SC_MODULE(dmem_interface)
{
public:
	// Declaration of clock and reset parameters
	sc_in < bool > clk;
	sc_in < bool > rst;
	// TODO: removeme
	// sc_in < bool > main_start;
	// sc_in < bool > main_end;
	Connections::Out< dmem_out_t > dmem_out; 
	Connections::In< dmem_in_t > dmem_in_read;
	Connections::In< dmem_in_t > dmem_in_write;
	// Instruction counters
	// sc_in < unsigned int > data_addr; 
	// sc_in < sc_uint<XLEN> > data_in; 
	// sc_out < sc_uint<XLEN> > data_out; 

	// sc_in < bool > valid;
	// sc_in < bool > read_en; 
	// sc_in < bool > write_en;  

	sc_uint<XLEN> *dmem;

	dmem_out_t dmem_dout;
	dmem_in_t dmem_din_read;
	dmem_in_t dmem_din_write;
	
	void dmem_th();

	SC_HAS_PROCESS(dmem_interface);
	dmem_interface(sc_module_name name, sc_uint<XLEN> dmem[ICACHE_SIZE])
		: clk("clk")
		, rst("rst")
		// , main_start("main_start")
		// , main_end("main_end")
		// , data_addr("data_addr")
		// , data_in("data_in")
		// , data_out("data_out")
		// , valid("valid")
		// , read_en("read")
		// , write_en("write")
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
