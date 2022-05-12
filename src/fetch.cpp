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
		imem_dout.Reset();
		imem_de.Reset();

		trap = "0";
		trap_cause = NULL_CAUSE;
        imem_in.valid = true;
		imem_in.instr_addr = 0;


		//  Init. pc to START_ADDRESS - 4 as on first fetch it will be incremented by
		//  4, thus fetching instruction at address 0
		pc = entry_point.read();
		wait();
	}

FETCH_BODY:
	while(true) {
		//sc_assert(sc_time_stamp().to_double() < 1000000);
	
		if (fetch_din.PopNB(fetch_in)) {
			// Mechanism for incrementing PC
			redirect = fetch_in.redirect;
			redirect_addr = fetch_in.address;
			freeze = fetch_in.freeze;
		}

		// Mechanism for incrementing PC
		if (redirect && redirect_addr != pc) {
			pc = fetch_in.address;
		}else if(!freeze) {
			pc = sc_uint<PC_LEN>(pc + 4);			
		}

		unsigned int aligned_pc;

		imem_pc = pc;

		aligned_pc = imem_pc >> 2;
		imem_in.instr_addr = aligned_pc;
		
		fe_out.pc = pc;
		
		if (!freeze) {
			imem_din.Push(imem_in);

			imem_out = imem_dout.Pop();
			
			imem_de.Push(imem_out);
			dout.Push(fe_out);
		}
		
		
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "pc= " << pc << endl);
		DPRINT(endl);
		wait();

	} // *** ENDOF while(true)
}   // *** ENDOF sc_cthread
