/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Header file for data instruction stage.

*/

#ifndef __IMEMORY__H
#define __IMEMORY__H

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

static std::random_device rd_imem;     // only used once to initialise (seed) engine

unsigned int random_num_imem(int min, int max) {
    std::mt19937 rng(rd_imem());    // random-number engine used (Mersenne-Twister in this case)
    std::uniform_int_distribution<int> uni(min, max); // guaranteed unbiased
    auto random_integer = uni(rng);
    return random_integer;
}

SC_MODULE(imemory)
{
public:
	// Declaration of clock and reset parameters
	sc_in < bool > clk;
	sc_in < bool > rst;
 
	// Instruction memory ports
	Connections::Out< imem_out_t > imem_out; 
	Connections::In< imem_in_t > imem_in;

	sc_uint<XLEN> imem[ICACHE_SIZE];

	imem_out_t imem_dout;
	imem_in_t imem_din;

	SC_HAS_PROCESS(imemory);
	imemory(sc_module_name name, const std::string &testing_program)
		: clk("clk")
		, rst("rst")
		, imem_out("imem_out")
		, imem_in("imem_in")
	{
		
		std::ifstream imem_init;
		imem_init.open(testing_program, std::ifstream::in);
		unsigned index;
		unsigned address;

		while (imem_init >> std::hex >> address) {

			index = address >> 2;
			if (index >= ICACHE_SIZE) {
				SC_REPORT_ERROR(sc_object::name(), "Program larger than memory size.");
				sc_stop();
				return;
			}

			imem_init >> std::hex >> imem[index];
			DPRINT("@" << sc_time_stamp() << "\t" << "imem[" << index << "]= " << imem[index] << endl);
		}

		imem_init.close();

		SC_CTHREAD(fetch_instr, clk.pos());
		async_reset_signal_is(rst, false);
	}

private:

// Source thread
void fetch_instr()
{	
IMEM_RST:
	{
	imem_in.Reset();
	imem_out.Reset();

	wait();
	}
IMEM_BODY:	
	while (true)
	{	imem_din = imem_in.Pop();

		unsigned int addr;
		
		addr = imem_din.instr_addr;
		imem_dout.instr_data = imem[addr];

		unsigned int random_stalls = random_num_imem(1,3);
		wait(random_stalls);

		imem_out.Push(imem_dout);
		
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "instr_data= " << imem_dout.instr_data << endl);
		DPRINT(endl);
		wait();
	}
	
}

};

#endif
