/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief 
	Header file for execute stage.
	Division algorithm for DIV, DIVU, REM, REMU instructions. Division by zero
	and overflow semantics are compliant with the RISC-V specs (page 32).

	@note Changes from HL5

		- Use of HLSLibs connections for communication with the rest of the processor.

		- Stall functionality

		- Consists of only one thread


*/

#ifndef __EXECUTE__H
#define __EXECUTE__H

#ifndef NDEBUG
    #include <iostream>
    #define DPRINT(msg) std::cout << msg;
#endif

#define BIT(_N)(1 << _N)

#include "drim4hls_datatypes.h"
#include "defines.h"
#include "globals.h"

#include <mc_connections.h>

// Signed division quotient and remainder struct.
struct div_res_t {
    sc_int < XLEN > quotient;
    sc_int < XLEN > remainder;
};

// Unsigned division quotient and remainder struct.
struct u_div_res_t {
    sc_uint < XLEN > quotient;
    sc_uint < XLEN > remainder;
};

SC_MODULE(execute) {
    
    #ifndef __SYNTHESIS__
    struct debug_exe_out // TODO: fix all sizes
    {
        //
        // Member declarations.
        //
        sc_uint < 3 > ld;
        sc_uint < 2 > st;
        sc_uint < 1 > memtoreg;
        sc_uint < 1 > regwrite;
        sc_uint < XLEN > alu_res;
        sc_uint < DATA_SIZE > mem_datain;
        sc_uint < REG_ADDR > dest_reg;
        sc_uint < TAG_WIDTH > tag;
        std::string alu_src;
        std::string alu_op;

    }
    debug_exe_out_t;
    #endif
    // Clock and reset signals
    sc_in < bool > CCS_INIT_S1(clk);
    sc_in < bool > CCS_INIT_S1(rst);
    
    // FlexChannel initiators
    Connections::In < de_out_t > CCS_INIT_S1(din);
    Connections::Out < exe_out_t > CCS_INIT_S1(dout);
    // Forward
    Connections::Out < reg_forward_t > CCS_INIT_S1(fwd_exe);
    
    // Member variables
    de_out_t data_in;
    de_out_t input;
    exe_out_t output;
    dmem_in_t dmem_din;
    reg_forward_t forward;

    sc_uint < XLEN > csr[CSR_NUM]; // Control and status registers.

    bool freeze;
    
    // Constructor
    SC_CTOR(execute): din("din"), dout("dout"), fwd_exe("fwd_exe"), clk("clk"), rst("rst") {
        SC_THREAD(execute_th);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);
    }

    u_div_res_t udiv_func(sc_uint < XLEN > num, sc_uint < XLEN > den) {
        sc_uint < XLEN > rem;
        sc_uint < XLEN > quotient;
        u_div_res_t u_div_res;

        rem = 0;
        quotient = 0;

        DIVIDE_LOOP:
            for (sc_int < 6 > i = 31; i >= 0; i--) {
                // Break EXE stage protocol for DSE

                const sc_uint < XLEN > mask = BIT(i);
                const sc_uint < XLEN > lsb = (mask & num) >> i;

                rem = rem << 1;
                rem = rem | lsb;

                if (rem >= den) {
                    rem -= den;
                    quotient = quotient | mask;
                }
                wait();
            }

        u_div_res.quotient = quotient;
        u_div_res.remainder = rem;

        return u_div_res;
    }

    div_res_t div_func(sc_int < XLEN > num, sc_int < XLEN > den) {
        bool num_neg;
        bool den_neg;
        div_res_t div_res;
        u_div_res_t u_div_res;

        num_neg = num < 0;
        den_neg = den < 0;

        if (num_neg)
            num = -num;
        if (den_neg)
            den = -den;

        u_div_res = udiv_func((sc_uint < XLEN > ) num, (sc_uint < XLEN > ) den);
        div_res.quotient = (sc_int < XLEN > ) u_div_res.quotient;
        div_res.remainder = (sc_int < XLEN > ) u_div_res.remainder;

        if (num_neg ^ den_neg)
            div_res.quotient = -div_res.quotient;
        else
            div_res.quotient = div_res.quotient;

        return div_res;
    }

    void execute_th(void) {
        EXE_RST: {
            din.Reset();
            dout.Reset();
            fwd_exe.Reset();
			
            output.tag = 0;

            csr[MISA_I] = 0x40001101; // RV32IMA
            csr[MARCHID_I] = 0x0; // Not implemented (should be assigned by RISC-V
            csr[MIMPID_I] = 0x0; // Not implemented (processor revision)
            csr[MHARTID_I] = 0x0; // Single thread (always 0)
            csr[MINSTRET_I] = 0x0; // Retired instructions
            csr[MCYCLE_I] = 0x0; // Cycle count (32-bits only for now)
			
            wait();
        }
        
        #pragma hls_pipeline_init_interval 1
        #pragma pipeline_stall_mode flush
        EXE_BODY: while (true) {
            input = din.Pop();
            
            csr[MCYCLE_I]++;            

            // Compute
            output.regwrite = input.regwrite;
            output.memtoreg = input.memtoreg;
            output.ld = input.ld;
            output.st = input.st;
            output.dest_reg = input.dest_reg;
            output.mem_datain = input.rs2;
            output.tag = input.tag;
            output.pc = input.pc;
			
            bool nop = false;
            if (input.regwrite[0] == 0 &&
                input.ld == NO_LOAD &&
                input.st == NO_STORE &&
                input.alu_op == ALUOP_NULL) {
                nop = true;
            }
            #ifdef MUL64
            // 64-bit temporary multiplication result, for upper 32 bit multiplications (MULH, MULHU, MULHSU).
            //int64_t tmp_mul_res = 0;
            sc_uint <64> tmp_mul_res = 0;
            #endif
            #ifdef DIV
            // Temporary division results.
            div_res_t div_res = {
                0,
                0
            };
            u_div_res_t u_div_res = {
                0,
                0
            };
            #endif
            #ifdef CSR_LOGIC
            // Temporary CSR index
            sc_uint < CSR_IDX_LEN > csr_index = 0;
            #endif

            // Sign extend the immediate operand for I-type instructions.
            sc_uint < XLEN > tmp_sigext_imm_i = 0;
            if (input.imm_u[19] == 1) {
                // Extend with 1s
                tmp_sigext_imm_i = (sc_uint < 20 > (1048575), (sc_uint < 12 > ) input.imm_u.range(19, 8));
            } else {
                // Extend with 0s
                tmp_sigext_imm_i = (sc_uint < 20 > (0), (sc_uint < 12 > ) input.imm_u.range(19, 8));
            }
            // Zero-fill the immediate operand for U-type instructions.
            sc_uint < XLEN > tmp_zerofill_imm_u = ((sc_uint < 20 > ) input.imm_u.range(19, 0), sc_uint < 12 > (0));
            // ALU 2nd operand multiplexing based on ALUSRC signal.
            sc_uint < XLEN > tmp_rs2 = 0;

            if (input.alu_src == ALUSRC_RS2) {
                tmp_rs2 = input.rs2;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_src = "ALUSRC_RS2";
                #endif

            } else if (input.alu_src == ALUSRC_IMM_I) {
                tmp_rs2 = tmp_sigext_imm_i;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_src = "ALUSRC_IMM_I";
                #endif

            } else if (input.alu_src == ALUSRC_IMM_S) {
                // reconstructs imm_s from imm_u and rd
                sc_uint<12> imm_s = (sc_uint<7>(input.imm_u.range(19, 13)), input.dest_reg);
                tmp_rs2 = sign_extend_imm_s(imm_s);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_src = "ALUSRC_IMM_S";
                #endif

            } else {
                // ALUSRC_IMM_U
                tmp_rs2 = tmp_zerofill_imm_u;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_src = "ALUSRC_IMM_U";
                #endif
            }

            // ALU body
            switch (input.alu_op) {
            case ALUOP_ADD: // ADD, ADDI, SB, SH, SW, LB, LH, LW, LBU, LHU.
                output.alu_res = (sc_uint<32>) input.rs1.to_int() + tmp_rs2.to_int();

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_ADD";
                #endif

                break;
            case ALUOP_SLT: // SLT, SLTI
                if ((sc_int<32>) input.rs1 < (sc_int<32>) tmp_rs2)
                    output.alu_res = 1;
                else
                    output.alu_res = 0;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_SLT";
                #endif

                break;
            case ALUOP_SLTU: // SLTU, SLTIU
                if ((sc_uint<32>) input.rs1  < (sc_uint<32>) tmp_rs2)
                    output.alu_res = 1;
                else
                    output.alu_res = 0;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_SLTU";
                #endif

                break;
            case ALUOP_XOR: // XOR, XORI
                output.alu_res = input.rs1 ^ tmp_rs2;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_XOR";
                #endif

                break;
            case ALUOP_OR: // OR, ORI
                output.alu_res = input.rs1 | tmp_rs2;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_OR";
                #endif

                break;
            case ALUOP_AND: // AND, ANDI
                output.alu_res = input.rs1 & tmp_rs2;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_AND";
                #endif

                break;
            case ALUOP_SLL: // SLL
                output.alu_res = (sc_uint < XLEN >) input.rs1 << (sc_uint < SHAMT >) tmp_rs2.range(4, 0);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_SLL";
                #endif

                break;
            case ALUOP_SRL: // SRL
                output.alu_res = (sc_uint < XLEN >) input.rs1 >> (sc_uint < SHAMT >) tmp_rs2.range(4, 0);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_SRL";
                #endif

                break;
            case ALUOP_SRA: // SRA
                // >> is arith right sh. for sc_int operand
                output.alu_res = (sc_int < XLEN >) input.rs1 >> (sc_uint < SHAMT >) tmp_rs2.range(4, 0);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_SRA";
                #endif

                break;
            case ALUOP_SUB: // SUB
                output.alu_res = (sc_uint < XLEN >) ((sc_int < XLEN >) input.rs1 - (sc_int < XLEN >) tmp_rs2);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_SUB";
                #endif

                break;
            case ALUOP_SLLI: // SLLI
                output.alu_res = (sc_uint < XLEN >) input.rs1 << (sc_uint < SHAMT >) tmp_rs2.range(24, 20);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_SLLI";
                #endif

                break;
            case ALUOP_SRLI: // SRLI
                output.alu_res = (sc_uint < XLEN >) input.rs1 >> (sc_uint < SHAMT >) tmp_rs2.range(24, 20);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_SRLI";
                #endif

                break;
            case ALUOP_SRAI: // SRAI
                // >> is arith right sh. for sc_int operand
                output.alu_res = (sc_int < XLEN >) input.rs1 >> (sc_uint < SHAMT >) tmp_rs2.range(24, 20);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_SRAI";
                #endif

                break;
            case ALUOP_LUI: // LUI
                // zerofill_imm_u
                output.alu_res = tmp_rs2;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_LUI";
                #endif

                break;
            case ALUOP_AUIPC: // AUIPC
                // zerofill_imm_u + pc
                output.alu_res = (sc_int < XLEN >) tmp_rs2 + (sc_int < XLEN >) input.pc;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_AUIPC";
                #endif

                break;
            case ALUOP_JAL: // JAL, JALR
                // link register update
                output.alu_res = (sc_int < XLEN >) input.pc + 4;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_JAL";
                #endif

                break;
                #ifdef MUL32
            case ALUOP_MUL: // MUL: signed * signed, return lower 32 bits
                output.alu_res = (sc_int < XLEN >) input.rs1 * (sc_int < XLEN >) tmp_rs2;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_MUL";
                #endif

                break;
                #endif
                #ifdef MUL64
            case ALUOP_MULH: // MULH: signed * signed, return upper 32 bits
                tmp_mul_res = input.rs1.to_int() * tmp_rs2.to_int();
                output.alu_res = sc_uint<XLEN*2>(sc_int<XLEN*2>(tmp_mul_res)).range((XLEN*2)-1, XLEN);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_MULH";
                #endif

                break;
            case ALUOP_MULHSU: // MULHSU: signed * unsigned, return upper 32 bits
                tmp_mul_res = input.rs1 * tmp_rs2.to_uint();
                output.alu_res = sc_uint<XLEN*2>(sc_int<XLEN*2>(tmp_mul_res)).range((XLEN*2)-1, XLEN);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_MULHSU";
                #endif

                break;
            case ALUOP_MULHU: // MULHU: unsigned * unsigned, return upper 32 bits
                tmp_mul_res = input.rs1.to_int() * tmp_rs2.to_uint();
                output.alu_res = sc_uint<XLEN*2>(sc_int<XLEN*2>(tmp_mul_res)).range((XLEN*2)-1, XLEN);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_MULHU";
                #endif

                break;
                #endif
                #ifdef DIV
            case ALUOP_DIV: // DIV calls div_func
                div_res = div_func((sc_int < XLEN >) input.rs1, (sc_int < XLEN >) tmp_rs2);
                output.alu_res = div_res.quotient;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_DIV";
                #endif

                break;
            case ALUOP_DIVU: // DIVU calls udiv_func
                u_div_res = udiv_func(input.rs1.to_uint(), tmp_rs2.to_uint());
                output.alu_res = u_div_res.quotient;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_DIVU";
                #endif

                break;
                #endif
                #ifdef REM
            case ALUOP_REM: // REM calls div_func
                div_res = div_func((sc_int < XLEN >) input.rs1, (sc_int < XLEN >) tmp_rs2);
                output.alu_res = div_res.remainder;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_REM";
                #endif

                break;
            case ALUOP_REMU: // REMU calls udiv_func
                u_div_res = udiv_func( input.rs1.to_uint(), tmp_rs2.to_uint());
                output.alu_res = u_div_res.remainder;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_REMU";
                #endif

                break;
                #endif
                #ifdef CSR_LOGIC
                // All CSRx instructions exploit imm_u[19:8] to get the csr address.
                // This avoids having 12 more bits on the FEDEC-EXE Flex Channel.
                // The same goes for imm_u[7:3] i.e. zimm for the 3 CSRxI instructions.
            case ALUOP_CSRRW: // CSRRW
                csr_index = get_csr_index(input.imm_u.range(19, 8));
                output.alu_res = csr[csr_index];
                set_csr_value(csr_index, input.rs1.to_uint(), CSR_OP_WR, input.imm_u.range(19, 18).to_uint());

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRW";
                #endif

                break;
            case ALUOP_CSRRS: // CSRRS
                csr_index = get_csr_index(input.imm_u.range(19, 8));
                output.alu_res = csr[csr_index];
                set_csr_value(csr_index, input.rs1.to_uint(), CSR_OP_SET, input.imm_u.range(19, 18).to_uint());

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRS";
                #endif

                break;
            case ALUOP_CSRRC: // CSRRC
                csr_index = get_csr_index(input.imm_u.range(19, 8));
                output.alu_res = csr[csr_index];
                set_csr_value(csr_index, input.rs1.to_uint(), CSR_OP_CLR, input.imm_u.range(19, 18).to_uint());

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRC";
                #endif

                break;
            case ALUOP_CSRRWI: // CSRRWI
                csr_index = get_csr_index(input.imm_u.range(19, 8));
                output.alu_res = csr[csr_index];
                set_csr_value(csr_index, input.imm_u.range(7, 3).to_uint(), CSR_OP_WR, input.imm_u.range(19, 18).to_uint());

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRWI";
                #endif

                break;
            case ALUOP_CSRRSI: // CSRRSI
                csr_index = get_csr_index(input.imm_u.range(19, 8));
                output.alu_res = csr[csr_index];
                set_csr_value(csr_index, input.imm_u.range(7, 3).to_uint(), CSR_OP_SET, input.imm_u.range(19, 18).to_uint());

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRSI";
                #endif

                break;
            case ALUOP_CSRRCI: // CSRRCI
                csr_index = get_csr_index(input.imm_u.range(19, 8));
                output.alu_res = csr[csr_index];
                set_csr_value(csr_index, input.imm_u.range(7, 3).to_uint(), CSR_OP_CLR, input.imm_u.range(19, 18).to_uint());

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRCI";
                #endif

                break;
                #endif
            default: // ALUOP_NULL (do nothing)
                output.alu_res = 0;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_NULL";
                #endif

                break;
            }
			
            if ((input.ld != NO_LOAD || input.st != NO_STORE) && !nop) {
                forward.ldst = true;
            } else {
                forward.ldst = false;
            }

            if (output.alu_res == ALUOP_NULL) {
                forward.ldst = true;
            }

            if (!nop) {
                forward.tag = output.tag;
                forward.regfile_data = output.alu_res;
                forward.pc = input.pc;
            }

            if (!nop)
               csr[MINSTRET_I]++;

            // Put
			fwd_exe.Push(forward);
            dout.Push(output);
            #ifndef __SYNTHESIS__
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "nop " << nop << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "pc= " << input.pc << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "forward.regfile_data " << forward.regfile_data << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "forward.tag " << forward.tag << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "output.alu_op " << debug_exe_out_t.alu_op << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "output.alu_res " << output.alu_res << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "output.ld " << output.ld << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "output.st " << output.st << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "output.regwrite  " << output.regwrite << endl);
            DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "output.dest_reg  " << output.dest_reg << endl);
            DPRINT(endl);
            #endif

            wait();
        }
    }

    /* Support functions */

    // Sign extend immS.
    sc_uint < XLEN > sign_extend_imm_s(sc_uint < 12 > imm) {
        sc_uint <XLEN> imm_ext = 0;
        if (imm[11] == 1) {
			// Extend with 1s
			imm_ext.range(11, 0) = imm;
			imm_ext.range(31, 12) = (sc_uint < 20 >) 1048575;
            return imm_ext;
        }
        else { 
			// Extend with 0s
			imm_ext.range(11 , 0) = imm;
            return imm_ext;
        }
    }

    #ifdef CSR_LOGIC
    // Zero extends the zimm immediate field of CSRRWI, CSRRSI, CSRRCI
    sc_uint < XLEN > zero_ext_zimm(sc_uint < ZIMM_SIZE > zimm) {
        sc_uint <XLEN> zimm_ext = 0;
        zimm_ext.range(4, 0) = zimm;
        return zimm_ext;
    }

    // Return index given a csr address.
    sc_uint < CSR_IDX_LEN > get_csr_index(sc_uint < CSR_ADDR > csr_addr) {
        switch (csr_addr) {
        case USTATUS_A:
            return USTATUS_I;
        case MSTATUS_A:
            return MSTATUS_I;
        case MISA_A:
            return MISA_I;
        case MTVECT_A:
            return MTVECT_I;
        case MEPC_A:
            return MEPC_I;
        case MCAUSE_A:
            return MCAUSE_I;
        case MCYCLE_A:
            return MCYCLE_I;
        case MARCHID_A:
            return MARCHID_I;
        case MIMPID_A:
            return MIMPID_I;
        case MINSTRET_A:
            return MINSTRET_I;
        case MHARTID_A:
            return MHARTID_I;
        default:
            return 6; // TODO: this is not ideal. I default unsupported CSRs to MARCHID as it's not a critical register.
        }
    }

    // Set value of csr[csr_addr]
    // TODO: respect unwritable fields, see manual for each individual implemented CSR.
    // TODO: for now any bits of every register are fully readable/writeable.
    // TODO: This must be changed in future implementations.
    void set_csr_value(sc_uint < CSR_IDX_LEN > csr_index, sc_uint < XLEN > rs1, sc_uint < LOG2_CSR_OP_NUM > operation, sc_uint < 2 > rw_permission) {
        if (rw_permission != 3)
            switch (operation) {
            case CSR_OP_WR:
                csr[csr_index] = rs1.to_uint();
                break;
            case CSR_OP_SET:
                csr[csr_index] |= rs1.to_uint();
                break;
            case CSR_OP_CLR:
                csr[csr_index] &= ~(rs1.to_uint());
                break;
            default:
                break;
            }
    }
    #endif
};

#endif
