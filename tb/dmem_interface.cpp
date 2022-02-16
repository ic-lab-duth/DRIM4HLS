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
	//data_out.write(0);
	dmem_in_read.Reset();
	dmem_in_write.Reset();
	dmem_out.Reset();

	dmem_dout.data_out = 0;
	dmem_out.Push(dmem_dout);
	while (true)
	{	
		dmem_din_read = dmem_in_read.Pop();
		dmem_din_write = dmem_in_write.Pop();
		cout << "@" << sc_time_stamp() << "\t" << name() << "\t" << "popped"<< endl;
		//unsigned int addr = data_addr.read();
		unsigned int addr_read = dmem_din_read.data_addr;
		unsigned int addr_write = dmem_din_write.data_addr;
		cout << "@" << sc_time_stamp() << "\t" << name() << "\t" << "addr_read= " << addr_read << endl;
		cout << "@" << sc_time_stamp() << "\t" << name() << "\t" << "addr_write= " << addr_write << endl;
		dmem_dout.valid = false;
		//if (read_en.read()) {	
		if (dmem_din_write.write_en) {
			//dmem[addr] = data_in.read();
			dmem[addr_write] = dmem_din_write.data_in;
			// dmem_dout.data_out = dmem[addr];
			// dmem_out.Push(dmem_dout);
			cout << "@" << sc_time_stamp() << "\t" << name() << "\t" << "write"<< endl;
		}
		if (dmem_din_read.read_en) {				
			//data_out.write(dmem[addr]);
			dmem_dout.data_out = dmem[addr_read];
			dmem_dout.valid = true;
			cout << "@" << sc_time_stamp() << "\t" << name() << "\t" << "read"<< endl;
			cout << "@" << sc_time_stamp() << "\t" << name() << "\t" << "data="<< dmem[addr_read] << endl;
			//}else if (write_en.read()) {
		}
		dmem_out.Push(dmem_dout);
		//wait();
	}
	
}

