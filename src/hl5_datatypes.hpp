/* Copyright 2017 Columbia University, SLD Group */

//
// hl5_datatypes.h - Robert Margelli
// Definition of custom data structs for storing and exchanging
// data among pipeline stages.
// Besides struct fields, all required operators for using them on Flex Channels are defined.
// Additionally, although not currently useful (but it may be in future works) operators for
// cynware metaports are defined.
//

#ifndef HL5_DATATYPES_H
#define HL5_DATATYPES_H

#include <systemc.h>
#include <mc_connections.h>

#include "defines.hpp"
#include "globals.hpp"

// Fetch
// ------------ fe_in_t
#ifndef de_in_t_SC_WRAPPER_TYPE
#define de_in_t_SC_WRAPPER_TYPE 1

struct de_in_t
{
	//
	// Member declarations.
	//
	sc_bv< 1 > jump;
	sc_bv< 1 > branch;
	sc_bv< PC_LEN > jump_address;
	sc_bv< PC_LEN > branch_address;

	static const int width = 2 + 2*PC_LEN;

	//
	// Default constructor.
	//
	de_in_t()
		{
			jump    = "0";
			branch  = "0";
			jump_address = (sc_bv< PC_LEN >)0;
			branch_address = (sc_bv< PC_LEN >)0;
		}

	//
	// Copy constructor.
	//
	de_in_t( const de_in_t& other )
		{
			jump = other.jump;
			branch = other.branch;
			jump_address = other.jump_address;
			branch_address = other.branch_address;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const de_in_t& other )
		{
			if ( !(jump == other.jump) )
				return false;
			if ( !(branch == other.branch) )
				return false;
			if ( !(jump_address == other.jump_address) )
				return false;
			if ( !(branch_address == other.branch_address) )
				return false;
			return true;
		}

	//
	// Assignment operator from fe_in_t.
	//
	inline de_in_t& operator = ( const de_in_t& other )
		{
			jump = other.jump;
			branch = other.branch;
			jump_address = other.jump_address;
			branch_address = other.branch_address;
			return *this;
		}
	
	
	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& jump; 
		m& branch;
		m& jump_address;
		m& branch_address;	   
  	}	
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const de_in_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.jump, in_name + std::string(".jump"));
		sc_trace(tf, object.branch, in_name + std::string(".branch"));
		sc_trace(tf, object.jump_address, in_name + std::string(".jump_address"));
		sc_trace(tf, object.branch_address, in_name + std::string(".branch_address"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const de_in_t& object )
	{

		os << "(";
		os <<  object.jump;
		os << "," <<  object.branch;
		os << "," <<  object.jump_address;
		os << "," <<  object.branch_address;
		os << ")";

		return os;
	}

};

#endif
// ------------ END fe_in_t

// ------------ fe_out_t
#ifndef fe_out_t_SC_WRAPPER_TYPE
#define fe_out_t_SC_WRAPPER_TYPE 1

struct fe_out_t
{
	//
	// Member declarations.
	//
	sc_uint< PC_LEN > pc;
	sc_uint< PC_LEN > next_pc;

	static const int width = 2*PC_LEN;

	//
	// Default constructor.
	//
	fe_out_t()
		{
			pc = (sc_uint< PC_LEN >)0;
			next_pc = (sc_uint< PC_LEN >)0;
		}

	//
	// Copy constructor.
	//
	fe_out_t( const fe_out_t& other )
		{
			pc = other.pc;
			next_pc = other.next_pc;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const fe_out_t& other )
		{
			if ( !(pc == other.pc) )
				return false;
			if ( !(next_pc == other.next_pc) )
				return false;
			return true;
		}

	//
	// Assignment operator from fe_in_t.
	//
	inline fe_out_t& operator = ( const fe_out_t& other )
		{
			pc = other.pc;
			next_pc = other.next_pc;
			return *this;
		}
	
	
	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& pc; 
		m& next_pc;	   
  	}	
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const fe_out_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.pc, in_name + std::string(".pc"));
		sc_trace(tf, object.next_pc, in_name + std::string(".next_pc"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const fe_out_t& object )
	{

		os << "(";
		os <<  object.pc;
		os <<  object.next_pc;
		os << ")";

		return os;
	}

};

#endif
// ------------ END fe_out_t


// Decode
// ------------ de_out_t
#ifndef de_out_t_SC_WRAPPER_TYPE
#define de_out_t_SC_WRAPPER_TYPE 1

