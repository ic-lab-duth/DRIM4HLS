/* Copyright 2017 Columbia University, SLD Group */

//
// fedec.cpp - Robert Margelli
// Implementation of the fedec stage ie combined fetch and decode stages.
//

#include "fedec.hpp"
#include <string>

// #define MAIN_START_O2 0x10074
#define SENTINEL_INIT (1 << (TAG_WIDTH - 1))

#ifndef NDEBUG
  #include <iostream>
  #define DPRINT(msg) std::cout << msg;
#else
  #define DPRINT(msg)
#endif

//for debugging purposes
struct debug_dout
{	//
	// Member declarations.
	//
	std::string regwrite;
	std::string memtoreg;
	std::string ld;
	std::string st;
	std::string alu_op;
	std::string alu_src;
	sc_bv< XLEN > rs1;
	sc_bv< XLEN > rs2;
	std::string dest_reg;
	int pc;
	int aligned_pc;
	sc_bv< XLEN-12 > imm_u;
	sc_uint< TAG_WIDTH > tag;

} debug_dout_t;


void fedec::fedec_th(void)
{
FEDEC_RST:
	{
		dout.Reset();
		feed_from_wb.Reset();

		// Init. sentinel flags to zero.
		for(int i = 0; i < REG_NUM; i++) {
			sentinel[i] = SENTINEL_INIT;
		}

		// Program has not completed
		program_end.write(false);

		// Reset instruction counters
		icount.write(0); // any
		j_icount.write(0); // jump
		b_icount.write(0); // branch
		m_icount.write(0); // load, store
		o_icount.write(0); // other

		position = 0;
		trap = "0";
		trap_cause = NULL_CAUSE;
		freeze = false;
		tag = 0;

		// TODO removeme
		// main_start.write(false);
		// main_end.write(false);

		do {wait();} while(!fetch_en);

		//  Init. pc to START_ADDRESS - 4 as on first fetch it will be incremented by
		//  4, thus fetching instruction at address 0
		pc = entry_point.read();
	}

FEDEC_BODY:
	while(true) {

		// --- Fetch
		if (!freeze) {
			// Increment instruction counter
			icount.write(icount.read() + 1);

			if (self_feed.jump == "1")
				pc = (sc_uint<PC_LEN>)self_feed.jump_address;
			else if (self_feed.branch == "1")
				pc = (sc_uint<PC_LEN>)self_feed.branch_address;
			else
				pc = sc_uint<PC_LEN>(pc + 4);
			tag = (tag + 1) % (1 << (TAG_WIDTH - 1));
		}

		output.pc = pc;
		unsigned int aligned_pc = pc >> 2;
		
		//sc_assert(sc_time_stamp().to_double() < 2000000);
		debug_dout_t.pc = pc;
		debug_dout_t.aligned_pc = aligned_pc;
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "pc= " << pc << endl);
		DPRINT(endl);

		wait();
		unsigned int imem_data = imem[aligned_pc];
		insn = imem_data;
		
		//cout << "@" << sc_time_stamp() << "\t" << name() << " instruction=" << imem_data << endl ;
		// Increment some instruction counters
		if (!freeze) {
			sc_uint<OPCODE_SIZE> opcode = sc_uint<OPCODE_SIZE>(sc_bv<OPCODE_SIZE>(insn.range(6, 2)));

			if (opcode == OPC_LW || opcode == OPC_SW)
				// Increment memory instruction counter
				m_icount.write(m_icount.read() + 1);
			else if (opcode == OPC_JAL || opcode == OPC_JALR)
				// Increment jump instruction counter
				j_icount.write(j_icount.read() + 1);
			else if (opcode == OPC_BEQ)
				// Increment branch instruction counter
				b_icount.write(b_icount.read() + 1);
			else
				// Increment other instruction counter
				o_icount.write(o_icount.read() + 1);
		}

		freeze = false; // Un-freeze fetching

		if (insn == 0x0000006f) {
			// jump to yourself (end of program).
			program_end.write(true);
		}
		// --- ENDOF Fetch

		// --- Decode
		if (position > 1) { // Synchronize with WB stage at startup.
			feedinput = feed_from_wb.Pop(); // Get from writeback.
			//cout << "@" << sc_time_stamp() << "\t" << name() << " receiving input feedinput=" << feedinput << endl ;
		}
		else
			position++;

		// *** Register file write.
		if (feedinput.regwrite == "1" && feedinput.regfile_address != "00000") { // Actual writeback.
			regfile[sc_uint<REG_ADDR>(feedinput.regfile_address)] = feedinput.regfile_data; // Overwrite register.
			DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "x" << std::dec << sc_uint<REG_ADDR>(feedinput.regfile_address) << std::hex << "= " << feedinput.regfile_data << endl);
			if (feedinput.tag == sentinel[sc_uint<REG_ADDR>(feedinput.regfile_address)])
				sentinel[sc_uint<REG_ADDR>(feedinput.regfile_address)] = SENTINEL_INIT; // Reset sentinel flag.
		}

		// *** Register file read.
		bool forward_success_rs1 = false;
		bool forward_success_rs2 = false;
		sc_uint<REG_ADDR> rs1_addr(sc_bv<REG_ADDR>(insn.range(19, 15)));
		sc_uint<REG_ADDR> rs2_addr(sc_bv<REG_ADDR>(insn.range(24, 20)));
		bool rs1_raw = false;
		bool rs2_raw = false;
		reg_forward_t fwd = fwd_exe.read();
