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
    Connections::In < imem_out_t > CCS_INIT_S1(imem_dout);
    Connections::Out < imem_in_t > CCS_INIT_S1(imem_din);
    Connections::Out < fe_out_t > CCS_INIT_S1(dout);
    Connections::Out < imem_out_t > CCS_INIT_S1(imem_de);

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
    
    ac_int < PC_LEN, false > redirect_addr;
	ac_int < PC_LEN, false > redirect_addr_tmp;
	
    bool freeze;
	bool freeze_tmp;
	int position;
    SC_CTOR(fetch): imem_din("imem_din"),
    fetch_din("fetch_din"),
    dout("dout"),
    imem_dout("imem_dout"),
    imem_de("imem_de"),
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
            imem_de.Reset();
									
            trap = 0;
            trap_cause = NULL_CAUSE;
            imem_in.instr_addr = 0;
            
            redirect_addr = 0;
			freeze = false;
			redirect = false;
            //  Init. pc to START_ADDRESS - 4 as on first fetch it will be incremented by
            //  4, thus fetching instruction at address 0
            pc = -4;
            pc_tmp = -4;
            position = 0;
            
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
			
            imem_in.instr_addr = pc;

            fe_out.pc = pc;

			imem_din.Push(imem_in);

            imem_out = imem_dout.Pop();

            imem_de.Push(imem_out);
            dout.Push(fe_out);
			
			#ifndef __SYNTHESIS__
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "pc= " << pc << endl);
            DPRINT(endl);
            #endif
            wait();

        } // *** ENDOF while(true)
    } // *** ENDOF sc_cthread
};

#endif
