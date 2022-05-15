/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Header file for writeback stage.

	@note Changes from HL5

		- Use of HLSLibs connections for communication with the rest of the processor.

		- Memory is outside of the processor

*/

#ifndef __WRITEBACK__H
#define __WRITEBACK__H


#ifndef __SYNTHESIS__
	#include <sstream>
#endif

#ifndef NDEBUG
  #include <iostream>
  #define DPRINT(msg) std::cout << msg;
#endif

#include <systemc.h>

#include "defines.h"
#include "globals.h"
#include "drim4hls_datatypes.h"

#include <mc_connections.h>

SC_MODULE(writeback)
{
    #ifndef __SYNTHESIS__
        struct writeback_out        // TODO: fix all sizes
        {
            //
            // Member declarations.
            //		
            unsigned int aligned_address;
            sc_uint<XLEN> load_data;
            sc_uint<XLEN> store_data;
            std::string load;
            std::string store;

        } writeback_out_t;
    #endif

	// FlexChannel initiators
	Connections::In< exe_out_t > CCS_INIT_S1(din);
	Connections::In< dmem_out_t > CCS_INIT_S1(dmem_out);

	Connections::Out< mem_out_t > CCS_INIT_S1(dout);
	Connections::Out< dmem_in_t > CCS_INIT_S1(dmem_in);

	// Clock and reset signals
	sc_in<bool> CCS_INIT_S1(clk);
	sc_in<bool> CCS_INIT_S1(rst);

    // Member variables
    bool freeze;
	bool loading_data;
	bool storing_data;

	exe_out_t input;
	dmem_in_t dmem_dout;
	dmem_out_t dmem_din;
	mem_out_t output;
    
	sc_bv<DATA_SIZE> mem_dout;  

	// Constructor
    SC_CTOR(writeback)
		: din("din")
		, dout("dout")
		, dmem_in("dmem_in")
		, dmem_out("dmem_out")
		, clk("clk")
		, rst("rst")
	{
		SC_THREAD(writeback_th);
        sensitive << clk.pos();
		async_reset_signal_is(rst, false);
	}

    void writeback_th(void)
    {
    WRITEBACK_RST:
        {	
            din.Reset();
            dmem_out.Reset();

            dout.Reset();
            dmem_in.Reset();

            // Write dummy data to decode feedback.
            output.regfile_address = "00000";
            output.regfile_data    = (sc_bv<XLEN>)0;
            output.regwrite        = "0";
            output.tag             = 0;

        }

    WRITEBACK_BODY:
        while(true) {

            // Get
            if (!freeze) {
                input = din.Pop();

                #ifndef __SYNTHESIS__
                    writeback_out_t.aligned_address = 0;
                    writeback_out_t.load_data = 0;
                    writeback_out_t.store_data = 0;
                    writeback_out_t.load = "NO_LOAD";
                    writeback_out_t.store = "NO_STORE";
                #endif
            }
            
            sc_uint<XLEN> dmem_data;
            
            if (dmem_out.PopNB(dmem_din)) {
                dmem_data = dmem_din.data_out;
                loading_data = false;
                storing_data = false;

                dmem_dout.read_en = false;
                dmem_dout.write_en = false;
            }
            // Compute
            // *** Memory access.

            // WARNING: only supporting aligned memory accesses
            // Preprocess address
            unsigned int aligned_address = input.alu_res.to_uint();
            unsigned char byte_index = (unsigned char) ((aligned_address & 0x3) << 3);
            unsigned char halfword_index = (unsigned char) ((aligned_address & 0x2) << 3);

            aligned_address = aligned_address >> 2;
            sc_uint<BYTE> db;
            sc_uint<2 * BYTE> dh;
            sc_uint<XLEN> dw;

            dmem_dout.data_addr = aligned_address;

            if (sc_uint<3>(input.ld) != NO_LOAD && !freeze) {
                dmem_dout.read_en = true;
                loading_data = true;

                #ifndef __SYNTHESIS__
                    writeback_out_t.load = "LOAD";
                #endif
            }


            if (sc_uint<2>(input.st) != NO_STORE && !freeze) {
                dmem_dout.write_en = true;
                storing_data = true;

            }	

            #ifndef __SYNTHESIS__
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
            #endif

            if (sc_uint<3>(input.ld) != NO_LOAD && !loading_data) {    // a load is requested
                
                freeze = false;
                switch(sc_uint<3>(input.ld)) {         // LOAD
                case LB_LOAD:
                    db = dmem_data.range(byte_index + BYTE - 1, byte_index);
                    mem_dout = ext_sign_byte(db);

                    #ifndef __SYNTHESIS__
                        writeback_out_t.load_data = mem_dout;
                        writeback_out_t.load = "LB_LOAD";
                    #endif

                    break;
                case LH_LOAD:
                    dh = dmem_data.range(halfword_index + 2 * BYTE - 1, halfword_index);				
                    mem_dout = ext_sign_halfword(dh);

                    #ifndef __SYNTHESIS__
                        writeback_out_t.load_data = mem_dout;
                        writeback_out_t.load = "LH_LOAD";
                    #endif

                    break;
                case LW_LOAD:
                    dw = dmem_data;
                    mem_dout = dw;

                    #ifndef __SYNTHESIS__
                        writeback_out_t.load_data = mem_dout;
                        writeback_out_t.load = "LW_LOAD";
                    #endif

                    break;
                case LBU_LOAD:
                    db = dmem_data.range(byte_index + BYTE - 1, byte_index);
                    mem_dout = ext_unsign_byte(db);

                    #ifndef __SYNTHESIS__
                        writeback_out_t.load_data = mem_dout;
                        writeback_out_t.load = "LBU_LOAD";
                    #endif

                    break;
                case LHU_LOAD:
                    dh = dmem_data.range(halfword_index + 2 * BYTE - 1, halfword_index);
                    mem_dout = ext_unsign_halfword(dh);

                    #ifndef __SYNTHESIS__
                        writeback_out_t.load_data = mem_dout;
                        writeback_out_t.load = "LHU_LOAD";
                    #endif

                    break;
                default:

                    #ifndef __SYNTHESIS__
                        writeback_out_t.load_data = mem_dout;
                        writeback_out_t.load = "NO_LOAD";
                    #endif

                    break;  // NO_LOAD
                }
            }
            else if (sc_uint<2>(input.st) != NO_STORE && !freeze) {  // a store is requested

                switch (sc_uint<2>(input.st)) {         // STORE
                case SB_STORE:  // store 8 bits of rs2
                    db = input.mem_datain.range(BYTE - 1, 0).to_uint();
                    dmem_data.range(byte_index + BYTE -1, byte_index) = db;
                    
                    #ifndef __SYNTHESIS__
                        writeback_out_t.store_data = db;
                        writeback_out_t.store = "SB_STORE";
                    #endif
                    
                    break;
                case SH_STORE:  // store 16 bits of rs2
                    dh = input.mem_datain.range(2 * BYTE - 1, 0).to_uint();
                    dmem_data.range(byte_index + BYTE -1, byte_index) = dh;
                    
                    #ifndef __SYNTHESIS__
                        writeback_out_t.store_data = dh;
                        writeback_out_t.store = "SH_STORE";
                    #endif
                    
                    break;
                case SW_STORE:  // store rs2
                    dw = input.mem_datain.to_uint();
                    dmem_data = dw;

                    #ifndef __SYNTHESIS__
                        writeback_out_t.store_data = dw;
                        writeback_out_t.store = "SW_STORE";
                    #endif

                    break;
                default:

                    #ifndef __SYNTHESIS__
                        writeback_out_t.store = "NO_STORE";
                    #endif

                    break;  // NO_STORE
                }
                
                dmem_dout.data_in = dmem_data;
            }
            // *** END of memory access.
            if (sc_uint<2>(input.st) != NO_STORE && !storing_data) {
                freeze = false;
            }
            /* Writeback */
            output.regwrite         = (storing_data || loading_data)? "0" : input.regwrite;
            output.regfile_address  = input.dest_reg;
            output.regfile_data     = (input.memtoreg == "1")? mem_dout : input.alu_res;
            output.tag              = input.tag;
            output.pc               = input.pc;
            
            // Put
            if ((storing_data || loading_data) && !freeze) {
                dmem_in.Push(dmem_dout);
                freeze = true;
            }
            
            if (!freeze && output.regwrite == "1") {
                dout.Push(output);
            }

            #ifndef __SYNTHESIS__
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "freeze=" << freeze << endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "load= " << writeback_out_t.load << endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "store= " << writeback_out_t.store << endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "input.regwrite=" << input.regwrite << endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "regwrite=" << output.regwrite << endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "mem_dout=" << mem_dout << endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "input.alu_res=" << input.alu_res << endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "output.regfile_address=" << output.regfile_address<< endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "output.regfile_data=" << output.regfile_data << endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "input.memtoreg=" << input.memtoreg << endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "pushed to memory=" << ((storing_data || loading_data) && !freeze) << endl);
                DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "pushed to fetch=" << (!freeze && output.regwrite == "1") << endl);
                DPRINT(endl);
            #endif
            wait();
        }
    }


    /* Support functions */

    // Sign extend byte read from memory. For LB
    sc_bv<XLEN> ext_sign_byte(sc_bv<BYTE> read_data)
    {
        if (read_data.range(7,7) == "1")
            return (sc_bv<BYTE*3>("111111111111111111111111"), read_data);
        else
            return (sc_bv<BYTE*3>("000000000000000000000000"), read_data);
    }

    // Zero extend byte read from memory. For LBU
    sc_bv<XLEN> ext_unsign_byte(sc_bv<BYTE> read_data)
    {
            return ("000000000000000000000000", read_data);
    }

    // Sign extend half-word read from memory. For LH
    sc_bv<XLEN> ext_sign_halfword(sc_bv<BYTE*2> read_data)
    {
        if (read_data.range(15,15) == "1")
            return (sc_bv<BYTE*2>("1111111111111111"), read_data);
        else
            return (sc_bv<BYTE*2>("0000000000000000"), read_data);
    }

    // Zero extend half-word read from memory. For LHU
    sc_bv<XLEN> ext_unsign_halfword(sc_bv<BYTE*2> read_data)
    {
            return ("0000000000000000", read_data);
    }

};


#endif