#ifdef FWD_ENABLE
		if (!fwd.ldst && fwd.tag == sentinel[rs1_addr] && fwd.tag != SENTINEL_INIT) {
			forward_success_rs1 = true;
			output.rs1 = fwd.regfile_data;
			debug_dout_t.rs1 = fwd.regfile_data;
			DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "rs1 forward data of x" << std::dec << rs1_addr << std::hex << "= " << fwd.regfile_data << endl);
		} else {
#endif		
			output.rs1 = regfile[rs1_addr];
			debug_dout_t.rs1 = regfile[rs1_addr];
			DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "rs1 forward data of x" << std::dec << rs1_addr << std::hex << "= " << regfile[rs1_addr] << endl);
#ifdef FWD_ENABLE
		}
		if (!fwd.ldst && fwd.tag == sentinel[rs2_addr] && fwd.tag != SENTINEL_INIT) {
			forward_success_rs2 = true;
			output.rs2 = fwd.regfile_data;
			debug_dout_t.rs2 = fwd.regfile_data;
			DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "rs2 forward data of x" << std::dec << rs2_addr << std::hex << "= " << fwd.regfile_data << endl);
		} else {
#endif		
			output.rs2 = regfile[rs2_addr];
			debug_dout_t.rs2 = regfile[rs2_addr];
			DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "rs2 forward data of x" << std::dec << rs2_addr << std::hex << "= " << regfile[rs2_addr] << endl);
#ifdef FWD_ENABLE
		}
