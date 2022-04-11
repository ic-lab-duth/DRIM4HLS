/* Copyright 2017 Columbia University, SLD Group */

//
// tb.cpp - Robert Margelli
// Implementation of tb.hpp.
// In a normal run the source thread does nothing, while the sink waits for the
// program_end signal to be asserted and stops simulation
// (sc_stop).
//

#include "dmem_interface.hpp"
#include <random>

#ifndef NDEBUG
  #include <iostream>
  #define DPRINT(msg) std::cout << msg;
#else
  #define DPRINT(msg)
#endif

// Source thread
void dmem_interface::dmem_th()
{	
	dmem_in_read.Reset();
	dmem_in_write.Reset();
	dmem_out.Reset();

	stall_fe.Reset();
	stall_de.Reset();
	stall_exe.Reset();
	stall_memwb.Reset();
	stall_imem.Reset();
	
	dmem_dout.data_out = 0;

	while (true)
	{	

		dmem_din_read = dmem_in_read.Pop();
		unsigned int addr_read = dmem_din_read.data_addr;

		dmem_dout.valid = false;

		if (dmem_in_write.PopNB(dmem_din_write)) {
			unsigned int addr_write = dmem_din_write.data_addr;
			
			// Write to memory
			if (dmem_din_write.write_en) {
				dmem[addr_write] = dmem_din_write.data_in;

				stall_fe_d.stall = true;
				stall_de_d.stall = true;
				stall_exe_d.stall = true;
				stall_memwb_d.stall = true;
				stall_dmem_d.stall = true;

				std::random_device mem;
				std::mt19937 rng(mem());
				std::uniform_int_distribution<std::mt19937::result_type> dist6(1,4);
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::dec << "random waits= " << dist6(rng) << endl);
				
				unsigned int random_stalls = dist6(rng);
				for (unsigned int i = 0; i < random_stalls; i++) {
					dmem_out.Push(dmem_dout);
					stall_fe.Push(stall_fe_d);
					stall_de.Push(stall_de_d);
					stall_exe.Push(stall_exe_d);
					stall_memwb.Push(stall_memwb_d);
					stall_imem.Push(stall_dmem_d);

					wait();
					dmem_in_read.Pop();
					dmem_in_write.PopNB(dmem_din_write);
				}
			}
		}

		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "read dmem= " << dmem_din_read.read_en << endl);
		// Read from memory
		if (dmem_din_read.read_en) {				
			
			dmem_dout.data_out = dmem[addr_read];

			stall_fe_d.stall = true;
			stall_de_d.stall = true;
			stall_exe_d.stall = true;
			stall_memwb_d.stall = true;
			stall_dmem_d.stall = true;

			std::random_device mem;
			std::mt19937 rng(mem());
			std::uniform_int_distribution<std::mt19937::result_type> dist6(1,3); // distribution in range [1, 3]
			DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::dec << "random waits= " << dist6(rng) << endl);
			
			unsigned int random_stalls = dist6(rng);
			for (unsigned int i = 0; i < random_stalls; i++) {
				dmem_out.Push(dmem_dout);
				stall_fe.Push(stall_fe_d);
				stall_de.Push(stall_de_d);
				stall_exe.Push(stall_exe_d);
				stall_memwb.Push(stall_memwb_d);
				stall_imem.Push(stall_dmem_d);

				wait();
				dmem_in_read.Pop();
				dmem_in_write.PopNB(dmem_din_write);
			}

			dmem_dout.valid = true;
		}

		stall_fe_d.stall = false;
		stall_de_d.stall = false;
		stall_exe_d.stall = false;
		stall_memwb_d.stall = false;
		stall_dmem_d.stall = false;


		stall_fe.Push(stall_fe_d);
		stall_de.Push(stall_de_d);
		stall_exe.Push(stall_exe_d);
		stall_memwb.Push(stall_memwb_d);
		stall_imem.Push(stall_dmem_d);
		
		dmem_out.Push(dmem_dout);
		DPRINT(endl);
	}
	
}

