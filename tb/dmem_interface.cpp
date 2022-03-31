/* Copyright 2017 Columbia University, SLD Group */

//
// tb.cpp - Robert Margelli
// Implementation of tb.hpp.
// In a normal run the source thread does nothing, while the sink waits for the
// program_end signal to be asserted and stops simulation
// (sc_stop).
//

#include "dmem_interface.hpp"

// Source thread
void dmem_interface::dmem_th()
{	
	dmem_in_read.Reset();
	dmem_in_write.Reset();
	dmem_out.Reset();

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
			}
		}

		// Read from memory
		if (dmem_din_read.read_en) {				
			dmem_dout.data_out = dmem[addr_read];
			dmem_dout.valid = true;

		}
		
		dmem_out.Push(dmem_dout);
	}
	
}