struct de_out_t
{
	//
	// Member declarations.
	//
	sc_bv< 1 > regwrite;
	sc_bv< 1 > memtoreg;
	sc_bv< 3 > ld;
	sc_bv< 2 > st;
	sc_bv< ALUOP_SIZE > alu_op;
	sc_bv< ALUSRC_SIZE > alu_src;
	sc_bv< XLEN > rs1;
	sc_bv< XLEN > rs2;
	sc_bv< REG_ADDR > dest_reg;
	sc_bv< PC_LEN > pc;
	sc_bv< XLEN-12 > imm_u;
	sc_uint< TAG_WIDTH > tag;

	static const int width = 1 + 1 + 3 + 2 + ALUOP_SIZE + ALUSRC_SIZE + 3*XLEN - 12 + REG_ADDR + PC_LEN + TAG_WIDTH;

	//
	// Default constructor.
	//
	de_out_t()
		{
			regwrite    = "0";
			memtoreg    = "0";
			ld          = NO_LOAD;
			st          = NO_STORE;
			alu_op      = (sc_bv< ALUOP_SIZE >)0;
			alu_src     = (sc_bv< ALUSRC_SIZE >)0;
			rs1         = (sc_bv< XLEN >)0;
			rs2         = (sc_bv< XLEN >)0;
			dest_reg    = (sc_bv< REG_ADDR >)0;
			pc          = (sc_bv< PC_LEN >)0;
			imm_u       = (sc_bv< XLEN-12 >)0;
			tag         = (sc_uint< TAG_WIDTH >)0;
		}

	//
	// Copy constructor.
	//
	de_out_t( const de_out_t& other )
		{
			regwrite = other.regwrite;
			memtoreg = other.memtoreg;
			ld = other.ld;
			st = other.st;
			alu_op = other.alu_op;
			alu_src = other.alu_src;
			rs1 = other.rs1;
			rs2 = other.rs2;
			dest_reg = other.dest_reg;
			pc = other.pc;
			imm_u = other.imm_u;
			tag = other.tag;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const de_out_t& other )
		{
			if ( !(regwrite == other.regwrite) )
				return false;
			if ( !(memtoreg == other.memtoreg) )
				return false;
			if ( !(ld == other.ld) )
				return false;
			if ( !(st == other.st) )
				return false;
			if ( !(alu_op == other.alu_op) )
				return false;
			if ( !(alu_src == other.alu_src) )
				return false;
			if ( !(rs1 == other.rs1) )
				return false;
			if ( !(rs2 == other.rs2) )
				return false;
			if ( !(dest_reg == other.dest_reg) )
				return false;
			if ( !(pc == other.pc) )
				return false;
			if ( !(imm_u == other.imm_u) )
				return false;
			if ( !(tag == other.tag) )
				return false;
			return true;
		}

	//
	// Assignment operator from de_out_t.
	//
	inline de_out_t& operator = ( const de_out_t& other )
		{
			regwrite = other.regwrite;
			memtoreg = other.memtoreg;
			ld = other.ld;
			st = other.st;
			alu_op = other.alu_op;
			alu_src = other.alu_src;
			rs1 = other.rs1;
			rs2 = other.rs2;
			dest_reg = other.dest_reg;
			pc = other.pc;
			imm_u = other.imm_u;
			tag = other.tag;
			return *this;
		}
	
	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& regwrite; 
		m& memtoreg;
		m& ld;
		m& st;
		m& alu_op;
		m& alu_src;
		m& rs1;
		m& rs2;
		m& dest_reg;
		m& pc;
		m& imm_u;
		m& tag;

  	}
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const de_out_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.regwrite, in_name + std::string(".regwrite"));
		sc_trace(tf, object.memtoreg, in_name + std::string(".memtoreg"));
		sc_trace(tf, object.ld, in_name + std::string(".ld"));
		sc_trace(tf, object.st, in_name + std::string(".st"));
		sc_trace(tf, object.alu_op, in_name + std::string(".alu_op"));
		sc_trace(tf, object.alu_src, in_name + std::string(".alu_src"));
		sc_trace(tf, object.rs1, in_name + std::string(".rs1"));
		sc_trace(tf, object.rs2, in_name + std::string(".rs2"));
		sc_trace(tf, object.dest_reg, in_name + std::string(".dest_reg"));
		sc_trace(tf, object.pc, in_name + std::string(".pc"));
		sc_trace(tf, object.imm_u, in_name + std::string(".imm_u"));
		sc_trace(tf, object.tag, in_name + std::string(".tag"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const de_out_t& object )
	{
		os << "(";
		os <<  object.regwrite;
		os << "," <<  object.memtoreg;
		os << "," <<  object.ld;
		os << "," <<  object.st;
		os << "," <<  object.alu_op;
		os << "," <<  object.alu_src;
		os << "," <<  object.rs1;
		os << "," <<  object.rs2;
		os << "," <<  object.dest_reg;
		os << "," <<  object.pc;
		os << "," <<  object.imm_u;
		os << "," <<  object.tag;
		os << ")";

		return os;
	}

};

