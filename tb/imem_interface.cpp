/* Copyright 2017 Columbia University, SLD Group */

//
// tb.cpp - Robert Margelli
// Implementation of tb.hpp.
// In a normal run the source thread does nothing, while the sink waits for the
// program_end signal to be asserted and stops simulation
// (sc_stop).
//

#include "imem_interface.hpp"

// Source thread
void imem_interface::fetch_instr()
{	
	//instr_data.write(imem[0]);
	imem_in.Reset();
	imem_out.Reset();
	while (true)
	{	imem_din = imem_in.Pop();
		//if(valid.read()) {
		if(imem_din.valid) {
			//unsigned int addr = instr_addr.read();
			unsigned int addr = imem_din.instr_addr;
			//instr_data.write(imem[addr]);

			imem_dout.instr_data = imem[addr];
			imem_out.Push(imem_dout);
			cout << "@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "index= " << addr << endl;
			cout << "@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "data= " << imem[addr] << endl;
		}

		wait();
	}
	
}

