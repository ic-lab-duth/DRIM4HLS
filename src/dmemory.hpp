/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Header file for data instruction stage.

*/

#ifndef __DMEMORY__H
#define __DMEMORY__H

#ifndef NDEBUG
  #include <iostream>
  #define DPRINT(msg) std::cout << msg;
#else
  #define DPRINT(msg)
#endif

#include <systemc.h>
#include "../src/drim4hls_datatypes.hpp"

#include "../src/globals.hpp"
#include "../src/defines.hpp"
#include <random>

static std::random_device rd_dmem;     // only used once to initialise (seed) engine

unsigned int random_num_dmem(int min, int max) {
    std::mt19937 rng(rd_dmem());    // random-number engine used (Mersenne-Twister in this case)
    std::uniform_int_distribution<int> uni(min, max); // guaranteed unbiased
    auto random_integer = uni(rng);
    return random_integer;
}

SC_MODULE(dmemory)
{
public:
	// Declaration of clock and reset parameters
	sc_in < bool > clk;
	sc_in < bool > rst;
 
	// Instruction memory ports
	Connections::Out< dmem_out_t > dmem_out; 
	Connections::In< dmem_in_t > dmem_in;

	sc_uint<XLEN> dmem[DCACHE_SIZE];

	dmem_out_t dmem_dout;
	dmem_in_t dmem_din;

	SC_HAS_PROCESS(dmemory);
	dmemory(sc_module_name name, const std::string &testing_program)
		: clk("clk")
		, rst("rst")
		, dmem_in("dmem_in")
		, dmem_out("dmem_out")
	{

		std::ifstream dmem_init;
		dmem_init.open(testing_program, std::ifstream::in);
		unsigned index;
		unsigned address;

		while (dmem_init >> std::hex >> address) {
			index = address >> 2;
				
			if (index >= ICACHE_SIZE) {
				SC_REPORT_ERROR(sc_object::name(), "Program larger than memory size.");
				sc_stop();
				return;
			}

			dmem_init >> std::hex >> dmem[index];
			
		}

		dmem_init.close();

		SC_CTHREAD(dmem_th, clk.pos());
		async_reset_signal_is(rst, false);
	}

private:

// Source thread
void dmem_th()
{	
DMEM_RST:
	{
	dmem_in.Reset();
	dmem_out.Reset();

	wait();
	}
DMEM_BODY:	
	while (true)
	{	
		dmem_din = dmem_in.Pop();
		unsigned int addr = dmem_din.data_addr;

		if (dmem_din.read_en) {

			dmem_dout.data_out = dmem[addr];

		}else if (dmem_din.write_en) {

			dmem[addr] = dmem_din.data_in;
			dmem_dout.data_out = dmem_din.data_in;

		}
		
		unsigned int random_stalls = random_num_dmem(1,3);
		wait(random_stalls);
		// REMOVE	
		dmem_out.Push(dmem_dout);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "PUSHED" << endl);

		#ifndef __SYNTHESIS__
			if (dmem_din.read_en) {
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "READ ADDRESS= " << addr << endl);
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "READ DATA= " << dmem_dout.data_out << endl);
			}

			if (dmem_din.write_en) {
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "WRITE ADDRESS= " << addr << endl);
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "WRITE DATA= " << dmem_din.data_in << endl);
			}
			DPRINT(endl);
		#endif
		wait();
	}
	
}

};

#endif
