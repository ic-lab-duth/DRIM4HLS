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

#ifndef __EXECUTE_FP__H
#define __EXECUTE_FP__H

#ifndef NDEBUG
    #include <iostream>
    #define DPRINT(msg) std::cout << msg;
#endif

#define BIT(_N)(1 << _N)

#include "drim4hls_datatypes.h"
#include "defines.h"
#include "globals.h"

#include <mc_connections.h>
#include <ac_int.h>
#include "fast_float.h"

typedef ffp32 T;

SC_MODULE(execute_fp) {
	
	#ifndef __SYNTHESIS__
    struct debug_exe_out // TODO: fix all sizes
    {
        //
        // Member declarations.
        //
        sc_bv < 3 > ld;
        sc_bv < 2 > st;
        sc_bv < 1 > memtoreg;
        sc_bv < 1 > regwrite;
        sc_bv < XLEN > alu_res;
        sc_bv < DATA_SIZE > mem_datain;
        sc_bv < REG_ADDR > dest_reg;
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
    de_out_t input;
    exe_out_t output;
    dmem_in_t dmem_din;
    reg_forward_t forward;

    ac_int < XLEN, false > fcsr; // Control and status registers.
    sc_out < ac_int < 32, false > > test_exefp_pc;
    
    // Constructor
    SC_CTOR(execute_fp): din("din"), dout("dout"), fwd_exe("fwd_exe"), clk("clk"), rst("rst") {
        SC_THREAD(executefp_th);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);
    }

    void executefp_th(void) {
        EXE_RST: {
            din.Reset();
            dout.Reset();
            fwd_exe.Reset();
			
            output.tag = 0;
			
            wait();
        }
        
        #pragma hls_pipeline_init_interval 1
        #pragma pipeline_stall_mode flush
        EXE_BODY: while (true) {
            input = din.Pop();
            
            // Compute
            output.regwrite = input.regwrite;
            output.memtoreg = input.memtoreg;
            output.ld = input.ld;
            output.st = input.st;
            output.dest_reg = input.dest_reg;
            output.mem_datain = input.rs2;
            output.tag = input.tag;
            output.pc = input.pc;
            output.dest_freg = input.dest_freg;
			
            bool nop = false;
            
            if (input.regwrite[0] == 0 &&
                !input.flw &&
                !input.fsw &&
                input.alu_op == ALUOP_NULL) {
                nop = true;
            }
            
            T output_fp;

            #ifdef CSR_LOGIC
				// Temporary CSR index
				ac_int < CSR_IDX_LEN, false > csr_index = 0;
            #endif
			
			// Sign extend the immediate operand for I-type instructions.
            ac_int < XLEN, false > tmp_sigext_imm_i = 0;
            if (input.imm_u[19] == 1) {
                // Extend with 1s
                //tmp_sigext_imm_i = (sc_bv < 20 > (1048575), (sc_bv < 12 > ) input.imm_u.range(19, 8));
                tmp_sigext_imm_i.set_slc(0, input.imm_u.slc<12>(8));
                tmp_sigext_imm_i.set_slc(12, (ac_int < 20, false >) 1048575);
            } else {
                // Extend with 0s
                //tmp_sigext_imm_i = (sc_bv < 20 > (0), (sc_bv < 12 > ) input.imm_u.range(19, 8));
                tmp_sigext_imm_i.set_slc(0, input.imm_u.slc<12>(8));
            }
            // Zero-fill the immediate operand for U-type instructions.
            //sc_bv < XLEN > tmp_zerofill_imm_u = ((sc_bv < 20 > ) input.imm_u.range(19, 0), sc_bv < 12 > (0));
			ac_int < XLEN, false > tmp_zerofill_imm_u;
			tmp_zerofill_imm_u.set_slc(0, (ac_int < 12, false >)0); 
			tmp_zerofill_imm_u.set_slc(12, input.imm_u.slc<20>(0));
            // ALU 2nd operand multiplexing based on ALUSRC signal.
            ac_int < XLEN, false > tmp_rs2 = 0;

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
                ac_int < 12, false > imm_s;
                imm_s.set_slc(0, input.dest_reg);
                imm_s.set_slc(5, input.imm_u.slc<7>(13));
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
            
            T fp_rs1 = (ac_int<XLEN, false>) input.rs1;
            T fp_rs2 = (ac_int<XLEN, false>) tmp_rs2;
            T fp_rs3 = (ac_int<XLEN, false>) input.rs3;
			
            // FALU body
            switch (input.alu_op) {
            case ALUOP_FADD: // FADD
                output_fp = fp_rs1 + fp_rs2;
                output.alu_res = mapFp(output_fp);

                break;
            case ALUOP_FSUB: // FSUB
				output_fp = fp_rs1 - fp_rs2;
				output.alu_res = mapFp(output_fp);

                break;
            case ALUOP_FMUL: // FMUL
				output_fp = fp_rs1 * fp_rs2;
				output.alu_res = mapFp(output_fp);

                break;
			//case ALUOP_FDIV: // FDIV -----------------------------------------> NOT SUPPORTED
            //    output_fp = fp_rs1 / fp_rs2;
            //    output.alu_res = mapFp(output_fp);

            //    break;
            case ALUOP_FMIN: // FMIN

                if (fp_rs1 < fp_rs2) {
					output_fp = fp_rs1;
				}else {
					output_fp = fp_rs2;
				}
				output.alu_res = mapFp(output_fp);
				
                break;
            case ALUOP_FMAX: // FMAX

                if (fp_rs1 > fp_rs2) {
					output_fp = fp_rs1;
				}else {
					output_fp = fp_rs2;
				}

                break;
            /*case ALUOP_FSQRT: // FSQRT
				output_fp = fp_rs1 * fp_rs2;
                output.alu_res = mapFp(output_fp);

                break; */
            case ALUOP_FMADD: // FMADD
                output_fp = (fp_rs1 * fp_rs2) + fp_rs3;
                output.alu_res = mapFp(output_fp);

                break;
            case ALUOP_FNMADD: // FNMADD
				fp_rs1.sign = ~fp_rs1.sign;
                output_fp = (fp_rs1 * fp_rs2) - fp_rs3;
                output.alu_res = mapFp(output_fp);

                break;
            case ALUOP_FMSUBS: // FNSUBS
                output_fp = (fp_rs1 * fp_rs2) - fp_rs3;
                output.alu_res = mapFp(output_fp);

                break;
            case ALUOP_FNMSUBS: // FNMSUBS
				fp_rs1.sign = ~fp_rs1.sign;
                output_fp = (fp_rs1 * fp_rs2) + fp_rs3;
                output.alu_res = mapFp(output_fp);

                break;
			case ALUOP_FCVTWS: // FCVTWS
				output.alu_res = ffp2int(fp_rs1);
				
                break;
            case ALUOP_FCVTWUS: // FCVTWUS
				output.alu_res = ffp2int(fp_rs1, true);
				
                break;
            case ALUOP_FCVTSW: // FCVTSW
				output_fp = int2ffp(input.rs1);
				//output_fp = (float) input.rs1.to_int();
				output.alu_res = mapFp(output_fp);
				
                break;
            case ALUOP_FCVTSWU: // FCVTSWU
				output_fp = int2ffp(input.rs1, true);
				//output_fp = (float) input.rs1.to_int();
				output.alu_res = mapFp(output_fp);
				
                break;
            case ALUOP_FSGNJ: // FSGNJ
				fp_rs1.sign = fp_rs2.sign;
				output.alu_res = mapFp(fp_rs1);

                break;
            case ALUOP_FSGNJN: // FSGNJN
				fp_rs1.sign = ~fp_rs2.sign;
				output.alu_res = mapFp(fp_rs1);

                break;
            case ALUOP_FSGNJX: // FSGNJX
				fp_rs1.sign = fp_rs1.sign ^ fp_rs2.sign;
				output.alu_res = mapFp(fp_rs1);

                break;
            case ALUOP_FMVWX: // FMVWX -------------------------------> check
				output.alu_res = input.rs1;

                break;
            case ALUOP_FMVXW: // FMVXW -------------------------------> check
				output.alu_res = input.rs1;

                break;
            case ALUOP_FEQ: // FEQ
				if (fp_rs1  == fp_rs2)
                    output.alu_res = 1;
                else
                    output.alu_res = 0;

                break;
            case ALUOP_FLT: // FLT
				if (fp_rs1  < fp_rs2)
                    output.alu_res = 1;
                else
                    output.alu_res = 0;

                break;
            case ALUOP_FLE: // FLE
				if (fp_rs1  <= fp_rs2)
                    output.alu_res = 1;
                else
                    output.alu_res = 0;

                break;
            case ALUOP_FCLASS: // FCLASS
				// rs1 is -Inf
				if (fp_rs1.sign == 1 && (ac_int< fp_rs1.man_width, false>) fp_rs1.exponent == 255)
                    output.alu_res = 1;
                // rs1 is negative normal number
                if (fp_rs1.sign == 1 && fp_rs1.mantissa[22] == 1)
                    output.alu_res = 2;
                // rs1 is negative subnormal number
                if (fp_rs1.sign == 1 && fp_rs1.mantissa[22] == 0)
                    output.alu_res = 4;
                // rs1 is -0
                if (fp_rs1.sign == 1 && fp_rs1.exponent == (ac_int< fp_rs1.exp_width, false>)  0)
                    output.alu_res = 8;
				// rs1 is +0
                if (fp_rs1.sign == 0 && fp_rs1.exponent == (ac_int< fp_rs1.exp_width, false>) 0)
                    output.alu_res = 16;
                // rs1 is positive subnormal number
                if (fp_rs1.sign == 0 && fp_rs1.mantissa[22] == 0)
                    output.alu_res = 32;
                // rs1 is positive normal number
                if (fp_rs1.sign == 0 && fp_rs1.mantissa[22] == 1)
                    output.alu_res = 64;
                // rs1 is +Inf
                if (fp_rs1.sign == 0 && fp_rs1.exponent == (ac_int< fp_rs1.exp_width, false>) 255)
                    output.alu_res = 128;
                // rs1 is NaN
                if ( fp_rs1.exponent == (ac_int< fp_rs1.exp_width, false>) 255 && fp_rs1.mantissa == (ac_int< fp_rs1.man_width, false>) NaN)
                    output.alu_res = 256;
                break;
            #ifdef CSR_LOGIC
                // All CSRx instructions exploit imm_u[19:8] to get the csr address.
                // This avoids having 12 more bits on the FEDEC-EXE Flex Channel.
                // The same goes for imm_u[7:3] i.e. zimm for the 3 CSRxI instructions.
            case ALUOP_FSCSR: // CSRRW
                //csr_index = get_csr_index(input.imm_u.slc<12>(8));
                output.alu_res = fcsr;
                set_csr_value(input.rs1, CSR_OP_WR, input.imm_u.slc<2>(18));

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRW";
                #endif

                break;
            case ALUOP_FRCSR: // CSRRS
                //csr_index = get_csr_index(input.imm_u.slc<12>(8));
                output.alu_res = fcsr;

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRS";
                #endif

                break;
            case ALUOP_FRRM: // CSRRS
                //csr_index = get_csr_index(input.imm_u.slc<12>(8));
                output.alu_res = 0;
                output.alu_res = fcsr.template slc<3>(5);
				
                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRS";
                #endif

                break;
            case ALUOP_FSRM: // CSRRS
                //csr_index = get_csr_index(input.imm_u.slc<12>(8));
                output.alu_res = 0;
                output.alu_res = fcsr.template slc<3>(5);
				set_csr_value(input.rs1.template slc<3>(0), CSR_OP_WR, input.imm_u.slc<2>(18));
                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRS";
                #endif

                break;
            case ALUOP_FSRMI: // CSRRWI
				output.alu_res = 0;
                output.alu_res = fcsr.template slc<3>(5);
                set_csr_value(input.imm_u.slc<5>(3), CSR_OP_WR, input.imm_u.slc<2>(18));

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRWI";
                #endif

                break;
            case ALUOP_FRFLAGS: // CSRRWI
                output.alu_res = 0;
                output.alu_res = fcsr.template slc<5>(0);

                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRWI";
                #endif

                break;
            case ALUOP_FSFLAGS: // CSRRWI
				output.alu_res = 0;
                output.alu_res = fcsr.template slc<5>(0);
				set_csr_value(input.rs1.template slc<5>(0), CSR_OP_WR, input.imm_u.slc<2>(18));
                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRWI";
                #endif

                break;
            case ALUOP_FSFLAGSI: // CSRRWI
				output.alu_res = 0;
                output.alu_res = fcsr.template slc<5>(0);
				set_csr_value(input.imm_u.slc<5>(3), CSR_OP_WR, input.imm_u.slc<2>(18));
                #ifndef __SYNTHESIS__
                debug_exe_out_t.alu_op = "ALUOP_CSRRWI";
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
    ac_int < XLEN, false > sign_extend_imm_s(ac_int < 12, false > imm) {
        ac_int <XLEN, false> imm_ext = 0;
        if (imm[11] == 1) {
			// Extend with 1s
			imm_ext.set_slc(0, imm);
			imm_ext.set_slc(12, (ac_int < 20, false >) 1048575);
            return imm_ext;
        }
        else { 
			// Extend with 0s
			imm_ext.set_slc(0, imm);
            return imm_ext;
        }
    }

    #ifdef CSR_LOGIC
    // Zero extends the zimm immediate field of CSRRWI, CSRRSI, CSRRCI
    ac_int < XLEN, false > zero_ext_zimm(ac_int < ZIMM_SIZE, false > zimm) {
        ac_int <XLEN, false> zimm_ext = 0;
        zimm_ext.set_slc(0, zimm);
        return zimm_ext;
    }

    // Return index given a csr address.
    ac_int < CSR_IDX_LEN, false > get_csr_index(ac_int < CSR_ADDR, false > csr_addr) {
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
    void set_csr_value( ac_int < XLEN, true> rs1, ac_int < LOG2_CSR_OP_NUM, false > operation, ac_int < 2, false > rw_permission) {
        if (rw_permission != 3)
            switch (operation) {
            case CSR_OP_WR:
                fcsr = rs1.to_uint();
                break;
            case CSR_OP_SET:
                fcsr |= rs1.to_uint();
                break;
            case CSR_OP_CLR:
                fcsr &= ~(rs1.to_uint());
                break;
            default:
                break;
            }
    }
    #endif
    
	ac_int<32, false> ffp2int (T in, bool u = false) {
  
	  ac_int<32, false> output = (ac_int<32, false>) 0;
	  
	  #ifndef __SYNTHESIS__
		std::cout << "initial output " << output << std::endl;
	  #endif
	  
	  ac_int<8, false> exponent = in.exponent - 127;
	  ac_int<24, false> mantissa = 0;
	  
	  mantissa[23] = 1;
	  mantissa.set_slc(0, in.mantissa);
	  #ifndef __SYNTHESIS__
		std::cout << "initial mantissa " << mantissa << std::endl;
	  #endif	  
		for (int i = 0; i < 127 ; i++) {
			output = output << 1;
			output[0] = mantissa[23];
			
			#ifndef __SYNTHESIS__
				std::cout << " output[0] " << output[0] << std::endl;
				std::cout << " mantissa[22] " << mantissa[23] << std::endl;
			#endif
			mantissa = mantissa << 1;


			if (exponent == i) {
				break;
			}
		}
	  
	  #ifndef __SYNTHESIS__
		std::cout << "exponent " << exponent.template slc<7>(0) << std::endl;
	  #endif
	  	  
	  #ifndef __SYNTHESIS__
		std::cout << " output " << output << std::endl;
	  #endif
	  if (in.sign == 1 && !u) {
		output = ~output;
		output = output + 1;
	  }
	  
	  return output;
	};
	
	T int2ffp (ac_int <32, false> in, bool u = false) {
	  T output = (ac_int<32, false>) 0;
	  ac_int<5, false> index_counter = 23;
	  
	  output.sign = (u) ? (ac_int<1, false>) 0 : in[31];
	  if (in[31] == 1) {
		in = -in;
	  }
	  in[31] = 0;
	  
	  if (in.template slc<8>(23) == 0) {
		for (int i = 0; i < 23; i++) {
			in = in << 1;
			index_counter--;
			
			if (in[23] == 1){
				break;
			}
		}
	  }else {
		
		for (int i = 0; i < 7; i++) {
			if (in.template slc<7>(24) == 0 && in[23] == 1) {
				break;
			}
			in = in >> 1;
			index_counter++;
		}
	  }
	  #ifndef __SYNTHESIS__
		std::cout << "index_counter " << index_counter << std::endl;
	  #endif
	  
	  output.exponent = (in[23] == 1) ? (ac_int<8,false>) (127 + index_counter) : (ac_int<8,false>) 0;
	  output.mantissa = in.template slc<23>(0);
	  
	  return output;
	};
	
	ac_int<XLEN, false> mapFp (T &in) {
		ac_int<XLEN, false> output = 0;
		
		output.set_slc(in.man_width + in.exp_width, in.sign);
		output.set_slc(in.man_width, in.exponent);
		output.set_slc(0, in.mantissa);
	  
		return output;
	};
	
	

};

#endif