#endif

		// *** Feedback to fetch data computation and put() section.
		// -- Address sign extensions.
		sc_bv<21> immjal_tmp = ((sc_bv<1>)insn.range(31, 31), (sc_bv<8>)insn.range(19, 12), (sc_bv<1>)insn.range(20, 20), (sc_bv<10>)insn.range(30, 21), (sc_bv<1>)(0));
		sc_bv<13> immbranch_tmp = ((sc_bv<1>)insn.range(31, 31), (sc_bv<1>)insn.range(7, 7), (sc_bv<6>)insn.range(30, 25), (sc_bv<4>)insn.range(11, 8), (sc_bv<1>)(0));
		self_feed.branch_address = sc_bv<PC_LEN>((sc_int<PC_LEN>)sign_extend_branch(immbranch_tmp) + (sc_int<PC_LEN>)pc);

		// -- Jump.
		if (insn.range(6, 2) == OPC_JAL) {
			self_feed.jump_address = sc_bv<PC_LEN>((sc_int<PC_LEN>)sign_extend_jump(immjal_tmp) + (sc_int<PC_LEN>)pc);
			self_feed.jump = "1";

			// TODO remove me
// 			if (MAIN_START_O2 == self_feed.jump_address) {
// 				main_start.write(true);
// 				main_end.write(false);
// 				return_address = pc + 4;
// #ifdef VERBOSE
// #ifndef STRATUS_HLS

// 				std::cout << "Jump to main; return address is " << std::hex << return_address << std::dec << std::endl;
// #endif
// #endif

// 			}
		}
		else if (insn.range(6, 2) == OPC_JALR) {
			sc_bv<PC_LEN> extended;
			if (insn[31] == 0)
				extended = ("00000000000000000000000000000000");
			else 
				extended = ("11111111111111111111111111111111");

			extended.range(11, 0) = insn.range(31, 20);
		    self_feed.jump_address = sc_bv<PC_LEN>((sc_int<PC_LEN>) extended + (sc_int<PC_LEN>) output.rs1);
			self_feed.jump_address.range(0, 0) = "0";
			self_feed.jump = "1";

			// TODO remove me
// 			if (return_address == self_feed.jump_address) {
// 				main_end.write(true);
// 				main_start.write(false);
// #ifdef VERBOSE
// #ifndef STRATUS_HLS

// 				std::cout << "Returning from main from address " << std::hex << pc << std::dec << std::endl;
// #endif
// #endif

// 			}
		}
		else{
			self_feed.jump = "0";
		}

		// -- Branch circuitry.
		self_feed.branch = "0";
		if (insn.range(6, 2) == OPC_BEQ) {  // BEQ,BNE, BLT, BGE, BLTU, BGEU
			switch(sc_uint<3>(sc_bv<3>(insn.range(14, 12)))) {
			case FUNCT3_BEQ:
				if (output.rs1 == output.rs2)
					self_feed.branch = "1";  // BEQ taken.
				break;
			case FUNCT3_BNE:
				if (output.rs1 != output.rs2)
					self_feed.branch = "1";  // BNE taken.
				break;
			case FUNCT3_BLT:
				if ((sc_int<XLEN>)output.rs1 < (sc_int<XLEN>)output.rs2)
					self_feed.branch = "1";  // BLT taken.
				break;
			case FUNCT3_BGE:
				if ((sc_int<XLEN>)output.rs1 >= (sc_int<XLEN>)output.rs2)
					self_feed.branch = "1";  // BGE taken.
				break;
			case FUNCT3_BLTU:
				if ((sc_uint<XLEN>)output.rs1 < (sc_uint<XLEN>)output.rs2)
					self_feed.branch = "1";  // BLTU taken.
				break;
			case FUNCT3_BGEU:
				if ((sc_uint<XLEN>)output.rs1 >= (sc_uint<XLEN>)output.rs2)
					self_feed.branch = "1";  // BGEU taken.
				break;
			default:
				self_feed.branch = "0";  // default to not taken.
				break;
			}
		}
		// -- All data for feedback path to fetch is ready now. Do put(): in this version it saved data in self_feed.
		// *** END of feedback to fetch data computation and put() section.

		// *** Propagations: rd, immediates sign extensions.
		output.dest_reg = insn.range(11, 7); 
		debug_dout_t.dest_reg = std::to_string(insn.range(11, 7).to_int());                           // RD field of insn.
		output.imm_u = insn.range(31, 12);                              // This field is then used in the execute stage not only as immU field but to obtain several subfields used by non U-type instructions.
		debug_dout_t.imm_u = insn.range(31, 12);  
		// *** END of RD propagation and immediates sign extensions.

		// *** Control word generation.
		switch(sc_uint<OPCODE_SIZE>(sc_bv<OPCODE_SIZE>(insn.range(6, 2)))) { // Opcode's 2 LSBs have been trimmed to save area.

		case OPC_LUI:
			output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_LUI;
			output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_IMM_U;
			output.regwrite = "1";
			output.ld       = NO_LOAD;
			output.st       = NO_STORE;			
			output.memtoreg = "0";		
			trap = "0";
			trap_cause = NULL_CAUSE;

			debug_dout_t.alu_op = "ALUOP_LUI";
			debug_dout_t.alu_src = "ALUSRC_IMM_U";
			debug_dout_t.regwrite = "REGWRITE YES";
			debug_dout_t.ld = "NO LOAD";
			debug_dout_t.st = "NO STORE";
			debug_dout_t.memtoreg = "MEMTOREG YES";
			break;

		case OPC_AUIPC:
			output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_AUIPC;			
			output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_IMM_U;
			output.regwrite = "1";
			output.ld       = NO_LOAD;
			output.st       = NO_STORE;
			output.memtoreg = "0";
			trap = "0";
			trap_cause = NULL_CAUSE;
			
			debug_dout_t.alu_op = "ALUOP_AUIPC";
			debug_dout_t.alu_src = "ALUSRC_IMM_U";
			debug_dout_t.regwrite = "REGWRITE YES";
			debug_dout_t.ld = "NO LOAD";
			debug_dout_t.st = "NO STORE";
			debug_dout_t.memtoreg = "MEMTOREG NO";
			break;

		case OPC_JAL:
			output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_JAL;
			output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_RS2;   // Actually does not use RS2 as it performs "rd = pc + 4"
			output.regwrite = "1";
			output.ld       = NO_LOAD;
			output.st       = NO_STORE;
			output.memtoreg = "0";			
			trap = "0";
			trap_cause = NULL_CAUSE;

			debug_dout_t.alu_op = "ALUOP_JAL";
			debug_dout_t.alu_src = "ALUSRC_RS2";
			debug_dout_t.regwrite = "REGWRITE YES";
			debug_dout_t.ld = "NO LOAD";
			debug_dout_t.st = "NO STORE";
			debug_dout_t.memtoreg = "MEMTOREG NO";
			break;

		case OPC_JALR:  // same as JAL, could optimize
			output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_JALR;			
			output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_RS2;   // Actually does not use RS2 as it performs "rd = pc + 4"
			output.regwrite = "1";
			output.ld       = NO_LOAD;
			output.st       = NO_STORE;
			output.memtoreg = "0";
			trap = "0";
			trap_cause = NULL_CAUSE;
			rs1_raw = true;

			debug_dout_t.alu_op = "ALUOP_JALR";
			debug_dout_t.alu_src = "ALUSRC_RS2";
			debug_dout_t.regwrite = "REGWRITE YES";
			debug_dout_t.ld = "NO LOAD";
			debug_dout_t.st = "NO STORE";
			debug_dout_t.memtoreg = "MEMTOREG NO";
			break;

		case OPC_BEQ: // Branch instructions: BEQ, BNE, BLT, BGE, BLTU, BGEU
			output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_NULL;
			output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_RS2;
			output.regwrite = "0";
			output.ld       = NO_LOAD;
			output.st       = NO_STORE;
			output.memtoreg = "0";			
			trap = "0";
			trap_cause = NULL_CAUSE;
			rs1_raw = true;
			rs2_raw = true;

			debug_dout_t.alu_op = "ALUOP_NULL";
			debug_dout_t.alu_src = "ALUSRC_RS2";
			debug_dout_t.regwrite = "REGWRITE NO";
			debug_dout_t.ld = "NO LOAD";
			debug_dout_t.st = "NO STORE";
			debug_dout_t.memtoreg = "MEMTOREG NO";
			break;

		case OPC_LW:
			switch(sc_uint<3>(sc_bv<3>(insn.range(14, 12)))) {
			case FUNCT3_LB:
				output.ld       = LB_LOAD;
				debug_dout_t.ld = "LB_LOAD";
				break;
			case FUNCT3_LH:
				output.ld       = LH_LOAD;
				debug_dout_t.ld = "LH_LOAD";
				break;
			case FUNCT3_LW:
				output.ld       = LW_LOAD;
				debug_dout_t.ld = "LW_LOAD";
				break;
			case FUNCT3_LBU:
				output.ld       = LBU_LOAD;
				debug_dout_t.ld = "LBU_LOAD";
				break;
			case FUNCT3_LHU:
				output.ld       = LHU_LOAD;
				debug_dout_t.ld = "LHU_LOAD";
				break;
			default:
				output.ld       = NO_LOAD;
				debug_dout_t.ld = "NO_LOAD";
				SC_REPORT_ERROR(sc_object::name(), "Unimplemented LOAD instruction");
				break;
			}
			output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_ADD;		
			output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_IMM_I;
			output.regwrite = "1";
			output.st       = NO_STORE;
			output.memtoreg = "1";
			trap = "0";
			trap_cause = NULL_CAUSE;
			rs1_raw = true; // Base address

			debug_dout_t.alu_op = "ALUOP_ADD";
			debug_dout_t.alu_src = "ALUSRC_IMM_I";
			debug_dout_t.regwrite = "REGWRITE YES";
			debug_dout_t.st = "NO_STORE";
			debug_dout_t.memtoreg = "MEMTOREG YES";
			break;

		case OPC_SW:
			switch(sc_uint<3>(sc_bv<3>(insn.range(14, 12)))) {
			case FUNCT3_SB:
				output.st       = SB_STORE;
				debug_dout_t.st = "SB_STORE";
				break;
			case FUNCT3_SH:
				output.st       = SH_STORE;
				debug_dout_t.st = "SH_STORE";
				break;
			case FUNCT3_SW:
				output.st       = SW_STORE;
				debug_dout_t.st = "SW_STORE";
				break;
			default:
				output.st       = NO_STORE;
				debug_dout_t.st = "NO_STORE";
				SC_REPORT_ERROR(sc_object::name(), "Unimplemented STORE instruction");
				break;
			}
			output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_ADD;
			output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_IMM_S;
			output.regwrite = "0";
			output.ld       = NO_LOAD;		
			output.memtoreg = "0";
			trap = "0";
			trap_cause = NULL_CAUSE;
			rs1_raw = true; // Base address
			rs2_raw = true; // Input Data

			debug_dout_t.alu_op = "ALUOP_ADD";
			debug_dout_t.alu_src = "ALUSRC_IMM_S";
			debug_dout_t.regwrite = "REGWRITE NO";
			debug_dout_t.ld = "NO_LOAD";
			debug_dout_t.memtoreg = "MEMTOREG NO";
			break;

		case OPC_ADDI:  // OP-IMM instructions (arithmetic and logical operations on immediates): ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI

			if (sc_uint<7>(sc_bv<7>(insn.range(31, 25))) == FUNCT7_SRAI && sc_uint<3>(sc_bv<3>(insn.range(14, 12))) == FUNCT3_SRAI) {
				output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SRAI;
				output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_IMM_U;

				debug_dout_t.alu_op = "ALUOP_SRAI";
				debug_dout_t.alu_src = "ALUSRC_IMM_U";
			}
			else if (sc_uint<7>(sc_bv<7>(insn.range(31, 25))) == FUNCT7_SLLI && sc_uint<3>(sc_bv<3>(insn.range(14, 12))) == FUNCT3_SLLI) {
				output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SLLI;
				output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_IMM_U;

				debug_dout_t.alu_op = "ALUOP_SLLI";
				debug_dout_t.alu_src = "ALUSRC_IMM_U";
			}
			else if (sc_uint<7>(sc_bv<7>(insn.range(31, 25))) == FUNCT7_SRLI && sc_uint<3>(sc_bv<3>(insn.range(14, 12))) == FUNCT3_SRLI) {
				output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SRLI;
				output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_IMM_U;

				debug_dout_t.alu_op = "ALUOP_SRLI";
				debug_dout_t.alu_src = "ALUSRC_IMM_U";
			}
			else{
				output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_IMM_I;
				debug_dout_t.alu_src = "ALUSRC_IMM_I";
				switch(sc_uint<3>(sc_bv<3>(insn.range(14, 12)))) {
				case FUNCT3_ADDI:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_ADDI;
					debug_dout_t.alu_op = "ALUOP_ADDI";
					break;
				case FUNCT3_SLTI:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SLTI;
					debug_dout_t.alu_op = "ALUOP_SLTI";
					break;
				case FUNCT3_SLTIU:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SLTIU;
					debug_dout_t.alu_op = "ALUOP_SLTIU";
					break;
				case FUNCT3_XORI:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_XORI;
					debug_dout_t.alu_op = "ALUOP_XORI";
					break;
				case FUNCT3_ORI:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_ORI;
					debug_dout_t.alu_op = "ALUOP_ORI";
					break;
				case FUNCT3_ANDI:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_ANDI;
					debug_dout_t.alu_op = "ALUOP_ANDI";
					break;
				default:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_NULL;
					debug_dout_t.alu_op = "ALUOP_NULL";
					SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_IMM instruction");
					break;
				}
			}
			output.regwrite = "1";
			output.ld       = NO_LOAD;
			output.st       = NO_STORE;
			output.memtoreg = "0";		
			trap = "0";
			trap_cause = NULL_CAUSE;
			rs1_raw = true;

			debug_dout_t.regwrite = "REGWRITE YES";
			debug_dout_t.ld = "NO_LOAD";
			debug_dout_t.st = "NO_STORE";
			debug_dout_t.memtoreg = "MEMTOREG NO";
			break;

		case OPC_ADD: // R-type instructions: ADD, SLL, SLT, SLTU, XOR, SRL, OR, AND, SUB, SRA, MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU.
			output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_RS2;
			output.regwrite = "1";
			output.ld       = NO_LOAD;
			output.st       = NO_STORE;
			output.memtoreg = "0";	
			trap = "0";
			trap_cause = NULL_CAUSE;

			debug_dout_t.alu_src = "ALUSRC_RS2";
			debug_dout_t.regwrite = "REGWRITE YES";
			debug_dout_t.ld = "NO_LOAD";
			debug_dout_t.st = "NO_STORE";
			debug_dout_t.memtoreg = "REGWRITE NO";
			// FUNCT7 switch discriminates between classes of R-type instructions.
			switch(sc_uint<7>(sc_bv<7>(insn.range(31, 25)))) {
			case FUNCT7_ADD:    // ADD, SLL, SLT, SLTU, XOR, SRL, OR, AND
				switch(sc_uint<3>(sc_bv<3>(insn.range(14, 12)))) {
				case FUNCT3_ADD:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_ADD;
					debug_dout_t.alu_op = "ALUOP_ADD";
					break;
				case FUNCT3_SLL:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SLL;
					debug_dout_t.alu_op = "ALUOP_SLL";
					break;
				case FUNCT3_SLT:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SLT;
					debug_dout_t.alu_op = "ALUOP_SLT";
					break;
				case FUNCT3_SLTU:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SLTU;
					debug_dout_t.alu_op = "ALUOP_SLTU";
					break;
				case FUNCT3_XOR:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_XOR;
					debug_dout_t.alu_op = "ALUOP_XOR";
					break;
				case FUNCT3_SRL:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SRL;
					debug_dout_t.alu_op = "ALUOP_SRL";
					break;
				case FUNCT3_OR:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_OR;
					debug_dout_t.alu_op = "ALUOP_OR";
					break;
				case FUNCT3_AND:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_AND;
					debug_dout_t.alu_op = "ALUOP_AND";
					break;
				default:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_NULL;
					debug_dout_t.alu_op = "ALUOP_NULL";
					SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_ADD instruction");
					break;
				}
				break;
			case FUNCT7_SUB:    // SUB, SRA
				switch(sc_uint<3>(sc_bv<3>(insn.range(14, 12)))) {
				case FUNCT3_SUB:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SUB;
					debug_dout_t.alu_op = "ALUOP_SUB";
					break;
				case FUNCT3_SRA:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_SRA;
					debug_dout_t.alu_op = "ALUOP_SRA";
					break;
				default:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_NULL;
					debug_dout_t.alu_op = "ALUOP_NULL";
					SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_SUB instruction");
					break;
				}
				break;
#if defined(MUL32) || defined(MUL64) || defined(DIV) || defined(REM)
			case FUNCT7_MUL:    // MUL, MULH, MULHSU, MULHU, DIV, DIVU, REM, REMU
				switch(sc_uint<3>(sc_bv<3>(insn.range(14, 12)))) {
				case FUNCT3_MUL:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_MUL;
					debug_dout_t.alu_op = "ALUOP_MUL";
					break;
				case FUNCT3_MULH:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_MULH;
					debug_dout_t.alu_op = "ALUOP_MULH";
					break;
				case FUNCT3_MULHSU:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_MULHSU;
					debug_dout_t.alu_op = "ALUOP_MULHSU";
					break;
				case FUNCT3_MULHU:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_MULHU;
					debug_dout_t.alu_op = "ALUOP_MULHU";
					break;
				case FUNCT3_DIV:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_DIV;
					debug_dout_t.alu_op = "ALUOP_DIV";
					break;
				case FUNCT3_DIVU:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_DIVU;
					debug_dout_t.alu_op = "ALUOP_DIVU";
					break;
				case FUNCT3_REM:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_REM;
					debug_dout_t.alu_op = "ALUOP_REM";
					break;
				case FUNCT3_REMU:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_REMU;
					debug_dout_t.alu_op = "ALUOP_REMU";
					break;
				default:
					output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_NULL;
					debug_dout_t.alu_op = "ALUOP_NULL";
					SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP_MUL instruction");
					break;
				}
				break;
#endif
			default:
				output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_NULL;
				debug_dout_t.alu_op = "ALUOP_NULL";
				SC_REPORT_ERROR(sc_object::name(), "Unimplemented ALUOP instruction");
				break;
			}
			rs1_raw = true;
			rs2_raw = true;
			break;

#ifdef CSR_LOGIC
		case OPC_SYSTEM:
			output.alu_op   = (sc_bv<ALUOP_SIZE>)ALUOP_NULL;
			output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_RS2;
			output.ld       = NO_LOAD;
			output.st       = NO_STORE;
			output.memtoreg = "0";
			output.regwrite = "1";		
			rs1_raw = true;

			debug_dout_t.alu_op = "ALUOP_NULL";
			debug_dout_t.alu_src = "ALUSRC_RS2";
			debug_dout_t.ld = "NO_LOAD";
			debug_dout_t.st = "NO_STORE";
			debug_dout_t.memtoreg = "MEMTOREG NO";
			debug_dout_t.regwrite = "REGWRITE YES";
			switch(sc_uint<3>(sc_bv<3>(insn.range(14, 12)))) {
			case FUNCT3_EBREAK: // EBREAK, ECALL
				output.regwrite = "0";
				rs1_raw = false;
				trap = "1";
				output.alu_op = (sc_bv<ALUOP_SIZE>)ALUOP_CSRRWI;
				output.imm_u.range(19, 8) = (sc_bv<CSR_ADDR>)MCAUSE_A;    // force the CSR address to MCAUSE's
				debug_dout_t.alu_op = "ALUOP_CSRRWI";
				debug_dout_t.imm_u.range(19, 8) = (sc_bv<CSR_ADDR>)MCAUSE_A;
				if (sc_bv<1>(insn.range(20, 20)) == (sc_bv<1>)FUNCT7_EBREAK) {   // Bit 20 discriminates b/n EBREAK and ECALL
					// EBREAK and ECALL leverage CSRRWI decoding to write into the MCAUSE register
					// but keep regwrite to "0" to prevent writeback
					trap_cause = EBREAK_CAUSE;  // may be not necessary but is kept for future implementations
					output.imm_u.range(7, 3) = (sc_bv<ZIMM_SIZE>)EBREAK_CAUSE;  // force the exception cause on the zimm field
					debug_dout_t.imm_u.range(7, 3) = (sc_bv<ZIMM_SIZE>)EBREAK_CAUSE;
				}
				else{  // FUNCT7_ECALL
					trap_cause = ECALL_CAUSE;   // may be not necessary but is kept for future implementations
					output.imm_u.range(7, 3) = (sc_bv<ZIMM_SIZE>)ECALL_CAUSE;   // force the exception cause on the zimm field
					debug_dout_t.imm_u.range(7, 3) = (sc_bv<ZIMM_SIZE>)ECALL_CAUSE;
				}
				break;
			case FUNCT3_CSRRW:
				output.alu_op = (sc_bv<ALUOP_SIZE>)ALUOP_CSRRW;
				trap = "0";
				trap_cause = NULL_CAUSE;

				debug_dout_t.alu_op = "ALUOP_CSRRW";
				break;
			case FUNCT3_CSRRS:
				output.alu_op = (sc_bv<ALUOP_SIZE>)ALUOP_CSRRS;
				trap = "0";
				trap_cause = NULL_CAUSE;

				debug_dout_t.alu_op = "ALUOP_CSRRS";
				break;
			case FUNCT3_CSRRC:
				output.alu_op = (sc_bv<ALUOP_SIZE>)ALUOP_CSRRC;
				trap = "0";
				trap_cause = NULL_CAUSE;

				debug_dout_t.alu_op = "ALUOP_CSRRC";
				break;
			case FUNCT3_CSRRWI:
				output.alu_op = (sc_bv<ALUOP_SIZE>)ALUOP_CSRRWI;
				output.regwrite = "1";
				trap = "0";
				trap_cause = NULL_CAUSE;

				debug_dout_t.alu_op = "ALUOP_CSRRWI";
				debug_dout_t.regwrite = "REGWRITE YES";
				break;
			case FUNCT3_CSRRSI:
				output.alu_op = (sc_bv<ALUOP_SIZE>)ALUOP_CSRRSI;
				trap = "0";
				trap_cause = NULL_CAUSE;

				debug_dout_t.alu_op = "ALUOP_CSRRSI";
				break;
			case FUNCT3_CSRRCI:
				output.alu_op = (sc_bv<ALUOP_SIZE>)ALUOP_CSRRCI;
				trap = "0";
				trap_cause = NULL_CAUSE;
				
				debug_dout_t.alu_op = "ALUOP_CSRRCI";
				break;
			default:
				output.alu_op = (sc_bv<ALUOP_SIZE>)ALUOP_NULL;
				trap = "0";
				trap_cause = NULL_CAUSE;
				debug_dout_t.alu_op = "ALUOP_NULL";
				SC_REPORT_ERROR(sc_object::name(), "Unimplemented SYSTEM instruction");
				break;
			}
			break;
#endif  // --- End of System instructions decoding

                default: // illegal instruction
			output.alu_src  = (sc_bv<ALUSRC_SIZE>)ALUSRC_RS2;			
			output.regwrite = "0";
			output.ld       = NO_LOAD;
			output.st       = NO_STORE;
			output.memtoreg = "0";
			trap = "1";
			trap_cause = ILL_INSN_CAUSE;
			output.alu_op = (sc_bv<ALUOP_SIZE>)ALUOP_CSRRWI;
			output.imm_u.range(19, 8) = (sc_bv<12>)MCAUSE_A;    // force the CSR address to MCAUSE's

			debug_dout_t.alu_src = "ALUSRC_RS2";
			debug_dout_t.regwrite = "REGWRITE NO";
			debug_dout_t.ld = "NO_LOAD";
			debug_dout_t.st = "NO_STORE";
			debug_dout_t.memtoreg = "MEMTOREG NO";
			debug_dout_t.alu_op = "ALUOP_CSRRWI";
			debug_dout_t.imm_u.range(19, 8) = (sc_bv<12>)MCAUSE_A;
			debug_dout_t.imm_u.range(7, 3) = (sc_bv<5>)ILL_INSN_CAUSE;
			break;
			SC_REPORT_ERROR(sc_object::name(), "Unimplemented instruction");
		} // --- END of OPCODE switch
		// *** END of control word generation.

		// *** Stall mechanism
		if ((sentinel[rs1_addr] != SENTINEL_INIT && rs1_raw && !forward_success_rs1) || // If RAW on RS1
		    (sentinel[rs2_addr] != SENTINEL_INIT && rs2_raw && !forward_success_rs2))   // If RAW on RS2
			freeze = true;


		if (!freeze && output.regwrite == "1" && output.dest_reg != "00000")
			sentinel[sc_uint<REG_ADDR>(output.dest_reg)] = tag;   // Set corresponding sentinel flag.

		if (!freeze)
			output.tag = tag;
		else
			output.tag = SENTINEL_INIT;

