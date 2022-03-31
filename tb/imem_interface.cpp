/* Copyright 2017 Columbia University, SLD Group */

//
// tb.cpp - Robert Margelli
// Implementation of tb.hpp.
// In a normal run the source thread does nothing, while the sink waits for the
// program_end signal to be asserted and stops simulation
// (sc_stop).
//

#include "imem_interface.hpp"

#ifndef NDEBUG
  #include <iostream>
  #define DPRINT(msg) std::cout << msg;
#else
  #define DPRINT(msg)
#endif

// Source thread
void imem_interface::fetch_instr()
{	
	imem_in.Reset();
	imem_out.Reset();
	imem_stall_in.Reset();

	while (true)
	{	imem_din = imem_in.Pop();

		unsigned int addr;
		
		if (imem_stall_in.PopNB(stall_din)) {
			// Read from instruction memory
			if(stall_din.valid) {
				addr = imem_din.instr_addr;
				imem_dout.instr_data = imem[addr];
			}
		}else {
			addr = imem_din.instr_addr;
			imem_dout.instr_data = imem[addr];
		}

		imem_out.Push(imem_dout);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "instr_data= " << imem_dout.instr_data << endl);
		DPRINT(endl);
		wait();
	}
	
}

