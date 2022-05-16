#include <iostream>

#include "drim4hls_datatypes.h"

#include "defines.h"

#include "globals.h"

#include "drim4hls.h"

#include <mc_scverify.h>

class Top: public sc_module {
    public:

    CCS_DESIGN(drim4hls) CCS_INIT_S1(m_dut);

    sc_clock clk;
    SC_SIG(bool, rst);

    // End of simulation signal.
    #pragma hls_direct_input
    sc_signal < bool > CCS_INIT_S1(program_end);

    // Instruction counters
    #pragma hls_direct_input
    sc_signal < long int > CCS_INIT_S1(icount);
    #pragma hls_direct_input
    sc_signal < long int > CCS_INIT_S1(j_icount);
    #pragma hls_direct_input
    sc_signal < long int > CCS_INIT_S1(b_icount);
    #pragma hls_direct_input
    sc_signal < long int > CCS_INIT_S1(m_icount);
    #pragma hls_direct_input
    sc_signal < long int > CCS_INIT_S1(o_icount);

    /* The testbench, DUT, IMEM and DMEM modules. */
    Connections::Combinational < imem_out_t > CCS_INIT_S1(imem2de_ch);
    Connections::Combinational < imem_in_t > CCS_INIT_S1(fe2imem_ch);

    Connections::Combinational < dmem_out_t > CCS_INIT_S1(dmem2wb_ch);
    Connections::Combinational < dmem_in_t > CCS_INIT_S1(wb2dmem_ch);

    sc_uint < XLEN > imem[ICACHE_SIZE];

    imem_out_t imem_dout;
    imem_in_t imem_din;

    sc_uint < XLEN > dmem[DCACHE_SIZE];

    dmem_out_t dmem_dout;
    dmem_in_t dmem_din;

    const std::string testing_program;

    SC_CTOR(Top);
    Top(const sc_module_name & name,
        const std::string & testing_program): clk("clk", 10, SC_NS, 5, 0, SC_NS, true),
    m_dut("drim4hls"),
    testing_program(testing_program) {
        Connections::set_sim_clk( & clk);

        // Connect the design module
        m_dut.clk(clk);
        m_dut.rst(rst);
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

        SC_CTHREAD(run, clk);

        SC_THREAD(imemory_th);
        sensitive << clk.posedge_event();
        async_reset_signal_is(rst, false);

        SC_THREAD(dmemory_th);
        sensitive << clk.posedge_event();
        async_reset_signal_is(rst, false);
    }

    void imemory_th() {
        IMEM_RST: {
            imem2de_ch.ResetWrite();
            fe2imem_ch.ResetRead();

            wait();
        }
        IMEM_BODY: while (true) {
            imem_din = fe2imem_ch.Pop();

            unsigned int addr;

            addr = imem_din.instr_addr;
            imem_dout.instr_data = imem[addr];

            imem2de_ch.Push(imem_dout);
            wait();
        }

    }

    void dmemory_th() {
        DMEM_RST: {
            wb2dmem_ch.ResetRead();
            dmem2wb_ch.ResetWrite();

            wait();
        }
        DMEM_BODY: while (true) {
            dmem_din = wb2dmem_ch.Pop();
            unsigned int addr = dmem_din.data_addr;

            if (dmem_din.read_en) {

                dmem_dout.data_out = dmem[addr];

            } else if (dmem_din.write_en) {

                dmem[addr] = dmem_din.data_in;
                dmem_dout.data_out = dmem_din.data_in;

            }

            // REMOVE	
            dmem2wb_ch.Push(dmem_dout);
            wait();
        }

    }

    void run() {

        std::ifstream load_program;
        load_program.open(testing_program, std::ifstream:: in );
        unsigned index;
        unsigned address;

        while (load_program >> std::hex >> address) {

            index = address >> 2;
            if (index >= ICACHE_SIZE) {
                SC_REPORT_ERROR(sc_object::name(), "Program larger than memory size.");
                sc_stop();
                return;
            }

            load_program >> std::hex >> imem[index];
            dmem[index] = imem[index];
        }

        load_program.close();

        rst.write(0);
        wait(5);
        rst.write(1);
        wait();

        do {
            wait();
        } while (!program_end.read());
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

int sc_main(int argc, char * argv[]) {

    // if (argc == 1) {
    //     std::cerr << "Usage: " << argv[0] << " <testing_program>" << std::endl;
    //     std::cerr << "where:  <testing_program> - path to .txt file of the testing program" << std::endl;
    //     return -1;
    // }

    std::string testing_program = argv[1];
    // USE IN QUESTASIM
    //std::string testing_program = "home/dpatsidis/Desktop/HLS4DRIM-main/examples/binary_search/hello.txt";

    Top top("top", testing_program);
    sc_start();
    return 0;
}