// #ifdef VERBOSE

// #ifndef STRATUS_HLS
// 			Color::Modifier red(Color::FG_RED);
// 			Color::Modifier yellow(Color::FG_YELLOW);
// 			Color::Modifier green(Color::FG_GREEN);
// 			Color::Modifier def(Color::FG_DEFAULT);

// 			if (freeze)
// 				std::cout << red;
// 			else if (self_feed.jump == "1" || self_feed.branch == "1")
// 				std::cout << yellow;
// 			std::cout << std::hex << std::right << "PC[0x" << std::setw(8)  << pc << "] -> " << insn << std::dec << " -> ";
// 			std::cout << def;


// 			if (rs1_raw)
// 				std::cout << " " << yellow << std::right << std::setfill(' ') << std::setw(3) << rs1_addr << " " << def;
// 			else
// 				std::cout << " " << std::right << std::setfill(' ') << std::setw(3) << rs1_addr << " ";
// 			if (rs2_raw)
// 				std::cout << yellow << std::right << std::setfill(' ') << std::setw(3) << rs2_addr << def << " -> ";
// 			else
// 				std::cout << std::right << std::setfill(' ') << std::setw(3) << rs2_addr << " -> ";
// 			if (output.regwrite == "1" )
// 				std::cout << yellow << std::right << std::setfill(' ') << std::setw(3) << output.dest_reg.to_uint() << def;
// 			else
// 				std::cout << std::right << std::setfill(' ') << std::setw(3) << output.dest_reg.to_uint();
// #endif


