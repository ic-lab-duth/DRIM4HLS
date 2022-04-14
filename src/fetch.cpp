/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Implementation of fetch stage

*/

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
		dmem_stall.Reset();
		imem_stall.Reset();

		trap = "0";
		trap_cause = NULL_CAUSE;
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
			redirect = fetch_in.redirect;
			redirect_addr = fetch_in.address;
			freeze = fetch_in.freeze;
		}else {
			redirect = false;
			freeze = false;
		}

		if (dmem_stall.PopNB(dmem_stall_d)) {
			dmem_freeze = dmem_stall_d.stall;
		}else {
			dmem_freeze = false;
		}

		if (imem_stall.PopNB(imem_stall_d)) {
			imem_freeze = imem_stall_d.stall;
		}else {
			imem_freeze = false;
		}

		// Mechanism for incrementing PC
		if (redirect && redirect_addr != pc && !dmem_freeze && !imem_freeze) {
			pc = fetch_in.address;
		}else if(!freeze && !dmem_freeze && !imem_freeze) {
			pc = sc_uint<PC_LEN>(pc + 4);			
		}

		unsigned int aligned_pc;

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
