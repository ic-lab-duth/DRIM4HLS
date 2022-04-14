/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief 
	Header file for the drim4hls CPU container.
	This module instantiates the stages and interconnects them.

	@note Changes from HL5

		- Use of HLSLibs connections for communication with the rest of the processor.

		- Connection with memories outside of the processor.


*/

#ifndef __DRIM4HLS__H
#define __DRIM4HLS__H

#include <systemc.h>
#include <mc_connections.h>
#include <mc_scverify.h>

#include "drim4hls_datatypes.hpp"
#include "defines.hpp"
#include "globals.hpp"

#include "execute.hpp"
#include "writeback.hpp"
#include "fetch.hpp"
#include "decode.hpp"

#pragma hls_design_top
SC_MODULE(drim4hls)
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

	Connections::In< stall_t > dmem2fe_stall; 
	Connections::In< stall_t > dmem2de_stall; 
	Connections::In< stall_t > dmem2exe_stall; 
	Connections::In< stall_t > dmem2wb_stall;

	Connections::In< stall_t > imem2fe_stall; 
	Connections::In< stall_t > imem2de_stall; 

	// Forwarding
	Connections::Combinational< reg_forward_t > fwd_exe_ch;
	SC_HAS_PROCESS(drim4hls);

	drim4hls(sc_module_name name)
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
		, dmem2fe_stall("dmem2fe_stall")
		, dmem2de_stall("dmem2de_stall")
		, dmem2exe_stall("dmem2exe_stall")
		, dmem2wb_stall("dmem2wb_stall")
		, imem2fe_stall("imem2fe_stall")
		, imem2de_stall("imem2de_stall")
		, fe("Fetch")
		, dec("Decode")
		, exe("Execute")
		, wb("Writeback")
	{
		// FETCH
		fe.clk(clk);
		fe.rst(rst);
		fe.dout(fe2de_ch);
		fe.fetch_din(de2fe_ch);
		fe.fetch_en(fetch_en);
		fe.entry_point(entry_point);
		
		fe.imem_din(fe2imem_data);
		fe.dmem_stall(dmem2fe_stall);
		fe.imem_stall(imem2fe_stall);

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
		dec.dmem_stall(dmem2de_stall);
		dec.imem_stall(imem2de_stall);

		// EXE
		exe.clk(clk);
		exe.rst(rst);
		exe.din(de2exe_ch);
		exe.dout(exe2mem_ch);
		exe.fwd_exe(fwd_exe_ch);

		exe.dmem_in(exe2dmem_data);
		exe.dmem_stall(dmem2exe_stall);

		// MEM
		wb.clk(clk);
		wb.rst(rst);
		wb.din(exe2mem_ch);
		wb.dout(wb2de_ch);
		wb.fetch_en(fetch_en);
		
		wb.dmem_out(dmem2wb_data);
		wb.dmem_in(wb2dmem_data);
		wb.dmem_stall(dmem2wb_stall);
	}

	// Instantiate the modules
	fetch fe;
	decode dec;
	execute exe;
	writeback wb;
};

#endif  // end __DRIM4HLS__H