#endif
// ------------ END de_out_t


// Execute
// ------------ exe_out_t
#ifndef exe_out_t_SC_WRAPPER_TYPE
#define exe_out_t_SC_WRAPPER_TYPE 1

struct exe_out_t        // TODO: fix all sizes
{
	//
	// Member declarations.
	//
	sc_bv< 3 > ld;
	sc_bv< 2 > st;
	sc_bv< 1 > memtoreg;
	sc_bv< 1 > regwrite;
	sc_bv< XLEN > alu_res;
	sc_bv< DATA_SIZE > mem_datain;
	sc_bv< REG_ADDR > dest_reg;
	sc_uint< TAG_WIDTH > tag;

	static const int width = 3 + 2 + 1 + 1 + XLEN + DATA_SIZE + REG_ADDR + TAG_WIDTH;

	//
	// Default constructor.
	//
	exe_out_t()
		{
			ld  = NO_LOAD;
			st  = NO_STORE;
			memtoreg    = "0";
			regwrite    = "0";
			alu_res     = (sc_bv< XLEN >)0;
			mem_datain  = (sc_bv< DATA_SIZE >)0;
			dest_reg    = (sc_bv< REG_ADDR >)0;
			tag         = (sc_uint< TAG_WIDTH >)0;
		}

	//
	// Copy constructor.
	//
	exe_out_t( const exe_out_t& other )
		{
			ld = other.ld;
			st = other.st;
			memtoreg = other.memtoreg;
			regwrite = other.regwrite;
			alu_res = other.alu_res;
			mem_datain = other.mem_datain;
			dest_reg = other.dest_reg;
			tag = other.tag;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const exe_out_t& other )
		{
			if ( !(ld == other.ld) )
				return false;
			if ( !(st == other.st) )
				return false;
			if ( !(memtoreg == other.memtoreg) )
				return false;
			if ( !(regwrite == other.regwrite) )
				return false;
			if ( !(alu_res == other.alu_res) )
				return false;
			if ( !(mem_datain == other.mem_datain) )
				return false;
			if ( !(dest_reg == other.dest_reg) )
				return false;
			if ( !(tag == other.tag) )
				return false;
			return true;
		}

	//
	// Assignment operator from exe_out_t.
	//
	inline exe_out_t& operator = ( const exe_out_t& other )
		{
			ld = other.ld;
			st = other.st;
			memtoreg = other.memtoreg;
			regwrite = other.regwrite;
			alu_res = other.alu_res;
			mem_datain = other.mem_datain;
			dest_reg = other.dest_reg;
			tag = other.tag;
			return *this;
		}

	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& ld; 
		m& st;
		m& memtoreg;
		m& regwrite;
		m& alu_res;
		m& mem_datain;
		m& dest_reg;
		m& tag;

  	}
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const exe_out_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.ld, in_name + std::string(".ld"));
		sc_trace(tf, object.st, in_name + std::string(".st"));
		sc_trace(tf, object.memtoreg, in_name + std::string(".memtoreg"));
		sc_trace(tf, object.regwrite, in_name + std::string(".regwrite"));
		sc_trace(tf, object.alu_res, in_name + std::string(".alu_res"));
		sc_trace(tf, object.mem_datain, in_name + std::string(".mem_datain"));
		sc_trace(tf, object.dest_reg, in_name + std::string(".dest_reg"));
		sc_trace(tf, object.tag, in_name + std::string(".tag"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const exe_out_t& object )
	{
		os << "(";
		os <<  object.ld;
		os << "," <<  object.st;
		os << "," <<  object.memtoreg;
		os << "," <<  object.regwrite;
		os << "," <<  object.alu_res;
		os << "," <<  object.mem_datain;
		os << "," <<  object.dest_reg;
		os << "," <<  object.tag;
		os << ")";

		return os;
	}

};

#endif
// ------------ END exe_out_t


