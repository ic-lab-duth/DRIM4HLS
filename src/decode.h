/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief Header file for decode stage

	@note Changes from HL5
		- Implements the logic only for the decode part from fedec.hpp.

		- Use of HLSLibs connections for communication with the rest of the processor.

		- Stall mechanism manages data dependencies, dynamic load/write memory stalls
		  and change of program direction.


*/

#ifndef __DEC__H
#define __DEC__H

#ifndef NDEBUG
    #include <iostream>
    #define DPRINT(msg) std::cout << msg;
#endif

#include "drim4hls_datatypes.h"
#include "defines.h"
#include "globals.h"

#include <mc_connections.h>

SC_MODULE(decode) {
    public:
    // Clock and reset signals
    sc_in < bool > CCS_INIT_S1(clk);
    sc_in < bool > CCS_INIT_S1(rst);
    // FlexChannel initiators
    Connections::Out < de_out_t > CCS_INIT_S1(dout);
    Connections::Out < fe_in_t > CCS_INIT_S1(fetch_dout);

    Connections::In < mem_out_t > CCS_INIT_S1(feed_from_wb);
    Connections::In < imem_out_t > CCS_INIT_S1(imem_out);
    Connections::In < fe_out_t > CCS_INIT_S1(fetch_din);
    Connections::In < reg_forward_t > CCS_INIT_S1(fwd_exe);
    
    // End of simulation signal.
    sc_out < bool > CCS_INIT_S1(program_end);

    // Instruction counters
    sc_out < long int > CCS_INIT_S1(icount);
    sc_out < long int > CCS_INIT_S1(j_icount);
    sc_out < long int > CCS_INIT_S1(b_icount);
    sc_out < long int > CCS_INIT_S1(m_icount);
    sc_out < long int > CCS_INIT_S1(o_icount);
    
    bool jump;
    bool branch;
    // Trap signals. TODO: not used. Left for future implementations.
    sc_signal < bool > CCS_INIT_S1(trap); //sc_out
    sc_signal < sc_uint < LOG2_NUM_CAUSES > > CCS_INIT_S1(trap_cause); //sc_out

    bool freeze;
    // Flushes current instruction in order to sychronize processor with a
    // change of direction in the execution
    bool flush;
	
    bool forward_success_rs1;
    bool forward_success_rs2;

    bool load_instruction;
    sc_int < PC_LEN > load_pc;

    sc_bv < INSN_LEN > insn; // Contains full instruction fetched from IMEM. Used in decoding.
    sc_int < PC_LEN > pc; // Contains PC for the current instruction that is decoded   
    // NB. x0 is included in this regfile so it is not a real hardcoded 0
    // constant. The writeback section of fedec has a guard fro writes on
    // x0. For double protection, some instructions that want to write into
    // x0 will have their regwrite signal forced to false.
    sc_bv < XLEN > regfile[REG_NUM];
    // Keeps track of in-flight instructions that are going to overwrite a
    // register. Implements a primitive stall mechanism for RAW hazards.
    sc_bv < XLEN + 1 > sentinel[REG_NUM];

    sc_uint < TAG_WIDTH > tag;
    // Stalls processor and sends a nop operation to the execute stage
    sc_uint < OPCODE_SIZE > opcode;

    int position;
    // Member variables (DECODE)
    de_in_t self_feed; // Contains branch and jump data		 
    imem_out_t imem_din; // Contains data from instruction memory
    mem_out_t feedinput; // Contains data from writeback stage
    mem_out_t feedinput_tmp; // Contains data from writeback stage
    de_out_t output; // Contains data for the execute stage
    fe_out_t input; // Contains data from the fetch stage
    fe_in_t fetch_out; // Contains data for the fetch stage about processor stalls

    reg_forward_t fwd;
    reg_forward_t temp_fwd;

    fe_out_t fetch_in; // Buffer for the data coming from the fetch stage
    imem_out_t imem_in;

    unsigned int imem_data; // Contains instruction data
   
	bool freeze_tmp;
	bool flush_tmp;
	sc_bv< 32 > addr_tmp;
	sc_bv< 5 > zero_reg_addr;
     
    bool flush_next;

    SC_CTOR(decode): clk("clk"),
    rst("rst"),
    dout("dout"),
    feed_from_wb("feed_from_wb"),
    fetch_din("fetch_din"),
    fetch_dout("fetch_dout"),
    program_end("program_end"),
    fwd_exe("fwd_exe"),
    icount("icount"),
    j_icount("j_icount"),
    b_icount("b_icount"),
    m_icount("m_icount"),
    o_icount("o_icount"),
    imem_out("imem_out") {
        
        SC_THREAD(decode_th);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

    }

    #ifndef __SYNTHESIS__
    //for debugging purposes
    struct debug_dout { //
        // Member declarations.
        //
        std::string regwrite;
        std::string memtoreg;
        std::string ld;
        std::string st;
        std::string alu_op;
        std::string alu_src;
        bool rs1_forward;
        bool rs2_forward;
        bool branch_taken;
        sc_bv < XLEN > rs1;
        sc_bv < XLEN > rs2;
        std::string dest_reg;
        int pc;
        int aligned_pc;
        sc_bv < XLEN - 12 > imm_u;
        sc_uint < TAG_WIDTH > tag;

    }
    debug_dout_t;
    #endif

    void decode_th(void) {
        DECODE_RST: {
            dout.Reset();
            fetch_din.Reset();
            feed_from_wb.Reset();
            fetch_dout.Reset();
            imem_out.Reset();
            fwd_exe.Reset();
			
            // Init. sentinel flags to zero.
            for (int i = 0; i < REG_NUM; i++) {
                sentinel[i] = SENTINEL_INIT;
            }

            // Program has not completed
            program_end.write(false);
            icount.write(0); // any
            j_icount.write(0); // jump
            b_icount.write(0); // branch
            m_icount.write(0); // load, store
            o_icount.write(0); // other
            
            addr_tmp = sc_bv < PC_LEN > ("0");
            self_feed.jump_address = sc_bv < PC_LEN > ("0");
            zero_reg_addr = sc_bv < 5 >("0");

            freeze = false;
            flush = false;
	        flush_next = false;
            freeze_tmp = false;
            flush_tmp = false;

            forward_success_rs1 = false;
            forward_success_rs2 = false;
            position = 0;
            insn = 0;
            branch = false;
            jump = false;
            pc = -4;
            wait();
        }
        
        #pragma hls_pipeline_init_interval 1
        #pragma pipeline_stall_mode flush
        DECODE_BODY: while (true) {
            // Retrieve data from instruction memory and fetch stage.
            // If processor stalls then just clear the channels from new data.

            if (fwd_exe.PopNB(temp_fwd)) {
                fwd = temp_fwd;
                
            }else {
				fwd.ldst = true;
			}

            if (!flush) {

                fetch_in = fetch_din.Pop();
                imem_in = imem_out.Pop();

            } else {
                imem_out.Pop();
                fetch_din.Pop();
            }

            if (feed_from_wb.PopNB(feedinput_tmp)) {
				feedinput = feedinput_tmp;

                if (feedinput_tmp.pc == load_pc && load_instruction) {
                    load_instruction = false;
                }
            }else {
				feedinput.regwrite = "0";
			}
            
            if (feedinput.regwrite[0] == "1" && (sc_uint < 5 >) feedinput.regfile_address != 0) { // Actual writeback.
                    regfile[sc_uint < REG_ADDR > (feedinput.regfile_address)] = feedinput.regfile_data; // Overwrite register.

				if ((feedinput.pc == sentinel[sc_uint < REG_ADDR > (feedinput.regfile_address)].range(32, 1)) && (sentinel[sc_uint < REG_ADDR > (feedinput.regfile_address)][0] == "1")) {
					sentinel[sc_uint < REG_ADDR > (feedinput.regfile_address)][0] = 0;
				}

            }
          
            flush_next = false;
            if (!freeze && (((jump) && self_feed.jump_address != fetch_in.pc) || ((branch) && self_feed.branch_address != fetch_in.pc) || (fetch_in.pc != pc + 4 && !branch && !jump))) {
				flush_next = true;
			}else if (!freeze) {
				pc = fetch_in.pc;

			    imem_din = imem_in;
			    imem_data = imem_din.instr_data;
			    
			    forward_success_rs1 = false;
                forward_success_rs2 = false;
			}

            insn = imem_data;

            #ifndef __SYNTHESIS__
            debug_dout_t.pc = pc;
            #endif

            output.pc = pc;

            // Increment some instruction counters
            opcode = sc_uint < OPCODE_SIZE > (sc_bv < OPCODE_SIZE > (insn.range(6, 2)));
            if (!freeze) {

                if (opcode == OPC_LW || opcode == OPC_SW)
                    // Increment memory instruction counter
                    m_icount.write(m_icount.read() + 1);
                else if (opcode == OPC_JAL || opcode == OPC_JALR) {
                    // Increment jump instruction counter
                    j_icount.write(j_icount.read() + 1);
                } else if (opcode == OPC_BEQ) {
                    // Increment branch instruction counter
                    b_icount.write(b_icount.read() + 1);
                } else
                    // Increment other instruction counter
                    o_icount.write(o_icount.read() + 1);

                icount.write(icount.read() + 1);
            }

            fetch_out.freeze = false;
            fetch_out.redirect = false;
            
            freeze_tmp = false;
            flush_tmp = false;

           if (insn == 0x0000006f) {
                // jump to yourself (end of program).
                program_end.write(true);
            }

            sc_uint < REG_ADDR > rs1_addr = ( sc_bv < REG_ADDR > ) insn.range(19, 15);
            sc_uint < REG_ADDR > rs2_addr = ( sc_bv < REG_ADDR > ) insn.range(24, 20);
			
			
			sc_bv < 32 > rs1_sent_pc = sentinel[rs1_addr].range(32, 1);
			sc_bv < 1 > rs1_sent_valid = sentinel[rs1_addr].range(0, 0);
            
            if (!fwd.ldst && fwd.pc == rs1_sent_pc && rs1_sent_valid[0] == "1") {
                forward_success_rs1 = true;
                output.rs1 = fwd.regfile_data;

                #ifndef __SYNTHESIS__
                debug_dout_t.rs1 = fwd.regfile_data;
                debug_dout_t.rs1_forward = forward_success_rs1;
                #endif
            } else {
        
                output.rs1 = regfile[rs1_addr];

                #ifndef __SYNTHESIS__
                debug_dout_t.rs1 = regfile[rs1_addr];
                debug_dout_t.rs1_forward = forward_success_rs1;
                #endif

                
            }

            sc_bv < 32 > rs2_sent_pc = sentinel[rs2_addr].range(32, 1);
			sc_bv < 1 > rs2_sent_valid = sentinel[rs2_addr].range(0,0);
			
            if (!fwd.ldst && fwd.pc == rs2_sent_pc && rs2_sent_valid[0] == "1") {
                forward_success_rs2 = true;
                output.rs2 = fwd.regfile_data;

                #ifndef __SYNTHESIS__
                debug_dout_t.rs2 = fwd.regfile_data;
                debug_dout_t.rs2_forward = forward_success_rs2;
                #endif

            } else {
                
                output.rs2 = regfile[rs2_addr];

                #ifndef __SYNTHESIS__
                debug_dout_t.rs2 = regfile[rs2_addr];
                debug_dout_t.rs2_forward = forward_success_rs2;
                #endif

            
            }
        
			
            // *** Feedback to fetch data computation and put() section.
            // -- Address sign extensions.
            sc_bv < 21 > immjal_tmp = ((sc_bv < 1 > ) insn.range(31, 31), (sc_bv < 8 > ) insn.range(19, 12), (sc_bv < 1 > ) insn.range(20, 20), (sc_bv < 10 > ) insn.range(30, 21), (sc_bv < 1 > )(0));
            sc_bv < 13 > immbranch_tmp = ((sc_bv < 1 > ) insn.range(31, 31), (sc_bv < 1 > ) insn.range(7, 7), (sc_bv < 6 > ) insn.range(30, 25), (sc_bv < 4 > ) insn.range(11, 8), (sc_bv < 1 > )(0));
            self_feed.branch_address = sc_bv < PC_LEN > ((sc_int < PC_LEN > ) sign_extend_branch(immbranch_tmp) + (sc_int < PC_LEN > ) pc);
            // -- Jump.
            if (insn.range(6, 2) == OPC_JAL) {
                self_feed.jump_address = sc_bv < PC_LEN > ((sc_int < PC_LEN > ) sign_extend_jump(immjal_tmp) + (sc_int < PC_LEN > ) pc);
                jump = true;
            } else if (insn.range(6, 2) == OPC_JALR) {
                sc_bv < PC_LEN > extended;
                if (insn[31] == 0)
                    extended = 0;
                else
                    extended = 4294967295;

                extended.range(11, 0) = insn.range(31, 20);
                self_feed.jump_address = sc_bv < PC_LEN > ((sc_int < PC_LEN > ) extended + (sc_int < PC_LEN > ) output.rs1);
                self_feed.jump_address.range(0, 0) = "0";
                jump = true;
            } else {
                jump = false;
            }

            // -- Branch circuitry.
            branch = false;
            if (insn.range(6, 2) == OPC_BEQ) { // BEQ,BNE, BLT, BGE, BLTU, BGEU
                switch (sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12)))) {
                case FUNCT3_BEQ:
                    if (output.rs1 == output.rs2)
						branch = true; // BEQ taken.
                    #ifndef __SYNTHESIS__
                    debug_dout_t.branch_taken = true;
                    #endif

                    break;
                case FUNCT3_BNE:
                    if ((sc_int < XLEN > ) output.rs1 != (sc_int < XLEN > ) output.rs2) {
						branch = true; //BNE taken.
                        #ifndef __SYNTHESIS__
                        debug_dout_t.branch_taken = true;
                        #endif
                    }
                    break;
                case FUNCT3_BLT:
                    if ((sc_int < XLEN > ) output.rs1 < (sc_int < XLEN > ) output.rs2) {
						branch = true; // BLT taken
                        #ifndef __SYNTHESIS__
                        debug_dout_t.branch_taken = true;
                        #endif
                    }
                    break;
                case FUNCT3_BGE:
                    if ((sc_int < XLEN > ) output.rs1 >= (sc_int < XLEN > ) output.rs2) {
						branch = true; // BGE taken.
                        #ifndef __SYNTHESIS__
                        debug_dout_t.branch_taken = true;
                        #endif
                    }
                    break;
                case FUNCT3_BLTU:
                    if ((sc_uint < XLEN > ) output.rs1 < (sc_uint < XLEN > ) output.rs2) {
						branch = true; // BLTU taken.
                        #ifndef __SYNTHESIS__
                        debug_dout_t.branch_taken = true;
                        #endif
                    }
                    break;
                case FUNCT3_BGEU:
                    if ((sc_uint < XLEN > ) output.rs1 >= (sc_uint < XLEN > ) output.rs2) {
						branch = true; // BGEU taken.
                        #ifndef __SYNTHESIS__
                        debug_dout_t.branch_taken = true;
                        #endif
                    }
                    break;
                default:
                    branch = false; // default to not taken.
                    #ifndef __SYNTHESIS__
                    debug_dout_t.branch_taken = false;
                    #endif
                    break;
                }
            }
            // -- All data for feedback path to fetch is ready now. Do put(): in this version it saved data in self_feed.
            // *** END of feedback to fetch data computation and put() section.

            // *** Propagations: rd, immediates sign extensions.
            output.dest_reg = insn.range(11, 7);
            // RD field of insn.
            output.imm_u = insn.range(31, 12); // This field is then used in the execute stage not only as immU field but to obtain several subfields used by non U-type instructions.

            #ifndef __SYNTHESIS__
            debug_dout_t.dest_reg = std::to_string(insn.range(11, 7).to_int());
            debug_dout_t.imm_u = insn.range(31, 12);
            #endif
            // *** END of RD propagation and immediates sign extensions.

            // *** Control word generation.
            switch (sc_uint < OPCODE_SIZE > (sc_bv < OPCODE_SIZE > (insn.range(6, 2)))) { // Opcode's 2 LSBs have been trimmed to save area.

            case OPC_LUI:
                output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_LUI;
                output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_IMM_U;
                output.regwrite = "1";
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = "0";
                trap = "0";
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_op = "ALUOP_LUI";
                debug_dout_t.alu_src = "ALUSRC_IMM_U";
                debug_dout_t.regwrite = "REGWRITE YES";
                debug_dout_t.ld = "NO LOAD";
                debug_dout_t.st = "NO STORE";
                debug_dout_t.memtoreg = "MEMTOREG YES";
                #endif
                break;

            case OPC_AUIPC:
                output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_AUIPC;
                output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_IMM_U;
                output.regwrite = "1";
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = "0";
                trap = "0";
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_op = "ALUOP_AUIPC";
                debug_dout_t.alu_src = "ALUSRC_IMM_U";
                debug_dout_t.regwrite = "REGWRITE YES";
                debug_dout_t.ld = "NO LOAD";
                debug_dout_t.st = "NO STORE";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                #endif
                break;

            case OPC_JAL:
                output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_JAL;
                output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_RS2; // Actually does not use RS2 as it performs "rd = pc + 4"
                output.regwrite = "1";
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = "0";
                trap = "0";
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_op = "ALUOP_JAL";
                debug_dout_t.alu_src = "ALUSRC_RS2";
                debug_dout_t.regwrite = "REGWRITE YES";
                debug_dout_t.ld = "NO LOAD";
                debug_dout_t.st = "NO STORE";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                #endif
                break;

            case OPC_JALR: // same as JAL, could optimize
                output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_JALR;
                output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_RS2; // Actually does not use RS2 as it performs "rd = pc + 4"
                output.regwrite = "1";
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = "0";
                trap = "0";
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_op = "ALUOP_JALR";
                debug_dout_t.alu_src = "ALUSRC_RS2";
                debug_dout_t.regwrite = "REGWRITE YES";
                debug_dout_t.ld = "NO LOAD";
                debug_dout_t.st = "NO STORE";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                #endif
                break;

            case OPC_BEQ: // Branch instructions: BEQ, BNE, BLT, BGE, BLTU, BGEU
                output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_NULL;
                output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_RS2;
                output.regwrite = "0";
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = "0";
                trap = "0";
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_op = "ALUOP_BEQ";
                debug_dout_t.alu_src = "ALUSRC_RS2";
                debug_dout_t.regwrite = "REGWRITE NO";
                debug_dout_t.ld = "NO LOAD";
                debug_dout_t.st = "NO STORE";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                #endif
                break;

            case OPC_LW:
                switch (sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12)))) {
                case FUNCT3_LB:
                    output.ld = LB_LOAD;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.ld = "LB_LOAD";
                    #endif
                    break;
                case FUNCT3_LH:
                    output.ld = LH_LOAD;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.ld = "LH_LOAD";
                    #endif
                    break;
                case FUNCT3_LW:
                    output.ld = LW_LOAD;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.ld = "LW_LOAD";
                    #endif
                    break;
                case FUNCT3_LBU:
                    output.ld = LBU_LOAD;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.ld = "LBU_LOAD";
                    #endif
                    break;
                case FUNCT3_LHU:
                    output.ld = LHU_LOAD;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.ld = "LHU_LOAD";
                    #endif
                    break;
                default:
                    output.ld = NO_LOAD;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.ld = "NO_LOAD";
                    #endif
                    SC_REPORT_ERROR(sc_object::name(), "Unimplemented LOAD instruction");
                    break;
                }
                output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_ADD;
                output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_IMM_I;
                output.regwrite = "1";
                output.st = NO_STORE;
                output.memtoreg = "1";
                trap = "0";
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_op = "ALUOP_ADD";
                debug_dout_t.alu_src = "ALUSRC_IMM_I";
                debug_dout_t.regwrite = "REGWRITE YES";
                debug_dout_t.st = "NO_STORE";
                debug_dout_t.memtoreg = "MEMTOREG YES";
                #endif
                break;

            case OPC_SW:
                switch (sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12)))) {
                case FUNCT3_SB:
                    output.st = SB_STORE;
                    #ifndef __SYNTHESIS__
                    debug_dout_t.st = "SB_STORE";
                    #endif
                    break;
                case FUNCT3_SH:
                    output.st = SH_STORE;
                    #ifndef __SYNTHESIS__
                    debug_dout_t.st = "SH_STORE";
                    #endif
                    break;
                case FUNCT3_SW:
                    output.st = SW_STORE;
                    #ifndef __SYNTHESIS__
                    debug_dout_t.st = "SW_STORE";
                    #endif
                    break;
                default:
                    output.st = NO_STORE;
                    #ifndef __SYNTHESIS__
                    debug_dout_t.st = "NO_STORE";
                    #endif
                    SC_REPORT_ERROR(sc_object::name(), "Unimplemented STORE instruction");
                    break;
                }
                output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_ADD;
                output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_IMM_S;
                output.regwrite = "0";
                output.ld = NO_LOAD;
                output.memtoreg = "0";
                trap = "0";
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_op = "ALUOP_ADD";
                debug_dout_t.alu_src = "ALUSRC_IMM_S";
                debug_dout_t.regwrite = "REGWRITE NO";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                #endif
                break;

            case OPC_ADDI: // OP-IMM instructions (arithmetic and logical operations on immediates): ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI

                if (sc_uint < 7 > (sc_bv < 7 > (insn.range(31, 25))) == FUNCT7_SRAI && sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12))) == FUNCT3_SRAI) {
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SRAI;
                    output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_IMM_U;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_SRAI";
                    debug_dout_t.alu_src = "ALUSRC_IMM_U";
                    #endif
                } else if (sc_uint < 7 > (sc_bv < 7 > (insn.range(31, 25))) == FUNCT7_SLLI && sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12))) == FUNCT3_SLLI) {
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SLLI;
                    output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_IMM_U;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_SLLI";
                    debug_dout_t.alu_src = "ALUSRC_IMM_U";
                    #endif
                } else if (sc_uint < 7 > (sc_bv < 7 > (insn.range(31, 25))) == FUNCT7_SRLI && sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12))) == FUNCT3_SRLI) {
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SRLI;
                    output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_IMM_U;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_SRLI";
                    debug_dout_t.alu_src = "ALUSRC_IMM_U";
                    #endif
                } else {
                    output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_IMM_I;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_src = "ALUSRC_IMM_I";
                    #endif
                    switch (sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12)))) {
                    case FUNCT3_ADDI:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_ADDI;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_ADDI";
                        #endif
                        break;
                    case FUNCT3_SLTI:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SLTI;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SLTI";
                        #endif
                        break;
                    case FUNCT3_SLTIU:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SLTIU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SLTIU";
                        #endif
                        break;
                    case FUNCT3_XORI:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_XORI;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_XORI";
                        #endif
                        break;
                    case FUNCT3_ORI:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_ORI;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_ORI";
                        #endif
                        break;
                    case FUNCT3_ANDI:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_ANDI;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_ANDI";
                        #endif
                        break;
                    default:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_NULL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_NULL";
                        #endif
                        SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_IMM instruction");
                        break;
                    }
                }
                output.regwrite = "1";
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = "0";
                trap = "0";
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.regwrite = "REGWRITE YES";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.st = "NO_STORE";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                #endif
                break;

            case OPC_ADD: // R-type instructions: ADD, SLL, SLT, SLTU, XOR, SRL, OR, AND, SUB, SRA, MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU.
                output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_RS2;
                output.regwrite = "1";
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = "0";
                trap = "0";
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_src = "ALUSRC_RS2";
                debug_dout_t.regwrite = "REGWRITE YES";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.st = "NO_STORE";
                debug_dout_t.memtoreg = "REGWRITE NO";
                #endif
                // FUNCT7 switch discriminates between classes of R-type instructions.
                switch (sc_uint < 7 > (sc_bv < 7 > (insn.range(31, 25)))) {
                case FUNCT7_ADD: // ADD, SLL, SLT, SLTU, XOR, SRL, OR, AND
                    switch (sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12)))) {
                    case FUNCT3_ADD:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_ADD;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_ADD";
                        #endif
                        break;
                    case FUNCT3_SLL:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SLL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SLL";
                        #endif
                        break;
                    case FUNCT3_SLT:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SLT;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SLT";
                        #endif
                        break;
                    case FUNCT3_SLTU:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SLTU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SLTU";
                        #endif
                        break;
                    case FUNCT3_XOR:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_XOR;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_XOR";
                        #endif
                        break;
                    case FUNCT3_SRL:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SRL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SRL";
                        #endif
                        break;
                    case FUNCT3_OR:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_OR;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_OR";
                        #endif
                        break;
                    case FUNCT3_AND:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_AND;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_AND";
                        #endif
                        break;
                    default:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_NULL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_NULL";
                        #endif
                        SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_ADD instruction");
                        break;
                    }
                    break;
                case FUNCT7_SUB: // SUB, SRA
                    switch (sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12)))) {
                    case FUNCT3_SUB:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SUB;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SUB";
                        #endif
                        break;
                    case FUNCT3_SRA:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_SRA;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SRA";
                        #endif
                        break;
                    default:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_NULL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_NULL";
                        #endif
                        SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_SUB instruction");
                        break;
                    }
                    break;
                    #if defined(MUL32) || defined(MUL64) || defined(DIV) || defined(REM)
                case FUNCT7_MUL: // MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU
                    switch (sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12)))) {
                    case FUNCT3_MUL:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_MUL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_MUL";
                        #endif
                        break;
                    case FUNCT3_MULH:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_MULH;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_MULH";
                        #endif
                        break;
                    case FUNCT3_MULHSU:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_MULHSU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_MULHSU";
                        #endif
                        break;
                    case FUNCT3_MULHU:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_MULHU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_MULHU";
                        #endif
                        break;
                    case FUNCT3_DIV:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_DIV;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_DIV";
                        #endif
                        break;
                    case FUNCT3_DIVU:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_DIVU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_DIVU";
                        #endif
                        break;
                    case FUNCT3_REM:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_REM;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_REM";
                        #endif
                        break;
                    case FUNCT3_REMU:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_REMU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_REMU";
                        #endif
                        break;
                    default:
                        output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_NULL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_NULL";
                        #endif
                        SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_MUL instruction");
                        break;
                    }
                    break;
                    #endif
                default:
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_NULL;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_NULL";
                    #endif
                    SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP instruction");
                    break;
                }
                break;

                #ifdef CSR_LOGIC
            case OPC_SYSTEM:
                output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_NULL;
                output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_RS2;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = "0";
                output.regwrite = "1";

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_op = "ALUOP_NULL";
                debug_dout_t.alu_src = "ALUSRC_RS2";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.st = "NO_STORE";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                debug_dout_t.regwrite = "REGWRITE YES";
                #endif
                switch (sc_uint < 3 > (sc_bv < 3 > (insn.range(14, 12)))) {
                case FUNCT3_EBREAK: // EBREAK, ECALL
                    output.regwrite = "0";
                    trap = "1";
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_CSRRWI;
                    output.imm_u.range(19, 8) = (sc_bv < CSR_ADDR > ) MCAUSE_A; // force the CSR address to MCAUSE's

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRWI";
                    debug_dout_t.imm_u.range(19, 8) = (sc_bv < CSR_ADDR > ) MCAUSE_A;
                    #endif
                    if (sc_bv < 1 > (insn.range(20, 20)) == (sc_bv < 1 > ) FUNCT7_EBREAK) { // Bit 20 discriminates b/n EBREAK and ECALL
                        // EBREAK and ECALL leverage CSRRWI decoding to write into the MCAUSE register
                        // but keep regwrite to "0" to prevent writeback
                        trap_cause = EBREAK_CAUSE; // may be not necessary but is kept for future implementations
                        output.imm_u.range(7, 3) = (sc_bv < ZIMM_SIZE > ) EBREAK_CAUSE; // force the exception cause on the zimm field

                        #ifndef __SYNTHESIS__
                        debug_dout_t.imm_u.range(7, 3) = (sc_bv < ZIMM_SIZE > ) EBREAK_CAUSE;
                        #endif
                    } else { // FUNCT7_ECALL
                        trap_cause = ECALL_CAUSE; // may be not necessary but is kept for future implementations
                        output.imm_u.range(7, 3) = (sc_bv < ZIMM_SIZE > ) ECALL_CAUSE; // force the exception cause on the zimm field

                        #ifndef __SYNTHESIS__
                        debug_dout_t.imm_u.range(7, 3) = (sc_bv < ZIMM_SIZE > ) ECALL_CAUSE;
                        #endif
                    }
                    break;
                case FUNCT3_CSRRW:
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_CSRRW;
                    trap = "0";
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRW";
                    #endif
                    break;
                case FUNCT3_CSRRS:
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_CSRRS;
                    trap = "0";
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRS";
                    #endif
                    break;
                case FUNCT3_CSRRC:
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_CSRRC;
                    trap = "0";
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRC";
                    #endif
                    break;
                case FUNCT3_CSRRWI:
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_CSRRWI;
                    output.regwrite = "1";
                    trap = "0";
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRWI";
                    debug_dout_t.regwrite = "REGWRITE YES";
                    #endif
                    break;
                case FUNCT3_CSRRSI:
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_CSRRSI;
                    trap = "0";
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRSI";
                    #endif
                    break;
                case FUNCT3_CSRRCI:
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_CSRRCI;
                    trap = "0";
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRCI";
                    #endif
                    break;
                default:
                    output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_NULL;
                    trap = "0";
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_NULL";
                    #endif
                    SC_REPORT_ERROR(sc_object::name(), "Unimplemented SYSTEM instruction");
                    break;
                }
                break;
                #endif // --- End of System instructions decoding

            default: // illegal instruction
                output.alu_src = (sc_bv < ALUSRC_SIZE > ) ALUSRC_RS2;
                output.regwrite = "0";
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = "0";
                trap = "1";
                trap_cause = ILL_INSN_CAUSE;
                output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_CSRRWI;
                output.imm_u.range(19, 8) = (sc_bv < 12 > ) MCAUSE_A; // force the CSR address to MCAUSE's

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_src = "ALUSRC_RS2";
                debug_dout_t.regwrite = "REGWRITE NO";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.st = "NO_STORE";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                debug_dout_t.alu_op = "ALUOP_CSRRWI";
                debug_dout_t.imm_u.range(19, 8) = (sc_bv < 12 > ) MCAUSE_A;
                debug_dout_t.imm_u.range(7, 3) = (sc_bv < 5 > ) ILL_INSN_CAUSE;
                #endif
                
                SC_REPORT_ERROR(sc_object::name(), "Unimplemented instruction");
                break;
            } // --- END of OPCODE switch
            // *** END of control word generation.
        
            if ((sentinel[rs1_addr][0] == "1" && !forward_success_rs1) || // If RAW on RS1
                (sentinel[rs2_addr][0] == "1" && !forward_success_rs2) ||
                (load_instruction)) {
                freeze = true;
                fetch_out.freeze = true;
                flush = false;
                fetch_out.address = pc + 4;

            } else if(flush_next) {				
				fetch_out.freeze = false;
				fetch_out.redirect = false;
				
			} else if ((jump) && !flush && self_feed.jump_address != pc + 4) {
                freeze = true;
                fetch_out.freeze = false;
                flush = true;
                fetch_out.address = self_feed.jump_address;
                fetch_out.redirect = true;
                
            } else if ((branch) && !flush && self_feed.branch_address != pc + 4) {
                freeze = true;
                fetch_out.freeze = false;
                flush = true;
                fetch_out.address = self_feed.branch_address;
                fetch_out.redirect = true;
                
            } else {
                freeze = false;
                flush = false;
            }
			
            
            sc_uint < 1 > out_regwrite = output.regwrite;
            sc_bv < 33 > sen_input;
            
            if (!freeze && output.regwrite[0] == "1" && (sc_uint< 5 >)output.dest_reg != 0) {
                sentinel[sc_uint < REG_ADDR > (output.dest_reg)].range(32, 1) =  pc; // Set corresponding sentinel flag.
                sentinel[sc_uint < REG_ADDR > (output.dest_reg)].range(0, 0) = 1;

                if (sc_uint < REG_ADDR > (output.dest_reg) == rs1_addr) {
                    forward_success_rs1 = true;
                } else if (sc_uint < REG_ADDR > (output.dest_reg) == rs2_addr) {
                    forward_success_rs2 = true;
                }
            }

            // *** Transform instruction into nop when freeze is active
            if (freeze || insn == "0" || flush_next) {
                // Bubble.
                output.regwrite = "0";
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.alu_op = (sc_bv < ALUOP_SIZE > ) ALUOP_NULL;

                #ifndef __SYNTHESIS__
                debug_dout_t.regwrite = "REGWRITE NO";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.st = "NO_STORE";
                #endif
            }

            if (output.ld != NO_LOAD) {
                load_instruction = true;
                load_pc = pc;
            }
            
            fetch_dout.Push(fetch_out);
            dout.Push(output);

            #ifndef __SYNTHESIS__
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "load_instruction=" << load_instruction << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "insn=" << insn << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "freeze= " << freeze << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "flush= " << flush << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "pc= " << debug_dout_t.pc << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "regwrite= " << debug_dout_t.regwrite << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "memtoreg= " << debug_dout_t.memtoreg << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "ld= " << debug_dout_t.ld << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "st= " << debug_dout_t.st << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "alu_op= " << debug_dout_t.alu_op << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "alu_src= " << debug_dout_t.alu_src << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "rs1= " << debug_dout_t.rs1 << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "rs2= " << debug_dout_t.rs2 << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "dest_reg= " << debug_dout_t.dest_reg << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "imm_u= " << debug_dout_t.imm_u << endl);
            DPRINT(endl);

            for (int i = 0; i < REG_NUM;) {
                DPRINT(endl);
                for (int j = 0; j < 8; j++) {
                    int r = regfile[i].to_int();
                    DPRINT(" " << std::right << std::setfill(' ') << std::setw(2) << i << ": 0x" << std::hex << std::left << std::setfill(' ') << std::setw(10) << r << std::dec);
                    i++;
                    if (i == REG_NUM)
                        break;
                }
                DPRINT(endl);
                if (i == REG_NUM)
                    break;
            }
            #endif

            DPRINT(endl);
            wait();

        } // *** ENDOF while(true)
    } // *** ENDOF sc_cthread

    // --- Utility functions.

    // Sign extend UJ insn.
    sc_bv < PC_LEN > sign_extend_jump(sc_bv < 21 > imm) {
        if (imm[20] == "1") {
			sc_bv < 32 > ext_imm = 4294967295;
            ext_imm.range(20, 0) = imm;
            return ext_imm;
        }
        else {
			sc_bv < 32 > ext_imm = imm;
			return ext_imm;
		}
    }

    // Sign extend branch insn.
    sc_bv < PC_LEN > sign_extend_branch(sc_bv < 13 > imm) {
        
        if (imm[12] == "1") {
			sc_bv < 32 > ext_imm = 4294967295;
            ext_imm.range(12, 0) = imm;
            return ext_imm;
        }
        else {
			sc_bv < 32 > ext_imm = imm;
			return ext_imm;
		}
    }

    // --- End of utility functions.
};

#endif
