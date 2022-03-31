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

#include "execute.hpp"
#include "memwb.hpp"
#include "fetch.hpp"
#include "decode.hpp"

// #include "execute.cpp"
// #include "memwb.cpp"
// #include "fetch.cpp"
// #include "decode.cpp"

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
	sc_out < long int > pre_b_icount;

	sc_out < bool > data_valid;
	sc_out < bool > read_en; 
	sc_out < bool > write_en;  
	sc_out < unsigned int > data_addr;
	sc_out < sc_uint<XLEN> > data_in; 
	sc_in < sc_uint<XLEN> > data_out; 

	// Inter-stage Channels and ports.
	Connections::Combinational< fe_out_t > fe2de_ch;
	Connections::Combinational< de_out_t > de2exe_ch;
	Connections::Combinational< fe_in_t > de2fe_ch;
	Connections::Combinational< mem_out_t > wb2de_ch; // Writeback loop
	Connections::Combinational< exe_out_t > exe2mem_ch;

	Connections::In< imem_out_t > imem2de_data; 
	Connections::Out< imem_in_t > fe2imem_data;
	Connections::Out< imem_in_t > de2imem_data;
	
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
		, fe2de_ch("fe2de_ch")
		, de2exe_ch("de2exe_ch")
		, de2fe_ch("de2fe_ch")
		, exe2mem_ch("exe2mem_ch")
		, wb2de_ch("wb2de_ch")
		, fwd_exe_ch("fwd_exe_ch")
		, imem2de_data("imem2de_data")
		, fe2imem_data("fe2imem_data")
		, de2imem_data("de2imem_data")
		, dmem2wb_data("dmem2wb_data")
		, wb2dmem_data("wb2dmem_data")
		, exe2dmem_data("exe2dmem_data")
		, fe("Fetch")
		, dec("Decode")
		, exe("Execute")
		, mewb("Memory")
	{
		// FETCH
		fe.clk(clk);
		fe.rst(rst);
		fe.dout(fe2de_ch);
		fe.fetch_din(de2fe_ch);
		fe.fetch_en(fetch_en);
		fe.entry_point(entry_point);
		
		fe.imem_din(fe2imem_data);

		// DECODE
		dec.clk(clk);
		dec.rst(rst);
		dec.fetch_en(fetch_en);
		dec.dout(de2exe_ch);
		dec.feed_from_wb(wb2de_ch);
		dec.fetch_din(fe2de_ch);
		dec.fetch_dout(de2fe_ch);
		dec.program_end(program_end);
		dec.fwd_exe(fwd_exe_ch);
		dec.icount(icount);
		dec.j_icount(j_icount);
		dec.b_icount(b_icount);
		dec.m_icount(m_icount);
		dec.o_icount(o_icount);
		dec.pre_b_icount(pre_b_icount);

		dec.imem_stall_out(de2imem_data);
		dec.imem_out(imem2de_data);

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
	//fedec fede;
	fetch fe;
	decode dec;
	execute exe;
	memwb mewb;
};

#endif  // end __HL5__H
