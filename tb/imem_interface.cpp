/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Implementation of data memory

*/

#include "imem_interface.hpp"
#include <random>

#ifndef NDEBUG
  #include <iostream>
  #define DPRINT(msg) std::cout << msg;
#else
  #define DPRINT(msg)
#endif

// Source thread
void imem_interface::fetch_instr()
{	
IMEM_RST:
	{
	imem_in.Reset();
	imem_out.Reset();
	stall_in.Reset();
	dmem_stall_in.Reset();

	stall_fe.Reset();
	stall_de.Reset();
	}
IMEM_BODY:	
	while (true)
	{	imem_din = imem_in.Pop();

		unsigned int addr;

		if (dmem_stall_in.PopNB(dmem_stall_din)) {
			dmem_stall = dmem_stall_din.stall;
		}else {
			dmem_stall = false;
		}
		
		if (stall_in.PopNB(stall_din)) {
			
			valid = stall_din.valid;
		}else {
			valid = true;
		}


		if (valid && !dmem_stall) {
			addr = imem_din.instr_addr;
			imem_dout.instr_data = imem[addr];

			stall_fe_d.stall = true;
			stall_de_d.stall = true;

			std::random_device imem;
			std::mt19937 irng(imem());
			std::uniform_int_distribution<std::mt19937::result_type> dist6(1, 3);

			unsigned int random_stalls = dist6(irng);
			for (unsigned int i = 0; i < random_stalls; i++) {
				imem_out.Push(imem_dout);
				stall_fe.Push(stall_fe_d);
				stall_de.Push(stall_de_d);

				wait();

				imem_in.Pop();
				dmem_stall_in.PopNB(dmem_stall_din);
				stall_in.PopNB(stall_din);
			}

		}

		stall_fe_d.stall = false;
		stall_de_d.stall = false;

		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "stall= " << stall_fe_d.stall<< endl);
		stall_fe.Push(stall_fe_d);
		stall_de.Push(stall_de_d);

		imem_out.Push(imem_dout);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "instr_data= " << imem_dout.instr_data << endl);
		DPRINT(endl);
		wait();
	}
	
}