// #ifndef STRATUS_HLS
// 			if (forward_success_rs1 || forward_success_rs2)
// 				std::cout << green;
// 			std::cout << "  **REGFILE** ";
// 			std::cout << def;
// 			int id1 = rs1_addr;
// 			int id2 = rs2_addr;
// 			int raw_id1 = rs1_raw ? id1 : -1;
// 			int raw_id2 = rs2_raw ? id2 : -1;
// 			for(int i = 0; i < REG_NUM; ) {
// 				for (int j = 0; j < 8; j++) {
// 					int r = regfile[i].to_int();
// 					if (sentinel[i] != SENTINEL_INIT && (raw_id1 == i || raw_id2 == i) && !freeze) {
// 						std::cout << " " << green << std::right << std::setfill(' ') << std::setw(2) << i << ": 0x" << std::dec << std::left << std::setfill(' ') << std::setw(10) << r << std::dec << def;
// 					} else if (sentinel[i] != SENTINEL_INIT && (raw_id1 == i || raw_id2 == i)) {
// 						std::cout << " " << red << std::right << std::setfill(' ') << std::setw(2) << i << ": 0x" << std::dec << std::left << std::setfill(' ') << std::setw(10) << r << std::dec << def;
// 					} else if (sentinel[i] != SENTINEL_INIT) {
// 						std::cout << " " << yellow << std::right << std::setfill(' ') << std::setw(2) << i << ": 0x" << std::dec << std::left << std::setfill(' ') << std::setw(10) << r << std::dec << def;
// 					} else {
// 						std::cout << " " << std::right << std::setfill(' ') << std::setw(2) << i << ": 0x" << std::dec << std::left << std::setfill(' ') << std::setw(10) << r << std::dec;
// 					}
// 					i++;
// 					if (i == REG_NUM)
// 						break;
// 				}
// 				std::cout << endl;
// 				if (i == REG_NUM)
// 					break;
// 				std::cout << "                                                             ";
// 			}
// #endif

