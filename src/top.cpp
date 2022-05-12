#include <iostream>
#include <mc_scverify.h>

#include "drim4hls_datatypes.hpp"
#include "defines.hpp"
#include "globals.hpp"

#include "drim4hls.hpp"

#include "imemory.hpp"
#include "dmemory.hpp"

#pragma once
class Top: public sc_module {
public:
	
	CCS_DESIGN(drim4hls) CCS_INIT_S1(m_dut);
	imemory m_imem;
	dmemory m_dmem;
	
	sc_clock clk;
	SC_SIG(bool, rst);

    // End of simulation signal.
	sc_signal < bool > CCS_INIT_S1(program_end);
	// Entry point
	sc_signal < unsigned > CCS_INIT_S1(entry_point);

	// Instruction counters
	sc_signal < long int > CCS_INIT_S1(icount); 
	sc_signal < long int > CCS_INIT_S1(j_icount); 
	sc_signal < long int > CCS_INIT_S1(b_icount); 
	sc_signal < long int > CCS_INIT_S1(m_icount); 
	sc_signal < long int > CCS_INIT_S1(o_icount);

	/* The testbench, DUT, IMEM and DMEM modules. */
	Connections::Combinational< imem_out_t > CCS_INIT_S1(imem2de_ch); 
	Connections::Combinational< imem_in_t > CCS_INIT_S1(fe2imem_ch);

	Connections::Combinational< dmem_out_t > CCS_INIT_S1(dmem2wb_ch);
	Connections::Combinational< dmem_in_t > CCS_INIT_S1(wb2dmem_ch);
	
    SC_HAS_PROCESS(Top);
	Top(const sc_module_name& name, const std::string &testing_program): 
        clk("clk", 10, SC_NS, 0.5, 0, SC_NS, true),
        m_dut("drim4hls"),
        m_imem("imemory", testing_program),
        m_dmem("dmem_interface", testing_program) {
		
        Connections::set_sim_clk(&clk);

		// Connect the design module
		m_dut.clk(clk);
		m_dut.rst(rst);
		m_dut.entry_point(entry_point);
		m_dut.program_end(program_end);

		m_dut.icount(icount);
		m_dut.j_icount(j_icount);
		m_dut.b_icount(b_icount);
		m_dut.m_icount(m_icount);
		m_dut.o_icount(o_icount);

		m_dut.imem2de_data(imem2de_ch);
		m_dut.fe2imem_data(fe2imem_ch);
		m_dut.dmem2wb_data(dmem2wb_ch);
		m_dut.wb2dmem_data(wb2dmem_ch);

		m_imem.clk(clk);
		m_imem.rst(rst);
		m_imem.imem_out(imem2de_ch);
		m_imem.imem_in(fe2imem_ch);

		m_dmem.clk(clk);
		m_dmem.rst(rst);
		m_dmem.dmem_out(dmem2wb_ch);
		m_dmem.dmem_in(wb2dmem_ch);

		SC_CTHREAD(run, clk);
	}

	void run() {
        entry_point.write(0xfffffffc);

    	rst.write(0);
    	wait(5);
    	rst.write(1);
    	wait();

        do { wait(); } while (!program_end.read());
        sc_stop();
        long icount_end, j_icount_end, b_icount_end, m_icount_end, o_icount_end, pre_b_icount_end;
        
        icount_end = icount.read();
        j_icount_end = j_icount.read();
        b_icount_end = b_icount.read();
        m_icount_end = m_icount.read();
        o_icount_end = o_icount.read();

        SC_REPORT_INFO(sc_object::name(), "Program complete.");

        std::cout << "INSTR TOT: " << icount_end << std::endl;
        std::cout << "   JUMP  : " << j_icount_end << std::endl;
        std::cout << "   BRANCH: " << b_icount_end << std::endl;
        std::cout << "   MEM   : " << m_icount_end << std::endl;
        std::cout << "   OTHER : " << o_icount_end << std::endl;
        
  	}

};

int sc_main(int argc, char *argv[]) {

    if (argc == 1 ) {
        std::cerr << "Usage: " << argv[0] << " <testing_program>" << std::endl;
        std::cerr << "where:  <testing_program> - path to .txt file of the testing program" << std::endl;
        return -1;
    }

    std::string testing_program = argv[1];

	Top top("top", testing_program);
	sc_start();
	return 0;
}