/* Copyright 2017 Columbia University, SLD Group */

//
// tb.h - Robert Margelli
// Testbench header file.
//

#ifndef __TB__H
#define __TB__H

#include <time.h>
#include <systemc.h>
#include "../src/hl5_datatypes.hpp"

#include "../src/globals.hpp"
#include "../src/defines.hpp"

SC_MODULE(tb)
{
public:
	// Declaration of clock and reset parameters
	sc_in < bool > CCS_INIT_S1(clk);
	sc_in < bool > CCS_INIT_S1(rst);
	// End of simulation signal.
	sc_in < bool > CCS_INIT_S1(program_end);
	// Fetch enable signal.
	sc_out < bool > CCS_INIT_S1(fetch_en);
	// CPU Reset
	sc_out < bool > CCS_INIT_S1(cpu_rst);
	// Entry point
	sc_out < unsigned > CCS_INIT_S1(entry_point);

	// Instruction counters
	sc_in < long int > CCS_INIT_S1(icount); 
	sc_in < long int > CCS_INIT_S1(j_icount); 
	sc_in < long int > CCS_INIT_S1(b_icount); 
	sc_in < long int > CCS_INIT_S1(m_icount); 
	sc_in < long int > CCS_INIT_S1(o_icount);

	// Counter for correct branch predictions
	sc_in < long int > pre_b_icount; 

	sc_uint<XLEN> *imem;
	sc_uint<XLEN> *dmem;
	
	void source();
	void sink();

	SC_HAS_PROCESS(tb);
	tb(sc_module_name name, sc_uint<XLEN> imem[ICACHE_SIZE], sc_uint<XLEN> dmem[DCACHE_SIZE])
		: clk("clk")
		, rst("rst")
		, program_end("program_end")
		, fetch_en("fetch_en")
		, cpu_rst("cpu_rst")
		, entry_point("entry_point")
		, icount("icount")
		, j_icount("j_icount")
		, b_icount("b_icount")
		, m_icount("m_icount")
		, o_icount("o_icount")
		, pre_b_icount("pre_b_icount")
		, imem(imem)
		, dmem(dmem)
	{
		SC_CTHREAD(source, clk.pos());
		reset_signal_is(rst, 0);
		SC_CTHREAD(sink, clk.pos());
		reset_signal_is(rst, 0);
	}

    double exec_start;
	const char * const *argv = sc_argv();
};

#endif