// #endif

		// *** Transform instruction into nop when freeze is active
		if (freeze) {
			// Bubble.
			output.regwrite     = "0";
			output.ld           = NO_LOAD;
			output.st           = NO_STORE;		

			debug_dout_t.regwrite = "REGWRITE NO";
			debug_dout_t.ld = "NO_LOAD";
			debug_dout_t.st = "NO_STORE";
		}

		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "freeze= " << freeze << endl);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << std::hex << "pc= " << debug_dout_t.pc << endl);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "regwrite= " << debug_dout_t.regwrite << endl);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "memtoreg= " << debug_dout_t.memtoreg << endl);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "ld= " << debug_dout_t.ld << endl);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "st= " << debug_dout_t.st << endl);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "alu_op= " << debug_dout_t.alu_op << endl);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "alu_src= " << debug_dout_t.alu_src << endl);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "rs1= " << debug_dout_t.rs1 << endl);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "dest_reg= " << debug_dout_t.dest_reg << endl);
		DPRINT("@" << sc_time_stamp() << "\t" << name() << "\t" << "imm_u= " << debug_dout_t.imm_u << endl);

		for(int i = 0; i < REG_NUM; ) {
			DPRINT(endl);
			for (int j = 0; j < 8; j++) {
				int r = regfile[i].to_int();
				DPRINT(" " << std::right << std::setfill(' ') << std::setw(2) << i << ": 0x" << std::hex << std::left << std::setfill(' ') << std::setw(10) << r << std::dec);
				i++;
				if (i == REG_NUM)
					break;
			}
			DPRINT(std::endl);
			if (i == REG_NUM)
				break;
		}

		dout.Push(output);

	} // *** ENDOF while(true)
}   // *** ENDOF sc_cthread


// --- Utility functions.

// Sign extend UJ insn.
sc_bv<PC_LEN> fedec::sign_extend_jump(sc_bv<21> imm)
{
	if (imm.range(20, 20) == "1")
		return ("11111111111", imm);
	else
		return ("00000000000", imm);
}

// Sign extend branch insn.
sc_bv<PC_LEN> fedec::sign_extend_branch(sc_bv<13> imm)
{
	if (imm.range(12, 12) == "1")
		return ("1111111111111111111", imm);
	else
		return ("0000000000000000000", imm);
}

// --- End of utility functions.
