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
#include <ac_int.h>

SC_MODULE(fetch) {
    public:
    // Clock and reset signals
    sc_in < bool > CCS_INIT_S1(clk);
    sc_in < bool > CCS_INIT_S1(rst);
    // Channel ports
    Connections::In < fe_in_t > CCS_INIT_S1(fetch_din);
    //#pragma hls_direct_input
    //sc_in < fe_in_t > CCS_INIT_S1(fetch_din);
    Connections::In < imem_out_t > CCS_INIT_S1(imem_dout);
    Connections::Out < imem_in_t > CCS_INIT_S1(imem_din);
    Connections::Out < fe_out_t > CCS_INIT_S1(dout);

    // Trap signals. TODO: not used. Left for future implementations.
    sc_signal < bool > CCS_INIT_S1(trap); //sc_out
    sc_signal < ac_int < LOG2_NUM_CAUSES, false > > CCS_INIT_S1(trap_cause); //sc_out

    // *** Internal variables
    ac_int < PC_LEN, true > pc; // Init. to -4, then before first insn fetch it will be updated to 0.	 
    ac_int < PC_LEN, false > imem_pc; // Used in fetching from instruction memory
	ac_int < PC_LEN, false > pc_tmp; // Init. to -4, then before first insn fetch it will be updated to 0.	 
    // Custom datatypes used for retrieving and sending data through the channels
    imem_in_t imem_in; // Contains data for fetching from the instruction memory
    fe_out_t fe_out; // Contains data for the decode stage
    fe_in_t fetch_in; // Contains data from the decode stage used in incrementing the PC
    imem_out_t imem_out;
		
    bool redirect;
    bool redirect_tmp;
    
    ras_data_t ra_stack[RAS_ENTRIES];
    ac_int < RAS_POINTER_SIZE, false > ras_pointer;
    ac_int < RAS_POINTER_SIZE, false > tosp_pointer;
    
    ac_int < PC_LEN, false > mispredictions;
    ac_int < PC_LEN, false > correct_predictions;
    ac_int < PC_LEN, false > redirect_addr;
	
	ac_int < DATA_SIZE, false > mem_dout;
    ac_int < ICACHE_LINE, false > imem_data;
    ac_int < XLEN, false > imem_data_offset;
    
    ac_int < ICACHE_TAG_WIDTH + ICACHE_INDEX_WIDTH + 1, false > icache_buffer_addr[ICACHE_BUFFER_SIZE][ICACHE_WAYS];
    ac_int < ICACHE_LINE, false > icache_buffer_instr[ICACHE_BUFFER_SIZE][ICACHE_WAYS];
    
    icache_data_t icache_data[ICACHE_ENTRIES][ICACHE_WAYS];
    icache_tag_t icache_tags[ICACHE_ENTRIES][ICACHE_WAYS];
    icache_out_t icache_out;
    
    icache_data_t cache_data[1][ICACHE_WAYS];
    icache_tag_t cache_tag[1][ICACHE_WAYS];
    
    btb_data_t btb_data[BTB_ENTRIES];
    btb_out_t btb_out;

    ac_int < ICACHE_TAG_WIDTH, false > tag;
    ac_int < ICACHE_INDEX_WIDTH, false > index;
    ac_int < ICACHE_OFFSET_WIDTH + 1, false> offset;
    
    ac_int < ICACHE_TAG_WIDTH + ICACHE_INDEX_WIDTH, false> buffer_addr;
	
    bool freeze;
    bool hit_buffer;
    int position;
	
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
            position = 0;
									
            trap = 0;
            trap_cause = NULL_CAUSE;
            imem_in.instr_addr = 0;
            
			ras_pointer = 0;
			tosp_pointer = 0;
            
            redirect_addr = 0;
			freeze = false;
			redirect = false;
            //  Init. pc to START_ADDRESS - 4 as on first fetch it will be incremented by
            //  4, thus fetching instruction at address 0
            pc = 0;
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
			
			mispredictions = 0;
			correct_predictions = 0;
            
            wait();
        }
        #pragma hls_pipeline_init_interval 1
        #pragma pipeline_stall_mode flush
        FETCH_BODY: while (true) {
            //sc_assert(sc_time_stamp().to_double() < 1500000);
			
			// step 1 fetch from memory/cache
            hit_buffer = false;
            fe_out.pc = pc;
            
            unsigned int aligned_addr = pc >> 2;
            imem_in.instr_addr = aligned_addr;
            
            ac_int < XLEN, false > addr = aligned_addr;
            
            tag = addr.slc<ICACHE_TAG_WIDTH>(ICACHE_INDEX_WIDTH + ICACHE_OFFSET_WIDTH);
            index = addr.slc<ICACHE_INDEX_WIDTH>(ICACHE_OFFSET_WIDTH);        
			if (ICACHE_OFFSET_WIDTH) {
                offset = addr.slc<ICACHE_OFFSET_WIDTH>(0);
            }
            else {
				offset = 0;
			}
			
			buffer_addr.set_slc(0, index);
			buffer_addr.set_slc(ICACHE_INDEX_WIDTH, tag);
			
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
				icache_buffer_addr[0][m].set_slc(0, (ac_int <1, false>) cache_tag[0][m].valid);
				icache_buffer_addr[0][m].set_slc(1, index);
				icache_buffer_addr[0][m].set_slc(ICACHE_INDEX_WIDTH + 1, cache_tag[0][m].tag);  
			}
			
			if (!icache_out.hit) {
				for (n = 0; n < ICACHE_BUFFER_SIZE; n++) {
					for (m = 0; m < ICACHE_WAYS; m++) {                 
						if (icache_buffer_addr[n][m].slc<ICACHE_TAG_WIDTH + ICACHE_INDEX_WIDTH>(1) == buffer_addr && icache_buffer_addr[n][m].slc<1>(0) == 1) {
							icache_out.data = icache_buffer_instr[n][m];
							icache_out.hit = true;
							hit_buffer = true;
						}
					}
				}
			}
			
            switch (icache_out.hit)
            {
				case CACHE_HIT:
                    imem_data = icache_out.data;
                    
                    imem_data_offset = imem_data.slc<DATA_WIDTH>(offset*DATA_WIDTH);
                    
                    fe_out.instr_data = imem_data_offset;
                    break;
                case CACHE_MISS:
				                    
                    imem_din.Push(imem_in);

					imem_out = imem_dout.Pop();
					
                    imem_data = imem_out.instr_data;
					imem_data_offset = imem_data.slc<DATA_WIDTH>(offset*DATA_WIDTH);
					fe_out.instr_data = imem_data_offset;
					
					icache_buffer_addr[0][ICACHE_WAYS - 1].set_slc(0, (ac_int <1, false>) 1);
					icache_buffer_addr[0][ICACHE_WAYS - 1].set_slc(1, buffer_addr);
					icache_buffer_instr[0][ICACHE_WAYS - 1] = imem_data;
                    
                    break;
                default:
                    break;
            }
            
			icache_write();
			
			//step2 read from backchannel (decode)
			if (position == 1 && !redirect) {
				fetch_in = fetch_din.Pop();
				redirect_addr = fetch_in.address;
			}else {
				position = 1;
			}
			// step3 if instruction correct send it, update btb, ras and get new pc
			if (redirect_addr == pc) {
				btb_write();
				ras_write();				
				btb();
				ras();
				pc = (btb_out.btb_valid || btb_out.ras_valid) ? btb_out.bta : (ac_int < PC_LEN, false >)(pc + 4);
				redirect = false;
				dout.Push(fe_out);
			}else { // step4 if instruction incorrect, redirect
				pc = redirect_addr;
				redirect = true;
			}
			
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
			
        ac_int < ICACHE_INDEX_WIDTH, false > write_index = icache_buffer_addr[1][0].slc<ICACHE_INDEX_WIDTH>(1);
        int i = 0;
        for (i = 0; i < ICACHE_WAYS; i++) {                 

            if (hit_buffer) {
				icache_buffer_instr[0][i] = icache_buffer_instr[1][i];
				icache_buffer_addr[0][i] = icache_buffer_addr[1][i];
			}
				icache_data[write_index][i].data = icache_buffer_instr[1][i];
				icache_tags[write_index][i].valid = icache_buffer_addr[1][i].slc<1>(0);
				icache_tags[write_index][i].tag = icache_buffer_addr[1][i].slc<ICACHE_TAG_WIDTH>(1 + ICACHE_INDEX_WIDTH);
        }
                      

    }
    
    void btb () {     
        ac_int < BTB_INDEX_WIDTH, false > next_index = pc.slc<BTB_INDEX_WIDTH>(0);
		ac_int < BTB_TAG_WIDTH, false > next_tag = pc.slc<BTB_TAG_WIDTH>(BTB_INDEX_WIDTH);
        
        if(next_tag == btb_data[next_index].tag && btb_data[next_index].prediction_data > WEAK_NON_TAKEN) {
            btb_out.bta = btb_data[next_index].bta;
            btb_out.btb_valid = true;
        }
        else {
            btb_out.btb_valid = false;
        }
    }
    
    void btb_write () {
		ac_int < PC_LEN, false > update_pc = fetch_in.pc;
		ac_int < BTB_INDEX_WIDTH, false > index = update_pc.slc<BTB_INDEX_WIDTH>(0);
		ac_int < BTB_TAG_WIDTH, false > tag = update_pc.slc<BTB_TAG_WIDTH>(BTB_INDEX_WIDTH);
		
		btb_data_t data = btb_data[index];
        //check if tag or branch target address differ in branch target buffer
        if (fetch_in.btb_update) {
            if(tag != data.tag || fetch_in.bta != data.bta) {
                btb_data[index].tag = tag;
                btb_data[index].bta = fetch_in.bta;
                btb_data[index].prediction_data = WEAK_NON_TAKEN;
                if (fetch_in.branch_taken) {
					mispredictions++;
				}    
                
            }else if (fetch_in.branch_taken && btb_data[index].prediction_data < STRONG_TAKEN){
                btb_data[index].prediction_data = btb_data[index].prediction_data + 1;
                if(btb_data[index].prediction_data > WEAK_NON_TAKEN + 1) {
					correct_predictions++;
				}else {
					mispredictions++;
				}
            }else if (!fetch_in.branch_taken && btb_data[index].prediction_data > 0) {
                btb_data[index].prediction_data = btb_data[index].prediction_data - 1;
                if(btb_data[index].prediction_data > WEAK_NON_TAKEN-1) {
					mispredictions++;
				}else {
					correct_predictions++;
				}
                
            }else {
				correct_predictions++;
			}
        }
	}
	
	void ras() {
		
		if (imem_data_offset.slc<5>(2) == OPC_JALR && ra_stack[ras_pointer].valid) {
			btb_out.ras_valid = true;
			btb_out.bta = ra_stack[ras_pointer].pc;
			ra_stack[ras_pointer].valid = false;
			tosp_pointer = tosp_pointer - 1;
			ras_pointer = tosp_pointer - 1;
		}else {
			btb_out.ras_valid = false;
		}
	}
	
	void ras_write() {
		if (fetch_in.ras_update) {
			ra_stack[tosp_pointer].pc = fetch_in.pc + 4;
			ra_stack[tosp_pointer].valid = true;
			tosp_pointer = tosp_pointer + 1;
			ras_pointer = tosp_pointer - 1;
		}
	}
};

#endif