// Memory
// ------------ mem_out_t
#ifndef mem_out_t_SC_WRAPPER_TYPE
#define mem_out_t_SC_WRAPPER_TYPE 1

struct mem_out_t
{
	//
	// Member declarations.
	//
	sc_bv< 1 > regwrite;
	sc_bv< REG_ADDR > regfile_address;
	sc_bv< XLEN > regfile_data;
	sc_uint< TAG_WIDTH > tag;

	static const int width = 1 + REG_ADDR + XLEN + TAG_WIDTH;
	//
	// Default constructor.
	//
	mem_out_t()
		{
			regwrite    = "0";
			regfile_address = (sc_bv< REG_ADDR >)0;
			regfile_data    = (sc_bv< XLEN >)0;
			tag             = (sc_uint< TAG_WIDTH >)0;
		}

	//
	// Copy constructor.
	//
	mem_out_t( const mem_out_t& other )
		{
			regwrite = other.regwrite;
			regfile_address = other.regfile_address;
			regfile_data = other.regfile_data;
			tag = other.tag;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const mem_out_t& other )
		{
			if ( !(regwrite == other.regwrite) )
				return false;
			if ( !(regfile_data == other.regfile_data) )
				return false;
			if ( !(regfile_address == other.regfile_address) )
				return false;
			if ( !(tag == other.tag) )
				return false;
			return true;
		}

	//
	// Assignment operator from mem_out_t.
	//
	inline mem_out_t& operator = ( const mem_out_t& other )
		{
			regwrite = other.regwrite;
			regfile_address = other.regfile_address;
			regfile_data = other.regfile_data;
			tag = other.tag;
			return *this;
		}

	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& regwrite;
    	m& regfile_address;
		m& regfile_data;
		m& tag;

  	}
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const mem_out_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.regwrite, in_name + std::string(".regwrite"));
		sc_trace(tf, object.regfile_address, in_name + std::string(".regfile_address"));
		sc_trace(tf, object.regfile_data, in_name + std::string(".regfile_data"));
		sc_trace(tf, object.tag, in_name + std::string(".tag"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const mem_out_t& object )
	{
		os << "(";
		os <<  object.regwrite;
		os << "," <<  object.regfile_address;
		os << "," <<  object.regfile_data;
		os << "," <<  object.tag;
		os << ")";
		return os;
	}

};
#endif
// ------------ mem_out_t


// Forward
// ------------ reg_forward_t
#ifndef reg_forward_t_SC_WRAPPER_TYPE
#define reg_forward_t_SC_WRAPPER_TYPE 1

struct reg_forward_t
{
	//
	// Member declarations.
	//
	sc_bv< XLEN > regfile_data;
	bool ldst;
	bool sync_fewb;
	sc_uint< TAG_WIDTH > tag;

	static const int width = XLEN + 1 + TAG_WIDTH + 1;
	//
	// Default constructor.
	//
	reg_forward_t()
		{
			regfile_data    = (sc_bv< XLEN >)0;
			ldst            = false;
			sync_fewb       = false;
			tag             = (sc_uint< TAG_WIDTH >)0;
		}

	//
	// Copy constructor.
	//
	reg_forward_t( const reg_forward_t& other )
		{
			regfile_data = other.regfile_data;
			ldst = other.ldst;
			sync_fewb = other.sync_fewb;
			tag = other.tag;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const reg_forward_t& other )
		{
			if ( !(regfile_data == other.regfile_data) )
				return false;
			if ( !(ldst == other.ldst) )
				return false;
			if ( !(sync_fewb == other.sync_fewb) )
				return false;
			if ( !(tag == other.tag) )
				return false;
			return true;
		}

	//
	// Assignment operator from reg_forward_t.
	//
	inline reg_forward_t& operator = ( const reg_forward_t& other )
		{
			regfile_data = other.regfile_data;
			ldst = other.ldst;
			sync_fewb = other.sync_fewb;
			tag = other.tag;
			return *this;
		}

	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& regfile_data; 
		m& ldst;
		m& sync_fewb;
		m& tag;

  	}
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const reg_forward_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.regfile_data, in_name + std::string(".regfile_data"));
		sc_trace(tf, object.ldst, in_name + std::string(".ldst"));
		sc_trace(tf, object.sync_fewb, in_name + std::string(".sync_fewb"));
		sc_trace(tf, object.tag, in_name + std::string(".tag"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const reg_forward_t& object )
	{
		os << "(";
		os <<  std::hex << object.regfile_data.to_uint() << std::dec;
		if (object.ldst)
			os << "," << " mem";
		os << "," <<  object.tag;
		os << "," <<  object.sync_fewb;
		os << ")";
		return os;
	}

};


