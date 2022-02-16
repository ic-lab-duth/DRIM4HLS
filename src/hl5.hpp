/* Copyright 2017 Columbia University, SLD Group */

//
// hl5.h - Robert Margelli
// Header file of the hl5 CPU container.
// This module instantiates the stages and interconnects them.
//

#ifndef __HL5__H
#define __HL5__H

#include <systemc.h>
#include <mc_connections.h>

#include "hl5_datatypes.hpp"
#include "defines.hpp"
#include "globals.hpp"

#include "fedec.hpp"
#include "execute.hpp"
#include "memwb.hpp"

// #include "fedec.cpp"
// #include "execute.cpp"
// #include "memwb.cpp"

SC_MODULE(hl5)
{
public:
	// Declaration of clock and reset signals
	sc_in_clk      clk;
	sc_in < bool > rst;

	//End of simulation signal.
	sc_out < bool > program_end;

	// Fetch enable signal.
	sc_in < bool > fetch_en;

	// Entry point
	sc_in < unsigned > entry_point;

	// Instruction counters
	sc_out < long int > icount; 
	sc_out < long int > j_icount; 
	sc_out < long int > b_icount; 
	sc_out < long int > m_icount; 
	sc_out < long int > o_icount;

	sc_out < bool > data_valid;
	sc_out < bool > read_en; 
	sc_out < bool > write_en;  
	sc_out < unsigned int > data_addr;
	sc_out < sc_uint<XLEN> > data_in; 
	sc_in < sc_uint<XLEN> > data_out; 

	// Inter-stage Flex Channels.
	Connections::Combinational< de_out_t > de2exe_ch;
	Connections::Combinational< mem_out_t > wb2de_ch; // Writeback loop
	Connections::Combinational< exe_out_t > exe2mem_ch;

	Connections::In< imem_out_t > imem2fe_data; 
	Connections::Out< imem_in_t > fe2imem_data;
	
	Connections::In< dmem_out_t > dmem2wb_data; 
	Connections::Out< dmem_in_t > wb2dmem_data;
	Connections::Out< dmem_in_t > exe2dmem_data;
	// Forwarding
	sc_signal< reg_forward_t > fwd_exe_ch;

	SC_HAS_PROCESS(hl5);

	hl5(sc_module_name name)
		: clk("clk")
		, rst("rst")
		, program_end("program_end")
		, fetch_en("fetch_en")
		, entry_point("entry_point")
		, de2exe_ch("de2exe_ch")
		, exe2mem_ch("exe2mem_ch")
		, wb2de_ch("wb2de_ch")
		, fwd_exe_ch("fwd_exe_ch")
		, imem2fe_data("imem2fe_data")
		, fe2imem_data("fe2imem_data")
		, dmem2wb_data("dmem2wb_data")
		, wb2dmem_data("wb2dmem_data")
		, exe2dmem_data("exe2dmem_data")
		, fede("Fedec")
		, exe("Execute")
		, mewb("Memory")
	{
		// FEDEC
		fede.clk(clk);
		fede.rst(rst);
		fede.dout(de2exe_ch);
		fede.feed_from_wb(wb2de_ch);
		fede.program_end(program_end);
		fede.fetch_en(fetch_en);
		fede.entry_point(entry_point);
		fede.fwd_exe(fwd_exe_ch);
		fede.icount(icount);
		fede.j_icount(j_icount);
		fede.b_icount(b_icount);
		fede.m_icount(m_icount);
		fede.o_icount(o_icount);

		fede.imem_out(imem2fe_data);
		fede.imem_in(fe2imem_data);

		// EXE
		exe.clk(clk);
		exe.rst(rst);
		exe.din(de2exe_ch);
		exe.dout(exe2mem_ch);
		exe.fwd_exe(fwd_exe_ch);

		exe.dmem_in(exe2dmem_data);

		// MEM
		mewb.clk(clk);
		mewb.rst(rst);
		mewb.din(exe2mem_ch);
		mewb.dout(wb2de_ch);
		mewb.fetch_en(fetch_en);
		
		mewb.dmem_out(dmem2wb_data);
		mewb.dmem_in(wb2dmem_data);
	}

	// Instantiate the modules
	fedec fede;
	execute exe;
	memwb mewb;
};

#endif  // end __HL5__H
