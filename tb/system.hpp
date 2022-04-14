/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief 
	Top-level model header file.
	Instantiates the CPU, TB, IMEM, DMEM.
	
	@note Changes from HL5

		- Use of HLSLibs connections for communication with the rest of the processor.

		- Connect processor with memory
*/

#ifndef SYSTEM_H_INCLUDED
#define SYSTEM_H_INCLUDED

#include <systemc.h>

#include "../src/drim4hls_datatypes.hpp"
#include "../src/defines.hpp"
#include "../src/globals.hpp"

#include "../src/drim4hls.hpp"

#include "tb.hpp"
#include "imem_interface.hpp"
#include "dmem_interface.hpp"

SC_MODULE(TOP)
{
public:
	// Clock and reset
	sc_in<bool> clk;
	sc_in<bool> rst;

	// End of simulation signal.
	sc_signal < bool > program_end;
	// Fetch enable signal.
	sc_signal < bool > fetch_en;
	// CPU Reset
	sc_signal < bool > cpu_rst;
	// Entry point
	sc_signal < unsigned > entry_point;

	// Instruction counters
	sc_signal < long int > icount; 
	sc_signal < long int > j_icount; 
	sc_signal < long int > b_icount; 
	sc_signal < long int > m_icount; 
	sc_signal < long int > o_icount;
	sc_signal < long int > pre_b_icount;

	// Cache modeled as arrays
	sc_uint<XLEN> imem[ICACHE_SIZE];
	sc_uint<XLEN> dmem[DCACHE_SIZE];

	sc_signal < sc_uint<XLEN> > instr_data; 
	sc_signal < bool > instr_valid; 
	sc_signal < unsigned int > instr_addr; 

	sc_signal < bool > data_valid;
	sc_signal < bool > read_en; 
	sc_signal < bool > write_en;  
	sc_signal < unsigned int > data_addr;
	sc_signal < sc_uint<XLEN> > data_in; 
	sc_signal < sc_uint<XLEN> > data_out; 

	/* The testbench, DUT, IMEM and DMEM modules. */
	tb   *m_tb;
	drim4hls *m_dut;

	imem_interface *m_imem;
	dmem_interface *m_dmem;

	Connections::Combinational< imem_out_t > imem2de_ch; 
	Connections::Combinational< imem_in_t > fe2imem_ch;
	Connections::Combinational< imem_in_t > de2imem_ch;

	Connections::Combinational< dmem_out_t > dmem2wb_ch;
	Connections::Combinational< dmem_in_t > exe2dmem_ch; 
	Connections::Combinational< dmem_in_t > wb2dmem_ch;

	Connections::Combinational< stall_t > dmem2fe_stall_ch;
	Connections::Combinational< stall_t > dmem2de_stall_ch; 
	Connections::Combinational< stall_t > dmem2exe_stall_ch;
	Connections::Combinational< stall_t > dmem2wb_stall_ch;
	Connections::Combinational< stall_t > dmem2imem_stall_ch;

	Connections::Combinational< stall_t > imem2fe_stall_ch;
	Connections::Combinational< stall_t > imem2de_stall_ch; 

	SC_CTOR(TOP)
		: clk("clk")
		, rst("rst")
		, program_end("program_end")
		, fetch_en("fetch_en")
		, cpu_rst("cpu_rst")
		, entry_point("entry_point")
	{
		m_tb = new tb("tb", imem, dmem);
		m_dut = new drim4hls("drim4hls");

		m_imem = new imem_interface("imem_interface", imem);
		m_dmem = new dmem_interface("dmem_interface", dmem);

		// Connect the design module
		m_dut->clk(clk);
		m_dut->rst(cpu_rst);
		m_dut->entry_point(entry_point);
		m_dut->program_end(program_end);
		m_dut->fetch_en(fetch_en);
		m_dut->icount(icount);
		m_dut->j_icount(j_icount);
		m_dut->b_icount(b_icount);
		m_dut->m_icount(m_icount);
		m_dut->o_icount(o_icount);
		m_dut->pre_b_icount(pre_b_icount);

		m_dut->imem2de_data(imem2de_ch);
		m_dut->fe2imem_data(fe2imem_ch);
		m_dut->de2imem_data(de2imem_ch);
		m_dut->dmem2wb_data(dmem2wb_ch);
		m_dut->exe2dmem_data(exe2dmem_ch);
		m_dut->wb2dmem_data(wb2dmem_ch);

		m_dut->data_valid(data_valid);
		m_dut->read_en(read_en);
		m_dut->write_en(write_en);
		m_dut->data_addr(data_addr);
		m_dut->data_in(data_in);
		m_dut->data_out(data_out);

		m_dut->dmem2fe_stall(dmem2fe_stall_ch);
		m_dut->dmem2de_stall(dmem2de_stall_ch);
		m_dut->dmem2exe_stall(dmem2exe_stall_ch);
		m_dut->dmem2wb_stall(dmem2wb_stall_ch);

		m_dut->imem2fe_stall(imem2fe_stall_ch);
		m_dut->imem2de_stall(imem2de_stall_ch);

		// Connect the testbench
		m_tb->clk(clk);
		m_tb->rst(rst);
		m_tb->cpu_rst(cpu_rst);
		m_tb->entry_point(entry_point);
		m_tb->program_end(program_end);
		m_tb->fetch_en(fetch_en);
		m_tb->icount(icount);
		m_tb->j_icount(j_icount);
		m_tb->b_icount(b_icount);
		m_tb->m_icount(m_icount);
		m_tb->o_icount(o_icount);
		m_tb->pre_b_icount(pre_b_icount);

		m_imem->clk(clk);
		m_imem->rst(cpu_rst);
		m_imem->imem_out(imem2de_ch);
		m_imem->imem_in(fe2imem_ch);
		m_imem->stall_in(de2imem_ch);
		m_imem->dmem_stall_in(dmem2imem_stall_ch);
		m_imem->stall_fe(imem2fe_stall_ch);
		m_imem->stall_de(imem2de_stall_ch);

		m_dmem->clk(clk);
		m_dmem->rst(cpu_rst);
		m_dmem->dmem_out(dmem2wb_ch);
		m_dmem->dmem_in_write(wb2dmem_ch);
		m_dmem->dmem_in_read(exe2dmem_ch);

		m_dmem->stall_fe(dmem2fe_stall_ch);
		m_dmem->stall_de(dmem2de_stall_ch);
		m_dmem->stall_exe(dmem2exe_stall_ch);
		m_dmem->stall_memwb(dmem2wb_stall_ch);
		m_dmem->stall_imem(dmem2imem_stall_ch);
	}

	~TOP()
	{
		delete m_tb;
		delete m_dut;

		delete m_imem;
		delete m_dmem;
	}

};

#endif // SYSTEM_H_INCLUDED

