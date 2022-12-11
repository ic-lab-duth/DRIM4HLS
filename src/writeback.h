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


#include "drim4hls_datatypes.h"
#include "defines.h"
#include "globals.h"

#include <mc_connections.h>
#include <ac_int.h>

SC_MODULE(writeback) {
    #ifndef __SYNTHESIS__
    struct writeback_out // TODO: fix all sizes
    {
        //
        // Member declarations.
        //		
        unsigned int aligned_address;
        sc_uint < XLEN > load_data;
        sc_uint < XLEN > store_data;
        std::string load;
        std::string store;

    }
    writeback_out_t;
    #endif

    // FlexChannel initiators
    Connections::In < exe_out_t > CCS_INIT_S1(din);
    Connections::In < dmem_out_t > CCS_INIT_S1(dmem_out);

    Connections::Out < mem_out_t > CCS_INIT_S1(dout);
    Connections::Out < dmem_in_t > CCS_INIT_S1(dmem_in);

    // Clock and reset signals
    sc_in < bool > CCS_INIT_S1(clk);
    sc_in < bool > CCS_INIT_S1(rst);
	
    // Member variables
    exe_out_t input;
    dmem_in_t dmem_dout;
    dmem_out_t dmem_din;
    mem_out_t output;

    ac_int < DATA_SIZE, false > mem_dout;
    ac_int < DCACHE_LINE, false > dmem_data;
    ac_int < XLEN, false > dmem_data_offset;
    
    dcache_data_t dcache_data[DCACHE_ENTRIES][DCACHE_WAYS];
    dcache_tag_t dcache_tags[DCACHE_ENTRIES][DCACHE_WAYS];
    dcache_out_t dcache_out;
    
    dcache_data_t cache_data[1][DCACHE_WAYS];
    dcache_tag_t cache_tag[1][DCACHE_WAYS];

    ac_int < DCACHE_TAG_WIDTH, false > tag;
    ac_int < DCACHE_INDEX_WIDTH, false > index;
    ac_int < DCACHE_OFFSET_WIDTH + 1, false> offset;
    
    bool freeze;
    bool freeze_write;
    // Constructor
    SC_CTOR(writeback): din("din"), dout("dout"), dmem_in("dmem_in"), dmem_out("dmem_out"), clk("clk"), rst("rst") {
        SC_THREAD(writeback_th);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);
    }

    void writeback_th(void) {
        WRITEBACK_RST: {
            din.Reset();
            dmem_out.Reset();

            dout.Reset();
            dmem_in.Reset();
			
            // Write dummy data to decode feedback.
            output.regfile_address = 0;
            output.regfile_data = 0;
            output.regwrite = 0;
            output.tag = 0;
			
			dcache_out.data = 0;
            dcache_out.hit = false;
			
            dmem_data = 0;
            dmem_data_offset = 0;
            mem_dout = 0;
            
            tag = 0;
            index = 0;
            offset = 0;

			freeze = false;
			freeze_write = false;
        }

        #pragma hls_pipeline_init_interval 1
        #pragma pipeline_stall_mode flush
        WRITEBACK_BODY: while (true) {

            // Get
            if (!freeze) {
				input = din.Pop();
			}
            #ifndef __SYNTHESIS__
                writeback_out_t.aligned_address = 0;
                writeback_out_t.load_data = 0;
                writeback_out_t.store_data = 0;
                writeback_out_t.load = "NO_LOAD";
                writeback_out_t.store = "NO_STORE";
            #endif
            
            // Compute
            // *** Memory access.
			dmem_data = 0;
			dmem_data_offset = 0;
            // WARNING: only supporting aligned memory accesses
            // Preprocess address
			
            unsigned int aligned_address = input.alu_res.to_uint();
            ac_int< 5, false> byte_index = (ac_int< 5, false>)((aligned_address & 0x3) << 3);
            ac_int< 5, false> halfword_index = (ac_int< 5, false>)((aligned_address & 0x2) << 3);

            aligned_address = aligned_address >> 2;
            ac_int < BYTE, false > db = (ac_int < BYTE, false >) 0;
            ac_int < 2 * BYTE, false > dh = (ac_int < 2 * BYTE, false >) 0;
            ac_int < XLEN, false > dw = (ac_int < XLEN, false >) 0;

            dmem_dout.data_addr = aligned_address;

            dmem_dout.read_en = false;
            dmem_dout.write_en = false;
            
            ac_int < XLEN, false > addr = aligned_address;
            
            tag = addr.slc<DCACHE_TAG_WIDTH>(DCACHE_INDEX_WIDTH + DCACHE_OFFSET_WIDTH);
            index = addr.slc<DCACHE_INDEX_WIDTH>(DCACHE_OFFSET_WIDTH);
            if (DCACHE_OFFSET_WIDTH) {
                offset = addr.slc<DCACHE_OFFSET_WIDTH>(0);
            }
            else {
				offset = 0;
			}
           //unsigned int max_offset = 1 << DCACHE_OFFSET_WIDTH;

            #ifndef __SYNTHESIS__
            if (sc_uint < 3 > (input.ld) != NO_LOAD || sc_uint < 2 > (input.st) != NO_STORE) {
                if (input.mem_datain.to_uint() == 0x11111111 ||
                    input.mem_datain.to_uint() == 0x22222222 ||
                    input.mem_datain.to_uint() == 0x11223344 ||
                    input.mem_datain.to_uint() == 0x88776655 ||
                    input.mem_datain.to_uint() == 0x12345678 ||
                    input.mem_datain.to_uint() == 0x87654321) {
                    std::stringstream stm;
                    stm << hex << "D$ access here2 -> 0x" << aligned_address << ". Value: " << input.mem_datain.to_uint() << std::endl;
                }
                //sc_assert(aligned_address < DCACHE_SIZE);
            }
            #endif
            
			if ((input.ld != NO_LOAD || input.st != NO_STORE) && !freeze) { // a load is requested
				freeze = true;
                dcache_out = dcache();

                switch (dcache_out.hit)
                {
                case CACHE_HIT:
                    dmem_data = dcache_out.data;
                    dmem_data_offset = dmem_data.slc<DATA_WIDTH>(offset*DATA_WIDTH);
                    
                    if (cache_tag[0][0].dirty && input.st != NO_STORE && cache_tag[0][0].tag != tag) {
                        dmem_dout.write_en = true;
                        dmem_dout.data_in = dcache_out.data;
                        if (DCACHE_OFFSET_WIDTH) {
							dmem_dout.write_addr = 0;
						}
						dmem_dout.write_addr.set_slc(DCACHE_OFFSET_WIDTH, index);
						dmem_dout.write_addr.set_slc(DCACHE_INDEX_WIDTH + DCACHE_OFFSET_WIDTH, cache_tag[0][0].tag);
                        
                        dmem_in.Push(dmem_dout);
                    }

                    break;
                case CACHE_MISS:
				
                    dmem_dout.read_en = true;
                    if (cache_tag[0][DCACHE_WAYS - 1].dirty && cache_tag[0][DCACHE_WAYS - 1].tag != tag) {
                        dmem_dout.write_en = true;
                        dmem_dout.data_in = dcache_out.data;
                        
                        if (DCACHE_OFFSET_WIDTH) {
							dmem_dout.write_addr = 0;
						}
						dmem_dout.write_addr.set_slc(DCACHE_OFFSET_WIDTH, index);
						dmem_dout.write_addr.set_slc(DCACHE_INDEX_WIDTH + DCACHE_OFFSET_WIDTH, cache_tag[0][DCACHE_WAYS - 1].tag);
                    }
                    
                    dmem_in.Push(dmem_dout);
					
                    dmem_din = dmem_out.Pop();
                    dmem_data = dmem_din.data_out;
					dmem_data_offset = dmem_data.slc<DATA_WIDTH>(offset*DATA_WIDTH);

                    dmem_dout.data_in = cache_data[0][DCACHE_WAYS - 1].data;
                    
                    if (input.ld != NO_LOAD) {
						cache_data[0][DCACHE_WAYS - 1].data = dmem_data;
					}
                    
                    break;
                default:
                    break;
                }
            }
            
            if (input.ld != NO_LOAD) { // a load is requested
                
                switch (input.ld) { // LOAD
                case LB_LOAD:                    
                    db.set_slc(0, dmem_data_offset.slc<BYTE>(byte_index));
                    mem_dout = ext_sign_byte(db);

                    #ifndef __SYNTHESIS__
                    writeback_out_t.load_data = mem_dout;
                    writeback_out_t.load = "LB_LOAD";
                    #endif

                    break;
                case LH_LOAD:
                    dh.set_slc(0, dmem_data_offset.slc< 2*BYTE >(halfword_index));
                    mem_dout = ext_sign_halfword(dh);

                    #ifndef __SYNTHESIS__
                    writeback_out_t.load_data = mem_dout;
                    writeback_out_t.load = "LH_LOAD";
                    #endif

                    break;
                case LW_LOAD:
                    dw = dmem_data_offset;
                    mem_dout = dw;

                    #ifndef __SYNTHESIS__
                    writeback_out_t.load_data = mem_dout;
                    writeback_out_t.load = "LW_LOAD";
                    #endif

                    break;
                case LBU_LOAD:
                    db.set_slc(0, dmem_data_offset.slc<BYTE>(byte_index));
                    mem_dout = ext_unsign_byte(db);

                    #ifndef __SYNTHESIS__
                    writeback_out_t.load_data = mem_dout;
                    writeback_out_t.load = "LBU_LOAD";
                    #endif

                    break;
                case LHU_LOAD:
                    dh.set_slc(0, dmem_data_offset.slc< 2 * BYTE >(halfword_index));
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

                    break; // NO_LOAD
                }
            } else if (input.st != NO_STORE) { // a store is requested
            
                switch (input.st) { // STORE
                case SB_STORE: // store 8 bits of rs2
					
					db.set_slc(0, (ac_int < BYTE, false >) input.mem_datain.slc<BYTE>(0));
					dmem_data_offset.set_slc(byte_index, (ac_int < BYTE, false >) db);

                    #ifndef __SYNTHESIS__
                    writeback_out_t.store_data = db;
                    writeback_out_t.store = "SB_STORE";
                    #endif
					
                    break;
                case SH_STORE: // store 16 bits of rs2

					dh.set_slc(0, input.mem_datain.slc<2*BYTE>(0));
					dmem_data_offset.set_slc(byte_index, dh);
                    
                    #ifndef __SYNTHESIS__
                    writeback_out_t.store_data = dh;
                    writeback_out_t.store = "SH_STORE";
                    #endif
					
                    break;
                case SW_STORE: // store rs2
                    dw = input.mem_datain;
                    dmem_data_offset = dw;

                    #ifndef __SYNTHESIS__
                    writeback_out_t.store_data = dw;
                    writeback_out_t.store = "SW_STORE";
                    #endif
					
					break;
                default:

                    #ifndef __SYNTHESIS__
                    writeback_out_t.store = "NO_STORE";
                    #endif
					
                    break; // NO_STORE
                }
				dmem_data.set_slc(offset*DATA_WIDTH, dmem_data_offset);

                if (dcache_out.hit) {
                    dmem_dout.data_in = cache_data[0][0].data;
                    cache_data[0][0].data = dmem_data;
                }else {
                    dmem_dout.data_in = cache_data[0][DCACHE_WAYS - 1].data;
                    cache_data[0][DCACHE_WAYS - 1].data = dmem_data;
                }
                
            }
            
            bool ld_st_write = (input.st != NO_STORE) ? false : true;

			if ((input.st != NO_STORE || input.ld != NO_LOAD) && !freeze_write) {
				dcache_write(ld_st_write, dcache_out.hit);
			}
			
            // *** END of memory access.
            
            /* Writeback */
            output.regwrite = input.regwrite;
            output.regfile_address = input.dest_reg;
            output.regfile_data = (input.memtoreg[0] == 1) ? mem_dout : input.alu_res;
            output.tag = input.tag;
            output.pc = input.pc;

            // Put
            freeze = false;
            freeze_write = false;
		    dout.Push(output);
            #ifndef __SYNTHESIS__
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "load= " << writeback_out_t.load << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "store= " << writeback_out_t.store << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "input.regwrite=" << input.regwrite << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "regwrite=" << output.regwrite << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "aligned_address=" << aligned_address << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "mem_dout=" << mem_dout << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "input.alu_res=" << input.alu_res << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "output.regfile_address=" << output.regfile_address << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "output.regfile_data=" << output.regfile_data << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "input.memtoreg=" << input.memtoreg << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "writeback_out_t.store_data =" << writeback_out_t.store_data  << endl);
            DPRINT(endl);
            #endif
            wait();
        }
    }

    /* Support functions */

    // Sign extend byte read from memory. For LB
    ac_int < XLEN, false > ext_sign_byte(ac_int < BYTE, false > read_data) {
		ac_int <XLEN, false> extended = 0;
		if (read_data[7] == 1) {
			extended.set_slc(0, read_data);
			extended.set_slc(BYTE, (ac_int < BYTE * 3, false >) 16777216);
		}
		else {
			extended.set_slc(0, read_data);
			extended.set_slc(BYTE, (ac_int < BYTE * 3, false >) 0);		
		}
		return extended;
    }

    // Zero extend byte read from memory. For LBU
    ac_int < XLEN, false > ext_unsign_byte(ac_int < BYTE, false > read_data) {
		ac_int <XLEN, false> extended = 0;
		extended.set_slc(0, read_data);
		extended.set_slc(BYTE, (ac_int < BYTE * 3, false >) 0);
		return extended;        
    }

    // Sign extend half-word read from memory. For LH
    ac_int < XLEN, false > ext_sign_halfword(ac_int < BYTE * 2, false > read_data) {
		ac_int <XLEN, false> extended = 0;
		        
        if (read_data[15] == 1) {
			extended.set_slc(0, read_data);
			extended.set_slc(2*BYTE, (ac_int < BYTE * 2, false >) 65535);
        }
        else {
			extended.set_slc(0, read_data);
			extended.set_slc(2*BYTE, (ac_int < BYTE * 2, false >) 0);
        }
        return extended;
    }

    // Zero extend half-word read from memory. For LHU
    ac_int < XLEN, false > ext_unsign_halfword(ac_int < BYTE * 2, false > read_data) {
		ac_int <XLEN, false> extended = 0;
		extended.set_slc(0, read_data);
		extended.set_slc(2*BYTE, (ac_int < BYTE * 2, false >) 0);
		return extended;
    }
    
    dcache_out_t dcache () {

        dcache_out_t dout;
        dcache_tag_t tmp_tag;
        dcache_data_t tmp_data;
        dout.data = 0;
        dout.hit = false;

		int i = 0;
        int j = 0;

        for (i = 0; i < DCACHE_WAYS; i++) {
            cache_tag[0][i] = dcache_tags[index][i];
            cache_data[0][i] = dcache_data[index][i];

            if ((tag == cache_tag[0][i].tag) && (cache_tag[0][i].valid)) {
                              
                tmp_data = cache_data[0][i];
                tmp_tag = cache_tag[0][i];
                dout.hit = true;
                j = i;

            }

			if (dout.hit && i < j + 1) {
                cache_data[0][i] = cache_data[0][i-1];
                cache_tag[0][i] = cache_tag[0][i-1];
			}

		}

        if (dout.hit) {
			
            cache_data[0][0] = tmp_data;
            cache_tag[0][0] = tmp_tag;
		}else {
			tmp_data = cache_data[0][DCACHE_WAYS - 1];
		}

        dout.data = tmp_data.data;

        return dout;
    }
    
    void dcache_write (bool load, bool hit) {
		
		freeze_write = true;
		
		unsigned int bank = 0;
        if (!hit) {
            bank = DCACHE_WAYS - 1;
        }


        if (cache_tag[0][bank].dirty && cache_tag[0][bank].tag != tag && !(load && hit)) {

            cache_tag[0][bank].tag = tag;
            cache_tag[0][bank].valid = true;
     
        } else if (!(load && hit)) { 
            
            cache_tag[0][bank].tag = tag;
            cache_tag[0][bank].valid = true;
        }

        if (!load) {
            cache_tag[0][bank].dirty = true;
        }else if (!hit){
            cache_tag[0][bank].dirty = false;
        }
        
        int i = 0;
        for (i = 0; i < DCACHE_WAYS; i++) {                 
            dcache_data[index][i] = cache_data[0][i];
            dcache_tags[index][i] = cache_tag[0][i];

        }      

    }

};

#endif
