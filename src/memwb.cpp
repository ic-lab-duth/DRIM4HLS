/* Copyright 2017 Columbia University, SLD Group */

//
// memory.cpp - Robert Margelli
// Implementation of the memwb stage ie combined memory and writeback stages.
//
#include <sstream>
#include "memwb.hpp"

#ifndef NDEBUG
  #include <iostream>
  #define DPRINT(msg) std::cout << msg;
#else
  #define DPRINT(msg)
#endif

void memwb::memwb_th(void)
{
MEMWB_RST:
	{	
		din.Reset();
		dout.Reset();

		// Write dummy data to decode feedback.
		output.regfile_address = "00000";
		output.regfile_data    = (sc_bv<XLEN>)0;
		output.regwrite        = "0";
		output.tag             = 0;

		do {wait();} while(!fetch_en); // Synchronize with fetch stage at startup.

		dout.Push(output);
		wait();
		dout.Push(output);
	}

MEMWB_BODY:
	while(true) {

		// Get
		input = din.Pop();
		//cout << "@" << sc_time_stamp() << "\t" << name() << "sending input=" << input << endl ;
		// Compute
		// *** Memory access.

		// WARNING: only supporting aligned memory accesses
		// Preprocess address
		unsigned int aligned_address = input.alu_res.to_uint();
		unsigned char byte_index = (unsigned char) ((aligned_address & 0x3) << 3);
		unsigned char halfword_index = (unsigned char) ((aligned_address & 0x2) << 3);

		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "aligned_address= "<< aligned_address <<  endl);
		aligned_address = aligned_address >> 2;
		sc_uint<BYTE> db;
		sc_uint<2 * BYTE> dh;
		sc_uint<XLEN> dw;
		wait();

		if (sc_uint<3>(input.ld) != NO_LOAD || sc_uint<2>(input.st) != NO_STORE) {
			if (input.mem_datain.to_uint() == 0x11111111 ||
			    input.mem_datain.to_uint() == 0x22222222 ||
			    input.mem_datain.to_uint() == 0x11223344 ||
			    input.mem_datain.to_uint() == 0x88776655 ||
			    input.mem_datain.to_uint() == 0x12345678 ||
			    input.mem_datain.to_uint() == 0x87654321) {
				std::stringstream stm;
				stm << hex << "D$ access here2 -> 0x" << aligned_address << ". Value: " << input.mem_datain.to_uint() << std::endl;
			}
			sc_assert(aligned_address < DCACHE_SIZE);
		}

		if (sc_uint<3>(input.ld) != NO_LOAD) {    // a load is requested
			switch(sc_uint<3>(input.ld)) {         // LOAD
			case LB_LOAD:
				db = dmem[aligned_address].range(byte_index + BYTE - 1, byte_index);
				mem_dout = ext_sign_byte(db);
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "LB_LOAD" <<  endl);
				break;
			case LH_LOAD:
				dh = dmem[aligned_address].range(halfword_index + 2 * BYTE - 1, halfword_index);
				mem_dout = ext_sign_halfword(dh);
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "LH_LOAD" <<  endl);
				break;
			case LW_LOAD:
				dw = dmem[aligned_address];
				mem_dout = dw;
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "LW_LOAD" <<  endl);
				break;
			case LBU_LOAD:
				db = dmem[aligned_address].range(byte_index + BYTE - 1, byte_index);
				mem_dout = ext_unsign_byte(db);
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "LBU_LOAD" <<  endl);
				break;
			case LHU_LOAD:
				dh = dmem[aligned_address].range(halfword_index + 2 * BYTE - 1, halfword_index);
				mem_dout = ext_unsign_halfword(dh);
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "LHU_LOAD" <<  endl);
				break;
			default:
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "NO_LOAD" <<  endl);
				break;  // NO_LOAD
			}
		}
		else if (sc_uint<2>(input.st) != NO_STORE) {  // a store is requested
			switch (sc_uint<2>(input.st)) {         // STORE
			case SB_STORE:  // store 8 bits of rs2
				db = input.mem_datain.range(BYTE - 1, 0).to_uint();
				dmem[aligned_address].range(byte_index + BYTE -1, byte_index) = db;
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "SB_STORE dmem[" << std::hex << aligned_address << "]=" << std::dec << dmem[aligned_address].range(byte_index + BYTE -1, byte_index) << endl);
				break;
			case SH_STORE:  // store 16 bits of rs2
				dh = input.mem_datain.range(2 * BYTE - 1, 0).to_uint();
				dmem[aligned_address].range(byte_index + BYTE -1, byte_index);
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "SH_STORE dmem[" << std::hex << aligned_address << "]=" << std::dec << dmem[aligned_address].range(byte_index + BYTE -1, byte_index) << endl);
				break;
			case SW_STORE:  // store rs2
				dw = input.mem_datain.to_uint();
				dmem[aligned_address] = dw;
				DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "SW_STORE dmem[" << std::hex << aligned_address << "]=" << std::dec << dmem[aligned_address] << endl);
				break;
			default:
			DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "NO STORE" << endl);
			break;  // NO_STORE
			}
		}

		// *** END of memory access.

		/* Writeback */
		output.regwrite         = input.regwrite;
		output.regfile_address  = input.dest_reg;
		output.regfile_data     = (input.memtoreg == "1")? mem_dout : input.alu_res;
		output.tag              = input.tag;

		// Put
		//cout << "@" << sc_time_stamp() << "\t" << name() << "sending output=" << output << endl ;
		dout.Push(output);
	}
}


/* Support functions */

// Sign extend byte read from memory. For LB
sc_bv<XLEN> memwb::ext_sign_byte(sc_bv<BYTE> read_data)
{
	if (read_data.range(7,7) == "1")
		return (sc_bv<BYTE*3>("111111111111111111111111"), read_data);
	else
		return (sc_bv<BYTE*3>("000000000000000000000000"), read_data);
}

// Zero extend byte read from memory. For LBU
sc_bv<XLEN> memwb::ext_unsign_byte(sc_bv<BYTE> read_data)
{
        return ("000000000000000000000000", read_data);
}

// Sign extend half-word read from memory. For LH
sc_bv<XLEN> memwb::ext_sign_halfword(sc_bv<BYTE*2> read_data)
{
	if (read_data.range(15,15) == "1")
		return (sc_bv<BYTE*2>("1111111111111111"), read_data);
	else
		return (sc_bv<BYTE*2>("0000000000000000"), read_data);
}

// Zero extend half-word read from memory. For LHU
sc_bv<XLEN> memwb::ext_unsign_halfword(sc_bv<BYTE*2> read_data)
{
        return ("0000000000000000", read_data);
}
