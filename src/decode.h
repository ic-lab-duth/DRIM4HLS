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
#include <ac_int.h>

SC_MODULE(decode) {
    public:
    // Clock and reset signals
    sc_in < bool > CCS_INIT_S1(clk);
    sc_in < bool > CCS_INIT_S1(rst);
    // FlexChannel initiators
    Connections::Out < de_out_t > CCS_INIT_S1(dout);
    Connections::Out < fe_in_t > CCS_INIT_S1(fetch_dout);
    Connections::In < mem_out_t > CCS_INIT_S1(feed_from_wb);
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
    sc_signal < ac_int < LOG2_NUM_CAUSES, false > > CCS_INIT_S1(trap_cause); //sc_out

    bool freeze;
    // Flushes current instruction in order to sychronize processor with a
    // change of direction in the execution
    bool flush;
	
    bool forward_success_rs1;
    bool forward_success_rs2;

    bool load_instruction;
    ac_int < PC_LEN, true > load_pc;

    ac_int < INSN_LEN, false > insn; // Contains full instruction fetched from IMEM. Used in decoding.
    ac_int < PC_LEN, true > pc; // Contains PC for the current instruction that is decoded   
    // NB. x0 is included in this regfile so it is not a real hardcoded 0
    // constant. The writeback section of fedec has a guard fro writes on
    // x0. For double protection, some instructions that want to write into
    // x0 will have their regwrite signal forced to false.
    ac_int < XLEN, false > regfile[REG_NUM];
    // Keeps track of in-flight instructions that are going to overwrite a
    // register. Implements a primitive stall mechanism for RAW hazards.
    ac_int < XLEN + 1, false > sentinel[REG_NUM];

    ac_int < TAG_WIDTH, false > tag;
    // Stalls processor and sends a nop operation to the execute stage
    sc_uint < OPCODE_SIZE > opcode;

    int position;
    // Member variables (DECODE)
    de_in_t self_feed; // Contains branch and jump data		 
    mem_out_t feedinput; // Contains data from writeback stage
    mem_out_t feedinput_tmp; // Contains data from writeback stage
    de_out_t output; // Contains data for the execute stage
    fe_out_t input; // Contains data from the fetch stage
    fe_in_t fetch_out; // Contains data for the fetch stage about processor stalls

    reg_forward_t fwd;
    reg_forward_t temp_fwd;

    fe_out_t fetch_in; // Buffer for the data coming from the fetch stage

    unsigned int imem_data; // Contains instruction data
   
	bool freeze_tmp;
	ac_int< 32, false > addr_tmp;
	ac_int< 5, false > zero_reg_addr;
     
    bool flush_next;
    bool new_instr;
    int position_fwd;
    int position_wb;
    
    ac_int < DCACHE_INDEX_WIDTH, false > last_ldst_addr;
    ac_int < DCACHE_INDEX_WIDTH, false > last_ldst_addr_temp;
    bool last_ldst_valid;
     
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
    o_icount("o_icount") {
        
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
        ac_int < XLEN, false > rs1;
        ac_int < XLEN, false > rs2;
        std::string dest_reg;
        int pc;
        int aligned_pc;
        ac_int < XLEN - 12, false > imm_u;
        ac_int < TAG_WIDTH, false > tag;

    }
    debug_dout_t;
    #endif

    void decode_th(void) {
        DECODE_RST: {
            dout.Reset();
            fetch_din.Reset();
            feed_from_wb.Reset();
            fetch_dout.Reset();
            fwd_exe.Reset();
			//fetch_dout.write(fetch_out);
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
            
            addr_tmp = 0;
            zero_reg_addr = 0;
			
            freeze = false;
            flush = false;
	        flush_next = false;
            freeze_tmp = false;

            forward_success_rs1 = false;
            forward_success_rs2 = false;
            position = 0;
            insn = 0;
            branch = false;
            jump = false;
            pc = -4;
            load_instruction = false;
            load_pc = -4;
            new_instr = false;
            position_fwd = 0;
            position_wb = 0;
            fetch_out.address = 0;

			last_ldst_valid = false;
			last_ldst_addr = 0;
			last_ldst_addr_temp = 0;
            
            wait();
        }
        
        #pragma hls_pipeline_init_interval 1
        #pragma pipeline_stall_mode flush
        DECODE_BODY: while (true) {
            // Retrieve data from instruction memory and fetch stage.
            // If processor stalls then just clear the channels from new data.
			
			if (!freeze) {
				fetch_in = fetch_din.Pop();
				pc = fetch_in.pc;
				imem_data = fetch_in.instr_data;
			}
			
    
            if (position_fwd == 1) {
				fwd = fwd_exe.Pop();
			}else {
				position_fwd = 1;
			}
			
			if (position_wb == 2) {
				feedinput = feed_from_wb.Pop();
				
				if (feedinput.pc == load_pc && load_instruction) {
                    load_instruction = false;
                }
			}else {
				position_wb++;
			}

            if (feedinput.regwrite == 1 && feedinput.regfile_address != 0) { // Actual writeback.
                    regfile[feedinput.regfile_address] = feedinput.regfile_data; // Overwrite register.

				if ((feedinput.pc == sentinel[feedinput.regfile_address].slc<32>(1)) && (sentinel[feedinput.regfile_address][0] == 1)) {
					sentinel[feedinput.regfile_address][0] = 0;
				}

            }

            flush_next = false;
            new_instr = false;
            
            insn = imem_data;
			    
			forward_success_rs1 = false;
            forward_success_rs2 = false;
                
            new_instr = true;

            #ifndef __SYNTHESIS__
            debug_dout_t.pc = pc;
            #endif

            output.pc = pc;
            fetch_out.pc = pc;
            // Increment some instruction counters
            //opcode = sc_uint < OPCODE_SIZE > (sc_bv < OPCODE_SIZE > (insn.range(6, 2)));
			opcode = insn.slc<OPCODE_SIZE>(2);

            if (opcode == OPC_LW || opcode == OPC_SW) {
                // Increment memory instruction counter
				m_icount.write(m_icount.read() + 1);
			}
            else if (opcode == OPC_JAL || opcode == OPC_JALR) {
				// Increment jump instruction counter
                j_icount.write(j_icount.read() + 1);
			} else if (opcode == OPC_BEQ) {
				// Increment branch instruction counter
				b_icount.write(b_icount.read() + 1);
            } else {
				// Increment other instruction counter
                o_icount.write(o_icount.read() + 1);
			}
            icount.write(icount.read() + 1);
            
            freeze_tmp = false;

           if (insn == 0x0000006f) {
                // jump to yourself (end of program).
                program_end.write(true);
            }

            ac_int < REG_ADDR, false > rs1_addr = insn.slc<REG_ADDR>(15);
            ac_int < REG_ADDR, false > rs2_addr = insn.slc<REG_ADDR>(20);
			
			
			ac_int< 32, false > rs1_sent_pc = sentinel[rs1_addr].slc<32>(1);
			ac_int < 1, false > rs1_sent_valid = sentinel[rs1_addr][0];
            
            if (!fwd.ldst && fwd.pc == rs1_sent_pc && rs1_sent_valid == 1) {
                forward_success_rs1 = true;
                output.rs1 = fwd.regfile_data;
                #ifndef __SYNTHESIS__
                debug_dout_t.rs1 = fwd.regfile_data;
                debug_dout_t.rs1_forward = forward_success_rs1;
                #endif
            } else if (!forward_success_rs1) {
        
                output.rs1 = regfile[rs1_addr];
                #ifndef __SYNTHESIS__
                debug_dout_t.rs1 = regfile[rs1_addr];
                debug_dout_t.rs1_forward = forward_success_rs1;
                #endif
     
            }

            ac_int < 32, false > rs2_sent_pc = sentinel[rs2_addr].slc<32>(1);
			ac_int < 1, false > rs2_sent_valid = sentinel[rs2_addr][0];
			
            if (!fwd.ldst && fwd.pc == rs2_sent_pc && rs2_sent_valid == 1) {
                forward_success_rs2 = true;
                output.rs2 = fwd.regfile_data;
				#ifndef __SYNTHESIS__
                debug_dout_t.rs2 = fwd.regfile_data;
                debug_dout_t.rs2_forward = forward_success_rs2;
                #endif

            } else if (!forward_success_rs2) {
                
                output.rs2 = regfile[rs2_addr];
                #ifndef __SYNTHESIS__
                debug_dout_t.rs2 = regfile[rs2_addr];
                debug_dout_t.rs2_forward = forward_success_rs2;
                #endif

            
            }

            // *** Feedback to fetch data computation and put() section.
            // -- Address sign extensions.
            ac_int < 21, false> immjal_tmp;
            ac_int < 13, false> immbranch_tmp;
            
            immjal_tmp[0] = 0;
            immjal_tmp.set_slc(1, insn.slc<10>(21));
            immjal_tmp[11] = insn[20];
            immjal_tmp.set_slc(12, insn.slc<8>(12));
			immjal_tmp[20] = insn[31];
			
			immbranch_tmp[0] = 0;
            immbranch_tmp.set_slc(1, insn.slc<4>(8));
            immbranch_tmp.set_slc(5, insn.slc<6>(25));            
            immbranch_tmp[11] = insn[7];
			immbranch_tmp[12] = insn[31];

            self_feed.branch_address = sign_extend_branch(immbranch_tmp + pc);
            // -- Jump.
            fetch_out.branch_taken = false;
            fetch_out.btb_update = false;
            fetch_out.address = pc + 4;
            jump = false;
            if (insn.slc<5>(2) == OPC_JAL) {
                self_feed.jump_address = sign_extend_jump(immjal_tmp + pc);
                jump = true;
                fetch_out.btb_update = true;
                fetch_out.branch_taken = true;
                fetch_out.bta = self_feed.jump_address;
                fetch_out.address = self_feed.jump_address;
            } else if (insn.slc<5>(2) == OPC_JALR) {
                ac_int < PC_LEN, false > extended;
                if (insn[31] == 0)
                    extended = 0;
                else
                    extended = 4294967295;

                extended.set_slc(0, insn.slc<12>(20));
                self_feed.jump_address = extended + output.rs1;
                self_feed.jump_address[0] = 0;
                
                jump = true;
                fetch_out.btb_update = true;
                fetch_out.branch_taken = true;
                fetch_out.bta = self_feed.jump_address;
                fetch_out.address = self_feed.jump_address;
            }

            // -- Branch circuitry.
            branch = false;
            if (insn.slc<5>(2) == OPC_BEQ) { // BEQ,BNE, BLT, BGE, BLTU, BGEU
				fetch_out.btb_update = true;
				fetch_out.bta = self_feed.branch_address;
                switch (insn.slc<3>(12)) {
                case FUNCT3_BEQ:
                    if (output.rs1 == output.rs2) {
						branch = true; // BEQ taken.
						fetch_out.branch_taken = true;
						fetch_out.address = self_feed.branch_address;
						#ifndef __SYNTHESIS__
						debug_dout_t.branch_taken = true;
						#endif
					}

                    break;
                case FUNCT3_BNE:
                    if (output.rs1 != output.rs2) {
						branch = true; //BNE taken.
						fetch_out.branch_taken = true;
                        fetch_out.address = self_feed.branch_address;
                        #ifndef __SYNTHESIS__
                        debug_dout_t.branch_taken = true;
                        #endif
                    }
                    break;
                case FUNCT3_BLT:
                    if (output.rs1 < output.rs2) {
						branch = true; // BLT taken
						fetch_out.branch_taken = true;
						fetch_out.address = self_feed.branch_address;
                        #ifndef __SYNTHESIS__
                        debug_dout_t.branch_taken = true;
                        #endif
                    }
                    break;
                case FUNCT3_BGE:
                    if (output.rs1 >= output.rs2) {
						branch = true; // BGE taken.
						fetch_out.branch_taken = true;
						fetch_out.address = self_feed.branch_address;
                        #ifndef __SYNTHESIS__
                        debug_dout_t.branch_taken = true;
                        #endif
                    }
                    break;
                case FUNCT3_BLTU:
                    if (output.rs1 < output.rs2) {
						branch = true; // BLTU taken.
						fetch_out.branch_taken = true;
						fetch_out.address = self_feed.branch_address;
                        #ifndef __SYNTHESIS__
                        debug_dout_t.branch_taken = true;
                        #endif
                    }
                    break;
                case FUNCT3_BGEU:
                    if (output.rs1 >= output.rs2) {
						branch = true; // BGEU taken.
						fetch_out.branch_taken = true;
						fetch_out.address = self_feed.branch_address;
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
            output.dest_reg = insn.slc<5>(7);
            // RD field of insn.
            output.imm_u = insn.slc<20>(12); // This field is then used in the execute stage not only as immU field but to obtain several subfields used by non U-type instructions.

            #ifndef __SYNTHESIS__
            debug_dout_t.dest_reg = std::to_string(insn.slc<5>(7).to_int());
            debug_dout_t.imm_u = insn.slc<20>(12);
            #endif
            // *** END of RD propagation and immediates sign extensions.

            // *** Control word generation.
            switch (insn.slc<OPCODE_SIZE>(2)) { // Opcode's 2 LSBs have been trimmed to save area.

            case OPC_LUI:
                output.alu_op = ALUOP_LUI;
                output.alu_src = ALUSRC_IMM_U;
                output.regwrite = 1;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = 0;
                trap = 0;
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
                output.alu_op = ALUOP_AUIPC;
                output.alu_src = ALUSRC_IMM_U;
                output.regwrite = 1;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = 0;
                trap = 0;
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
                output.alu_op = ALUOP_JAL;
                output.alu_src = ALUSRC_RS2; // Actually does not use RS2 as it performs "rd = pc + 4"
                output.regwrite = 1;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = 0;
                trap = 0;
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
                output.alu_op = ALUOP_JALR;
                output.alu_src = ALUSRC_RS2; // Actually does not use RS2 as it performs "rd = pc + 4"
                output.regwrite = 1;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = 0;
                trap = 0;
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
                output.alu_op = ALUOP_NULL;
                output.alu_src = ALUSRC_RS2;
                output.regwrite = 0;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = 0;
                trap = 0;
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
                switch (insn.slc<3>(12)) {
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
                output.alu_op = ALUOP_ADD;
                output.alu_src = ALUSRC_IMM_I;
                output.regwrite = 1;
                output.st = NO_STORE;
                output.memtoreg = 1;
                trap = 0;
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
                switch (insn.slc<3>(12)) {
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
                output.alu_op = ALUOP_ADD;
                output.alu_src = ALUSRC_IMM_S;
                output.regwrite = 0;
                output.ld = NO_LOAD;
                output.memtoreg = 0;
                trap = 0;
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

                if (insn.slc<7>(25) == FUNCT7_SRAI && insn.slc<3>(12) == FUNCT3_SRAI) {
                    output.alu_op = ALUOP_SRAI;
                    output.alu_src = ALUSRC_IMM_U;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_SRAI";
                    debug_dout_t.alu_src = "ALUSRC_IMM_U";
                    #endif
                } else if (insn.slc<7>(25) == FUNCT7_SLLI && insn.slc<3>(12) == FUNCT3_SLLI) {
                    output.alu_op = ALUOP_SLLI;
                    output.alu_src = ALUSRC_IMM_U;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_SLLI";
                    debug_dout_t.alu_src = "ALUSRC_IMM_U";
                    #endif
                } else if (insn.slc<7>(25) == FUNCT7_SRLI && insn.slc<3>(12) == FUNCT3_SRLI) {
                    output.alu_op = ALUOP_SRLI;
                    output.alu_src = ALUSRC_IMM_U;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_SRLI";
                    debug_dout_t.alu_src = "ALUSRC_IMM_U";
                    #endif
                } else {
                    output.alu_src = ALUSRC_IMM_I;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_src = "ALUSRC_IMM_I";
                    #endif
                    switch (insn.slc<3>(12)) {
                    case FUNCT3_ADDI:
                        output.alu_op = ALUOP_ADDI;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_ADDI";
                        #endif
                        break;
                    case FUNCT3_SLTI:
                        output.alu_op = ALUOP_SLTI;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SLTI";
                        #endif
                        break;
                    case FUNCT3_SLTIU:
                        output.alu_op = ALUOP_SLTIU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SLTIU";
                        #endif
                        break;
                    case FUNCT3_XORI:
                        output.alu_op = ALUOP_XORI;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_XORI";
                        #endif
                        break;
                    case FUNCT3_ORI:
                        output.alu_op = ALUOP_ORI;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_ORI";
                        #endif
                        break;
                    case FUNCT3_ANDI:
                        output.alu_op = ALUOP_ANDI;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_ANDI";
                        #endif
                        break;
                    default:
                        output.alu_op = ALUOP_NULL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_NULL";
                        #endif
                        SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_IMM instruction");
                        break;
                    }
                }
                output.regwrite = 1;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = 0;
                trap = 0;
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.regwrite = "REGWRITE YES";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.st = "NO_STORE";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                #endif
                break;

            case OPC_ADD: // R-type instructions: ADD, SLL, SLT, SLTU, XOR, SRL, OR, AND, SUB, SRA, MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU.
                output.alu_src = ALUSRC_RS2;
                output.regwrite = 1;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = 0;
                trap = 0;
                trap_cause = NULL_CAUSE;

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_src = "ALUSRC_RS2";
                debug_dout_t.regwrite = "REGWRITE YES";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.st = "NO_STORE";
                debug_dout_t.memtoreg = "REGWRITE NO";
                #endif
                // FUNCT7 switch discriminates between classes of R-type instructions.
                switch (insn.slc<7>(25)) {
                case FUNCT7_ADD: // ADD, SLL, SLT, SLTU, XOR, SRL, OR, AND
                    switch (insn.slc<3>(12)) {
                    case FUNCT3_ADD:
                        output.alu_op = ALUOP_ADD;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_ADD";
                        #endif
                        break;
                    case FUNCT3_SLL:
                        output.alu_op = ALUOP_SLL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SLL";
                        #endif
                        break;
                    case FUNCT3_SLT:
                        output.alu_op = ALUOP_SLT;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SLT";
                        #endif
                        break;
                    case FUNCT3_SLTU:
                        output.alu_op = ALUOP_SLTU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SLTU";
                        #endif
                        break;
                    case FUNCT3_XOR:
                        output.alu_op = ALUOP_XOR;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_XOR";
                        #endif
                        break;
                    case FUNCT3_SRL:
                        output.alu_op = ALUOP_SRL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SRL";
                        #endif
                        break;
                    case FUNCT3_OR:
                        output.alu_op = ALUOP_OR;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_OR";
                        #endif
                        break;
                    case FUNCT3_AND:
                        output.alu_op = ALUOP_AND;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_AND";
                        #endif
                        break;
                    default:
                        output.alu_op = ALUOP_NULL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_NULL";
                        #endif
                        SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_ADD instruction");
                        break;
                    }
                    break;
                case FUNCT7_SUB: // SUB, SRA
                    switch (insn.slc<3>(12)) {
                    case FUNCT3_SUB:
                        output.alu_op = ALUOP_SUB;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SUB";
                        #endif
                        break;
                    case FUNCT3_SRA:
                        output.alu_op = ALUOP_SRA;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_SRA";
                        #endif
                        break;
                    default:
                        output.alu_op = ALUOP_NULL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_NULL";
                        #endif
                        SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_SUB instruction");
                        break;
                    }
                    break;
                    #if defined(MUL32) || defined(MUL64) || defined(DIV) || defined(REM)
                case FUNCT7_MUL: // MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU
                    switch (insn<3>(12)) {
                    case FUNCT3_MUL:
                        output.alu_op = ALUOP_MUL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_MUL";
                        #endif
                        break;
                    case FUNCT3_MULH:
                        output.alu_op = ALUOP_MULH;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_MULH";
                        #endif
                        break;
                    case FUNCT3_MULHSU:
                        output.alu_op = ALUOP_MULHSU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_MULHSU";
                        #endif
                        break;
                    case FUNCT3_MULHU:
                        output.alu_op = ALUOP_MULHU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_MULHU";
                        #endif
                        break;
                    case FUNCT3_DIV:
                        output.alu_op = ALUOP_DIV;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_DIV";
                        #endif
                        break;
                    case FUNCT3_DIVU:
                        output.alu_op = ALUOP_DIVU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_DIVU";
                        #endif
                        break;
                    case FUNCT3_REM:
                        output.alu_op = ALUOP_REM;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_REM";
                        #endif
                        break;
                    case FUNCT3_REMU:
                        output.alu_op = ALUOP_REMU;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_REMU";
                        #endif
                        break;
                    default:
                        output.alu_op = ALUOP_NULL;

                        #ifndef __SYNTHESIS__
                        debug_dout_t.alu_op = "ALUOP_NULL";
                        #endif
                        SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_MUL instruction");
                        break;
                    }
                    break;
                    #endif
                default:
                    output.alu_op = ALUOP_NULL;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_NULL";
                    #endif
                    SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP instruction");
                    break;
                }
                break;

                #ifdef CSR_LOGIC
            case OPC_SYSTEM:
                output.alu_op = ALUOP_NULL;
                output.alu_src = ALUSRC_RS2;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = 0;
                output.regwrite = 1;

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_op = "ALUOP_NULL";
                debug_dout_t.alu_src = "ALUSRC_RS2";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.st = "NO_STORE";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                debug_dout_t.regwrite = "REGWRITE YES";
                #endif
                switch (insn.slc<3>(12)) {
                case FUNCT3_EBREAK: // EBREAK, ECALL
                    output.regwrite = 0;
                    trap = 1;
                    output.alu_op = ALUOP_CSRRWI;
                    output.imm_u.set_slc(8, (ac_int<CSR_ADDR, false>)MCAUSE_A); // force the CSR address to MCAUSE's

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRWI";
                    debug_dout_t.imm_u.set_slc(8, (ac_int<3, false>)MCAUSE_A);
                    #endif
                    if (insn[20] == FUNCT7_EBREAK) { // Bit 20 discriminates b/n EBREAK and ECALL
                        // EBREAK and ECALL leverage CSRRWI decoding to write into the MCAUSE register
                        // but keep regwrite to "0" to prevent writeback
                        trap_cause = EBREAK_CAUSE; // may be not necessary but is kept for future implementations
                        output.imm_u.set_slc(3, (ac_int<3, false>)EBREAK_CAUSE); // force the exception cause on the zimm field

                        #ifndef __SYNTHESIS__
                        debug_dout_t.imm_u.set_slc(3, (ac_int<ZIMM_SIZE, false>)EBREAK_CAUSE);
                        #endif
                    } else { // FUNCT7_ECALL
                        trap_cause = ECALL_CAUSE; // may be not necessary but is kept for future implementations
                        output.imm_u.set_slc(3, (ac_int<ZIMM_SIZE, false>)ECALL_CAUSE); // force the exception cause on the zimm field

                        #ifndef __SYNTHESIS__
                        debug_dout_t.imm_u.set_slc(3, (ac_int<3, false>)ECALL_CAUSE);
                        #endif
                    }
                    break;
                case FUNCT3_CSRRW:
                    output.alu_op = ALUOP_CSRRW;
                    trap = 0;
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRW";
                    #endif
                    break;
                case FUNCT3_CSRRS:
                    output.alu_op = ALUOP_CSRRS;
                    trap = 0;
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRS";
                    #endif
                    break;
                case FUNCT3_CSRRC:
                    output.alu_op = ALUOP_CSRRC;
                    trap = 0;
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRC";
                    #endif
                    break;
                case FUNCT3_CSRRWI:
                    output.alu_op = ALUOP_CSRRWI;
                    output.regwrite = 1;
                    trap = 0;
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRWI";
                    debug_dout_t.regwrite = "REGWRITE YES";
                    #endif
                    break;
                case FUNCT3_CSRRSI:
                    output.alu_op = ALUOP_CSRRSI;
                    trap = 0;
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRSI";
                    #endif
                    break;
                case FUNCT3_CSRRCI:
                    output.alu_op = ALUOP_CSRRCI;
                    trap = 0;
                    trap_cause = NULL_CAUSE;

                    #ifndef __SYNTHESIS__
                    debug_dout_t.alu_op = "ALUOP_CSRRCI";
                    #endif
                    break;
                default:
                    output.alu_op = ALUOP_NULL;
                    trap = 0;
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
                output.alu_src = ALUSRC_RS2;
                output.regwrite = 0;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.memtoreg = 0;
                trap = 1;
                trap_cause = ILL_INSN_CAUSE;
                output.alu_op = ALUOP_CSRRWI;
                output.imm_u.set_slc(8, (ac_int<CSR_ADDR, false>)MCAUSE_A); // force the CSR address to MCAUSE's

                #ifndef __SYNTHESIS__
                debug_dout_t.alu_src = "ALUSRC_RS2";
                debug_dout_t.regwrite = "REGWRITE NO";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.st = "NO_STORE";
                debug_dout_t.memtoreg = "MEMTOREG NO";
                debug_dout_t.alu_op = "ALUOP_CSRRWI";
                debug_dout_t.imm_u.set_slc(8, (ac_int<12, false>)MCAUSE_A);
                debug_dout_t.imm_u.set_slc(3, (ac_int<5>)ILL_INSN_CAUSE);
                #endif
                
                SC_REPORT_ERROR(sc_object::name(), "Unimplemented instruction");
                break;
            } // --- END of OPCODE switch
            // *** END of control word generation.
            ac_int <1, false> sen1_test = sentinel[rs1_addr][0];
            ac_int <1, false> sen2_test = sentinel[rs2_addr][0];    
            
            ac_int <XLEN, false> curr_temp_rs2 = 0;
            if (output.alu_src == ALUSRC_RS2) {
                curr_temp_rs2 = output.rs2;

            } else if (output.alu_src == ALUSRC_IMM_I) {
                curr_temp_rs2.set_slc(0, output.imm_u.slc<12>(8));
                if (output.imm_u[19] == 1) {
					curr_temp_rs2.set_slc(12, (ac_int < 20, false >) 1048575);
				}
            } else if (output.alu_src == ALUSRC_IMM_S) {
				curr_temp_rs2.set_slc(0, output.dest_reg);
                curr_temp_rs2.set_slc(5, output.imm_u.slc<7>(13));
                 if (curr_temp_rs2[11] == 1) {
					curr_temp_rs2.set_slc(12, (ac_int < 20, false >) 1048575);
				}

            } else {
				curr_temp_rs2.set_slc(0, (ac_int < 12, false >)0); 
				curr_temp_rs2.set_slc(12, output.imm_u.slc<20>(0));
            }
            
            last_ldst_addr_temp = (output.rs1 + curr_temp_rs2).slc<DCACHE_INDEX_WIDTH>(2 + DCACHE_OFFSET_WIDTH);
            
            if ((output.ld != NO_LOAD || output.st != NO_STORE) && (last_ldst_addr_temp == last_ldst_addr) && last_ldst_valid && !freeze) {
                load_instruction = true;
                load_pc = pc;
            }
            
            freeze = false;
            if (load_instruction || (sen1_test && !forward_success_rs1) || (sen2_test && !forward_success_rs2)) {
				freeze = true;
			}  
            
            ac_int < 1, false > out_regwrite = output.regwrite;
            ac_int < 33, false > sen_input;
            
            if (!freeze && output.regwrite[0] == 1 && output.dest_reg != 0) {
                sentinel[output.dest_reg].set_slc(1,pc); // Set corresponding sentinel flag.
                sentinel[output.dest_reg][0] = 1;

                if (output.dest_reg == rs1_addr) {
                    forward_success_rs1 = true;
                } else if (output.dest_reg == rs2_addr) {
                    forward_success_rs2 = true;
                }
            }

            // *** Transform instruction into nop when freeze is active
            if (insn == 0 || freeze) {
                // Bubble.
                output.regwrite = 0;
                output.ld = NO_LOAD;
                output.st = NO_STORE;
                output.alu_op = ALUOP_NULL;
				fetch_out.btb_update = false;
                #ifndef __SYNTHESIS__
                debug_dout_t.regwrite = "REGWRITE NO";
                debug_dout_t.ld = "NO_LOAD";
                debug_dout_t.st = "NO_STORE";
                #endif
            }
            
            if (output.ld != NO_LOAD || output.st != NO_STORE) {
				last_ldst_addr = last_ldst_addr_temp;
				last_ldst_valid = true;
			}else {
				last_ldst_valid = false;
			}
			
            if (!freeze) {
				fetch_dout.Push(fetch_out);
			}
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
            DPRINT(endl);
            #endif
           
            wait();

        } // *** ENDOF while(true)
    } // *** ENDOF sc_cthread

    // --- Utility functions.

    // Sign extend UJ insn.
    ac_int < PC_LEN, false > sign_extend_jump(ac_int < 21, false > imm) {
        if (imm[20] == 1) {
			ac_int < 32, false > ext_imm = 4294967295;
            ext_imm.set_slc(0, imm);
            return ext_imm;
        }
        else {
			ac_int < 32, false > ext_imm = imm;
			return ext_imm;
		}
    }

    // Sign extend branch insn.
    ac_int < PC_LEN, false > sign_extend_branch(ac_int < 13, false > imm) {
        
        if (imm[12] == 1) {
			ac_int < 32, false > ext_imm = 4294967295;
            ext_imm.set_slc(0, imm);
            return ext_imm;
        }
        else {
			ac_int < 32, false > ext_imm = imm;
			return ext_imm;
		}
    }

    // --- End of utility functions.
};

#endif