#endif
// ------------ reg_forward_t

// IMEMORY
// ------------ imem_in_t
#ifndef imem_in_t_SC_WRAPPER_TYPE
#define imem_in_t_SC_WRAPPER_TYPE 1

struct imem_in_t
{
	//
	// Member declarations.
	//
	sc_uint< XLEN > instr_addr;
	bool valid;
	
	static const int width = 1 + XLEN;
	//
	// Default constructor.
	//
	imem_in_t()
		{
			instr_addr    = (sc_uint< XLEN >)0;
			valid 		  = false;
		}

	//
	// Copy constructor.
	//
	imem_in_t( const imem_in_t& other )
		{
			instr_addr = other.instr_addr;
			valid = other.valid;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const imem_in_t& other )
		{
			if ( !(instr_addr == other.instr_addr) )
				return false;
			if ( !(valid == other.valid) )
				return false;
			return true;
		}

	//
	// Assignment operator from imem_in_t.
	//
	inline imem_in_t& operator = ( const imem_in_t& other )
		{
			instr_addr = other.instr_addr;
			valid = other.valid;
			return *this;
		}

	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& instr_addr; 
		m& valid;
  	}
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const imem_in_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.instr_addr, in_name + std::string(".instr_addr"));
		sc_trace(tf, object.valid, in_name + std::string(".valid"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const imem_in_t& object )
	{
		os << "(";
		os <<  object.instr_addr;
		os << "," <<  object.valid;
		os << ")";
		return os;
	}

};
#endif
// ------------ imem_in_t

// ------------ imem_out_t
#ifndef imem_out_t_SC_WRAPPER_TYPE
#define imem_out_t_SC_WRAPPER_TYPE 1

struct imem_out_t
{
	//
	// Member declarations.
	//
	sc_uint< XLEN > instr_data;
	
	static const int width = XLEN;
	//
	// Default constructor.
	//
	imem_out_t()
		{
			instr_data    = (sc_uint< XLEN >)0;
		}

	//
	// Copy constructor.
	//
	imem_out_t( const imem_out_t& other )
		{
			instr_data = other.instr_data;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const imem_out_t& other )
		{
			if ( !(instr_data == other.instr_data) )
				return false;
			return true;
		}

	//
	// Assignment operator from imem_out_t.
	//
	inline imem_out_t& operator = ( const imem_out_t& other )
		{
			instr_data = other.instr_data;
			return *this;
		}

	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& instr_data; 
  	}
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const imem_out_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.instr_data, in_name + std::string(".instr_data"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const imem_out_t& object )
	{
		os << "(";
		os <<  object.instr_data;
		os << ")";
		return os;
	}

};
#endif
// ------------ imem_out_t

// ------------ dmem_in_t
#ifndef dmem_in_t_SC_WRAPPER_TYPE
#define dmem_in_t_SC_WRAPPER_TYPE 1

struct dmem_in_t
{
	//
	// Member declarations.
	//
	sc_uint< XLEN > data_addr;
	sc_uint< XLEN > data_in;
	bool valid;
	bool read_en;
	bool write_en;
	
	static const int width = 2*XLEN + 3;
	//
	// Default constructor.
	//
	dmem_in_t()
		{
			data_addr    = (sc_uint< XLEN >)0;
			data_in    = (sc_uint< XLEN >)0;
			valid		 = false;
			read_en		 = false;
			write_en		 = false;
		}

	//
	// Copy constructor.
	//
	dmem_in_t( const dmem_in_t& other )
		{
			data_addr = other.data_addr;
			data_in = other.data_in;
			valid = other.valid;
			read_en = other.read_en;
			write_en = other.write_en;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const dmem_in_t& other )
		{
			if ( !(data_addr == other.data_addr) )
				return false;
			if ( !(data_in == other.data_in) )
				return false;
			if ( !(valid == other.valid) )
				return false;
			if ( !(read_en == other.read_en) )
				return false;
			if ( !(write_en == other.write_en) )
				return false;
			return true;
		}

