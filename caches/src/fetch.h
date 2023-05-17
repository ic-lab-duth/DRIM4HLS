/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Header file for fetch stage

	@note Changes from HL5
		- Implements the logic only for the fetch part from fedec.hpp.

		- Use of HLSLibs connections for communication with the rest of the processor.

		- Increment program counter based on new stall functionality.


*/

#ifndef __FETCH__H
#define __FETCH__H

#ifndef NDEBUG
    #include <iostream>
    #define DPRINT(msg) std::cout << msg;
#endif


#include "drim4hls_datatypes.h"
#include "defines.h"
#include "globals.h"

#include <mc_connections.h>

SC_MODULE(fetch) {
    public:
    // Clock and reset signals
    sc_in < bool > CCS_INIT_S1(clk);
    sc_in < bool > CCS_INIT_S1(rst);
    // Channel ports
    Connections::In < fe_in_t > CCS_INIT_S1(fetch_din);
    Connections::In < imem_out_t > CCS_INIT_S1(imem_dout);
    Connections::Out < imem_in_t > CCS_INIT_S1(imem_din);
    Connections::Out < fe_out_t > CCS_INIT_S1(dout);

    // Trap signals. TODO: not used. Left for future implementations.
    sc_signal < bool > CCS_INIT_S1(trap); //sc_out
    sc_signal < ac_int < LOG2_NUM_CAUSES, false > > CCS_INIT_S1(trap_cause); //sc_out

    // *** Internal variables
    sc_int < PC_LEN > pc; // Init. to -4, then before first insn fetch it will be updated to 0.	 
    sc_uint < PC_LEN > imem_pc; // Used in fetching from instruction memory
	sc_uint < PC_LEN > pc_tmp; // Init. to -4, then before first insn fetch it will be updated to 0.	 
    // Custom datatypes used for retrieving and sending data through the channels
    imem_in_t imem_in; // Contains data for fetching from the instruction memory
    fe_out_t fe_out; // Contains data for the decode stage
    fe_in_t fetch_in; // Contains data from the decode stage used in incrementing the PC
    imem_out_t imem_out;
		
    bool redirect;
    bool redirect_tmp;
    
    sc_uint < PC_LEN > redirect_addr;
	sc_uint < PC_LEN > redirect_addr_tmp;
	
	sc_uint < DATA_SIZE > mem_dout;
    sc_uint < ICACHE_LINE > imem_data;
    sc_uint < XLEN > imem_data_offset;
    
    sc_uint < ICACHE_TAG_WIDTH + ICACHE_INDEX_WIDTH + 1 > icache_buffer_addr[ICACHE_BUFFER_SIZE][ICACHE_WAYS];
    sc_uint < ICACHE_LINE > icache_buffer_instr[ICACHE_BUFFER_SIZE][ICACHE_WAYS];
    
    icache_data_t icache_data[ICACHE_ENTRIES][ICACHE_WAYS];
    icache_tag_t icache_tags[ICACHE_ENTRIES][ICACHE_WAYS];
    icache_out_t icache_out;
    
    icache_data_t cache_data[1][ICACHE_WAYS];
    icache_tag_t cache_tag[1][ICACHE_WAYS];

    sc_uint < ICACHE_TAG_WIDTH > tag;
    sc_uint < ICACHE_INDEX_WIDTH > index;
    sc_uint < ICACHE_OFFSET_WIDTH + 1 > offset;
    
    sc_uint < ICACHE_TAG_WIDTH + ICACHE_INDEX_WIDTH > buffer_addr;

    bool freeze;
	bool hit_buffer;
		
    SC_CTOR(fetch): imem_din("imem_din"),
    fetch_din("fetch_din"),
    dout("dout"),
    imem_dout("imem_dout"),
    clk("clk"),
    rst("rst") {
        SC_THREAD(fetch_th);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

    }

    void fetch_th(void) {
        FETCH_RST: {
            dout.Reset();
            fetch_din.Reset();
            imem_din.Reset();
            imem_dout.Reset();
            
            tag = 0;
            index= 0;
            offset = 0;
									
            trap = 0;
            trap_cause = NULL_CAUSE;
            imem_in.instr_addr = 0;
            
            redirect_addr = 0;
			freeze = false;
			redirect = false;
			hit_buffer = false;
            //  Init. pc to START_ADDRESS - 4 as on first fetch it will be incremented by
            //  4, thus fetching instruction at address 0
            pc = -4;
            pc_tmp = -4;
            buffer_addr = 0;
            
            int n = 0;
            int l = 0;
            for (n = 0; n < ICACHE_BUFFER_SIZE; n++) {
                for (l = 0; l < ICACHE_WAYS; l++) {             
				    icache_buffer_addr[n][l] = 0;
				    icache_buffer_instr[n][l] = 0;
				}
			}
            
            wait();
        }
        #pragma hls_pipeline_init_interval 1
        #pragma pipeline_stall_mode flush
        FETCH_BODY: while (true) {
            //sc_assert(sc_time_stamp().to_double() < 1500000);
            
            if (fetch_din.PopNB(fetch_in)) {
                // Mechanism for incrementing PC
                redirect = fetch_in.redirect;
                redirect_addr = fetch_in.address;
                freeze = fetch_in.freeze;
            }

            // Mechanism for incrementing PC
            if ((redirect && redirect_addr != pc) || freeze) {
                pc = redirect_addr;
            } else if (!freeze) {
                pc = (pc + 4);
            }
			
            //imem_in.instr_addr = pc;
			hit_buffer = false;
            fe_out.pc = pc;
            
            unsigned int aligned_addr = pc >> 2;
            imem_in.instr_addr = aligned_addr;
            
            sc_uint < XLEN > addr = aligned_addr;
            
            tag = addr.range(ICACHE_TAG_WIDTH + ICACHE_INDEX_WIDTH + ICACHE_OFFSET_WIDTH - 1, ICACHE_INDEX_WIDTH + ICACHE_OFFSET_WIDTH);
            index = addr.range(ICACHE_INDEX_WIDTH + ICACHE_OFFSET_WIDTH - 1,ICACHE_OFFSET_WIDTH);        
			if (ICACHE_OFFSET_WIDTH) {
                offset = addr.range(ICACHE_OFFSET_WIDTH - 1, 0);
            }
            else {
				offset = 0;
			}
			
			buffer_addr.range(ICACHE_INDEX_WIDTH - 1, 0) = index;
			buffer_addr.range (ICACHE_TAG_WIDTH + ICACHE_INDEX_WIDTH - 1, ICACHE_INDEX_WIDTH) = tag;
			
			int m = 0;
			int n = 0;
			int k = ICACHE_BUFFER_SIZE - 1;
			
			for (k; k > 0; k = k - 1) {
				for (m = 0; m < ICACHE_WAYS; m++) {
					icache_buffer_addr[k][m] = icache_buffer_addr[k-1][m];
					icache_buffer_instr[k][m] = icache_buffer_instr[k-1][m]; 
				}                 
			}
			
			icache_out = icache();
			
			for (m = 0; m < ICACHE_WAYS; m++) {                 
				icache_buffer_instr[0][m] = cache_data[0][m].data;
				icache_buffer_addr[0][m].range(0, 0) = (ac_int <1, false>) cache_tag[0][m].valid;
				icache_buffer_addr[0][m].range(ICACHE_INDEX_WIDTH, 1) = index;
				icache_buffer_addr[0][m].range(ICACHE_TAG_WIDTH + ICACHE_INDEX_WIDTH, ICACHE_INDEX_WIDTH + 1) = cache_tag[0][m].tag;  
			}
			
			if (!icache_out.hit) {
				for (n = 0; n < ICACHE_BUFFER_SIZE; n++) {
					for (m = 0; m < ICACHE_WAYS; m++) {                 
						if (icache_buffer_addr[n][m].range(ICACHE_TAG_WIDTH + ICACHE_INDEX_WIDTH, 1) == buffer_addr && icache_buffer_addr[n][m].range(0, 0) == 1) {
							icache_out.data = icache_buffer_instr[n][m];
							icache_out.hit = true;
							hit_buffer = true;
						}
					}
				}
			}
			
			sc_uint<ICACHE_LINE> imem_data_tmp = 0;
			int j = 0;
            switch (icache_out.hit)
            {
				case CACHE_HIT:
                    imem_data = icache_out.data;
                    
                    //imem_data_offset = (sc_uint < XLEN >) imem_data.range(offset*DATA_WIDTH + DATA_WIDTH - 1, offset*DATA_WIDTH);
                    
                    for (int i = 0; i < ICACHE_LINE; i++) {
						if (i >= offset*DATA_WIDTH && i <= offset*DATA_WIDTH + DATA_WIDTH - 1) {
							imem_data_offset[j] = imem_data[i];
							j++;
						}
					}
                    
                    fe_out.instr_data = imem_data_offset;
                    break;
                case CACHE_MISS:
				                    
                    imem_din.Push(imem_in);

					imem_out = imem_dout.Pop();
					
                    imem_data = imem_out.instr_data;
                    
                    for (int i = 0; i < ICACHE_LINE; i++) {
						if (i >= offset*DATA_WIDTH && i <= offset*DATA_WIDTH + DATA_WIDTH - 1) {
							imem_data_offset[j] = imem_data[i];
							j++;
						}
					}
					//imem_data_offset = (sc_uint < XLEN >) imem_data.range(offset*DATA_WIDTH + DATA_WIDTH - 1, offset*DATA_WIDTH);
					fe_out.instr_data = imem_data_offset;
					
					icache_buffer_addr[0][ICACHE_WAYS - 1].range(0, 0) = 1;
					icache_buffer_addr[0][ICACHE_WAYS - 1].range(ICACHE_TAG_WIDTH + ICACHE_INDEX_WIDTH, 1) = buffer_addr;
					icache_buffer_instr[0][ICACHE_WAYS - 1] = imem_data;
                    
                    break;
                default:
                    break;
            }
            
			icache_write();
			
            dout.Push(fe_out);
			
			#ifndef __SYNTHESIS__
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "pc= " << pc << endl);
            DPRINT(endl);
            #endif
            wait();

        } // *** ENDOF while(true)
    } // *** ENDOF sc_cthread
    
    icache_out_t icache () {

        icache_out_t iout;
        icache_tag_t tmp_tag;
        icache_data_t tmp_data;
        iout.data = 0;
        iout.hit = false;

		int i = 0;
        int j = 0;

        for (i = 0; i < ICACHE_WAYS; i++) {
            cache_tag[0][i] = icache_tags[index][i];
            cache_data[0][i] = icache_data[index][i];

            if ((tag == cache_tag[0][i].tag) && (cache_tag[0][i].valid)) {
                              
                tmp_data = cache_data[0][i];
                tmp_tag = cache_tag[0][i];
                iout.hit = true;
                j = i;

            }

		}
		
		for (i = ICACHE_WAYS - 1; i > 0; i--) {
			if (iout.hit && i <= j) {
				cache_data[0][i] = cache_data[0][i-1];
				cache_tag[0][i] = cache_tag[0][i-1];
			}
				
		}

        if (iout.hit) {
			
            cache_data[0][0] = tmp_data;
            cache_tag[0][0] = tmp_tag;
		}else {
			tmp_data = cache_data[0][ICACHE_WAYS - 1];
		}

        iout.data = tmp_data.data;

        return iout;
    }
    
    void icache_write () {
			
        sc_uint < ICACHE_INDEX_WIDTH > write_index = icache_buffer_addr[1][0].range(ICACHE_INDEX_WIDTH, 1);
        int i = 0;
        for (i = 0; i < ICACHE_WAYS; i++) {                 

            if (hit_buffer) {
				icache_buffer_instr[0][i] = icache_buffer_instr[1][i];
				icache_buffer_addr[0][i] = icache_buffer_addr[1][i];
			}
				icache_data[write_index][i].data = icache_buffer_instr[1][i];
				icache_tags[write_index][i].valid = icache_buffer_addr[1][i].range(0,0);
				icache_tags[write_index][i].tag = icache_buffer_addr[1][i].range(ICACHE_INDEX_WIDTH + ICACHE_TAG_WIDTH, 1 + ICACHE_INDEX_WIDTH);
        }
                      

    }
};

#endif
