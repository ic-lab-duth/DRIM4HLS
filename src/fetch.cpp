/* Copyright 2017 Columbia University, SLD Group */

//
// fedec.cpp - Robert Margelli
// Implementation of the fedec stage ie combined fetch and decode stages.
//

#include "fetch.hpp"

#ifndef NDEBUG
  #include <iostream>
  #define DPRINT(msg) std::cout << msg;
#else
  #define DPRINT(msg)
#endif

void fetch::fetch_th(void)
{
FETCH_RST:
	{
		dout.Reset();
		fetch_din.Reset();
		imem_din.Reset();

		trap = "0";
		trap_cause = NULL_CAUSE;
		counter = 2;
        imem_in.valid = true;
		imem_in.instr_addr = 0;

		do {wait();} while(!fetch_en);

		//  Init. pc to START_ADDRESS - 4 as on first fetch it will be incremented by
		//  4, thus fetching instruction at address 0
		pc = entry_point.read();
	}

FETCH_BODY:
	while(true) {
		//sc_assert(sc_time_stamp().to_double() < 1000000);
		
		if (fetch_din.PopNB(fetch_in)) {
			// Mechanism for incrementing PC
			if (fetch_in.redirect && counter == 2 && fetch_in.address != pc) {
				pc = fetch_in.address;
				counter = 0;
			}else if(!fetch_in.freeze) {
				pc = sc_uint<PC_LEN>(pc + 4);			
			}
		}else {
			pc = sc_uint<PC_LEN>(pc + 4);
		}
		unsigned int aligned_pc;

		// Counter becomes 0 when processor changes direction (because of jumps/branches)
		if (counter != 2) {
			counter++;
		}

		imem_pc = pc;

		aligned_pc = imem_pc >> 2;
		imem_in.instr_addr = aligned_pc;
		fe_out.pc = pc;
		
		imem_din.Push(imem_in);
		dout.Push(fe_out);
		
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "pc= " << pc << endl);
		DPRINT(endl);
		wait();

	} // *** ENDOF while(true)
}   // *** ENDOF sc_cthread