	//
	// Assignment operator from dmem_in_t.
	//
	inline dmem_in_t& operator = ( const dmem_in_t& other )
		{
			data_addr = other.data_addr;
			data_in = other.data_in;
			valid = other.valid;
			read_en = other.read_en;
			write_en = other.write_en;
			return *this;
		}

	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& data_addr;
		m& data_in; 
		m& valid; 
		m& read_en;
		m& write_en; 
  	}
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const dmem_in_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.data_addr, in_name + std::string(".data_addr"));
		sc_trace(tf, object.data_in, in_name + std::string(".data_in"));
		sc_trace(tf, object.valid, in_name + std::string(".valid"));
		sc_trace(tf, object.read_en, in_name + std::string(".read_en"));
		sc_trace(tf, object.write_en, in_name + std::string(".write_en"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const dmem_in_t& object )
	{
		os << "(";
		os <<  object.data_addr;
		os <<  object.data_in;
		os <<  object.valid;
		os <<  object.read_en;
		os <<  object.write_en;
		os << ")";
		return os;
	}

};
#endif
// ------------ dmem_in_t


// ------------ dmem_out_t
#ifndef dmem_out_t_SC_WRAPPER_TYPE
#define dmem_out_t_SC_WRAPPER_TYPE 1

struct dmem_out_t
{
	//
	// Member declarations.
	//
	sc_uint< XLEN > data_out;
	bool			valid;
	
	static const int width = XLEN + 1;
	//
	// Default constructor.
	//
	dmem_out_t()
		{
			data_out    = (sc_uint< XLEN >)0;
			valid		= false;
		}

	//
	// Copy constructor.
	//
	dmem_out_t( const dmem_out_t& other )
		{
			data_out = other.data_out;
			valid	 = other.valid;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const dmem_out_t& other )
		{
			if ( !(data_out == other.data_out) )
				return false;
			if ( !(valid == other.valid) )
				return false;
			return true;
		}

	//
	// Assignment operator from dmem_out_t.
	//
	inline dmem_out_t& operator = ( const dmem_out_t& other )
		{
			data_out = other.data_out;
			valid 	 = other.valid;
			return *this;
		}

	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& data_out;
		m& valid;
  	}
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const dmem_out_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.data_out, in_name + std::string(".data_out"));
		sc_trace(tf, object.valid, in_name + std::string(".valid"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const dmem_out_t& object )
	{
		os << "(";
		os <<  object.data_out;
		os <<  object.valid;
		os << ")";
		return os;
	}

};
#endif

// ------------ dmem_out_t
#ifndef fe_in_t_SC_WRAPPER_TYPE
#define fe_in_t_SC_WRAPPER_TYPE 1

struct fe_in_t
{
	//
	// Member declarations.
	//
	bool			freeze;
	bool 		    redirect;
	sc_bv< PC_LEN > address;
	
	static const int width = 1 + 1 + PC_LEN;
	//
	// Default constructor.
	//
	fe_in_t()
		{
			freeze             = false;
			redirect  		   = false;
			address   		   = sc_bv< PC_LEN >(0);
		}

	//
	// Copy constructor.
	//
	fe_in_t( const fe_in_t& other )
		{
			freeze    = other.freeze;
			redirect  = other.redirect;
			address   = other.address;
		}

	//
	// Comparison operator.
	//
	inline bool operator == ( const fe_in_t& other )
		{
			if ( !(freeze == other.freeze) )
				return false;
			if ( !(redirect == other.redirect) )
				return false;
			if ( !(address == other.address) )
				return false;	
			return true;
		}

	//
	// Assignment operator from stall_t.
	//
	inline fe_in_t& operator = ( const fe_in_t& other )
		{
			freeze				= other.freeze;
			redirect			= other.redirect;
			address	    		= other.address;

			return *this;
		}

	template<unsigned int Size>
  	void Marshall(Marshaller<Size>& m) {
    	m& freeze;
		m& redirect;
		m& address;
  	}
  	
  	//
	// sc_trace function.
	//
	inline friend void sc_trace( sc_trace_file* tf, const fe_in_t& object, const std::string& in_name )
	{
		sc_trace(tf, object.freeze, in_name + std::string(".freeze"));
		sc_trace(tf, object.redirect, in_name + std::string(".redirect"));
		sc_trace(tf, object.address, in_name + std::string(".address"));
	}
	
	//
	// stream operator.
	//
	inline friend ostream & operator << ( ostream & os, const fe_in_t& object )
	{
		os << "(";
		os <<  object.freeze;
		os <<  object.redirect;
		os <<  object.address;
		os << ")";
		return os;
	}

};
#endif

#endif  // ------------ hl5_datatypes.h include guard
