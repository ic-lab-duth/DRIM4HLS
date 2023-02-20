/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief 
    Definition of custom data structs for storing and exchanging
	data among pipeline stages.
	Besides struct fields, all required operators for using them on HLSLibs Channels are defined.

	@note Changes from HL5
		- Added custom datatypes

*/

#ifndef HL5_DATATYPES_H
#define HL5_DATATYPES_H

// Decode
// ------------ de_in_t
#ifndef de_in_t_SC_WRAPPER_TYPE
#define de_in_t_SC_WRAPPER_TYPE 1

#include "defines.h"
#include "globals.h"

#include <mc_connections.h>
#include <ac_int.h>

struct de_in_t {
    //
    // Member declarations.
    //
    ac_int < PC_LEN, false > jump_address;
    ac_int < PC_LEN, false > branch_address;

    //
    // Default constructor.
    //
    de_in_t() {
        jump_address = 0;
        branch_address = 0;
    }

    //
    // Copy constructor.
    //
    de_in_t(const de_in_t & other) {
        jump_address = other.jump_address;
        branch_address = other.branch_address;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const de_in_t & other) {
        if (!(jump_address == other.jump_address))
            return false;
        if (!(branch_address == other.branch_address))
            return false;
        return true;
    }

    //
    // Assignment operator from de_in_t.
    //
    inline de_in_t & operator = (const de_in_t & other) {
        jump_address = other.jump_address;
        branch_address = other.branch_address;
        return *this;
    }

};

#endif
// ------------ END de_in_t

// ------------ fe_out_t
#ifndef fe_out_t_SC_WRAPPER_TYPE
#define fe_out_t_SC_WRAPPER_TYPE 1

struct fe_out_t {
    //
    // Member declarations.
    //
    ac_int < PC_LEN, false > pc;
    ac_int < XLEN, false > instr_data;

    static const int width = PC_LEN + XLEN;

    //
    // Default constructor.
    //
    fe_out_t() {
        pc = 0;
        instr_data = 0;
    }

    //
    // Copy constructor.
    //
    fe_out_t(const fe_out_t & other) {
        pc = other.pc;
        instr_data = other.instr_data;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const fe_out_t & other) {
        if (!(pc == other.pc))
            return false;
        if (!(instr_data == other.instr_data))
            return false;
        return true;
    }

    //
    // Assignment operator from fe_out_t.
    //
    inline fe_out_t & operator = (const fe_out_t & other) {
        pc = other.pc;
        instr_data = other.instr_data;
        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & pc;
            m & instr_data;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const fe_out_t & object, const std::string & in_name) {
        sc_trace(tf, object.pc, in_name + std::string(".pc"));
        sc_trace(tf, object.instr_data, in_name + std::string(".instr_data"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const fe_out_t & object) {

        os << "(";
        os << object.pc;
        os << object.instr_data;
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

struct de_out_t {
    //
    // Member declarations.
    //
    ac_int < 1, false > regwrite;
    ac_int < 1, false > memtoreg;
    ac_int < 3, false > ld;
    ac_int < 2, false > st;
    ac_int < ALUOP_SIZE, false > alu_op;
    ac_int < ALUSRC_SIZE, false > alu_src;
    ac_int < XLEN, true > rs1;
    ac_int < XLEN, true > rs2;
    ac_int < REG_ADDR, false > dest_reg;
    ac_int < PC_LEN, false > pc;
    ac_int < XLEN - 12, false > imm_u;
    ac_int < TAG_WIDTH, false > tag;

    static
    const int width = 1 + 1 + 3 + 2 + ALUOP_SIZE + ALUSRC_SIZE + 3 * XLEN - 12 + REG_ADDR + PC_LEN + TAG_WIDTH;

    //
    // Default constructor.
    //
    de_out_t() {
        regwrite = 0;
        memtoreg = 0;
        ld = NO_LOAD;
        st = NO_STORE;
        alu_op = 0;
        alu_src = 0;
        rs1 = 0;
        rs2 = 0;
        dest_reg = 0;
        pc = 0;
        imm_u = 0;
        tag = 0;
    }

    //
    // Copy constructor.
    //
    de_out_t(const de_out_t & other) {
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
    inline bool operator == (const de_out_t & other) {
        if (!(regwrite == other.regwrite))
            return false;
        if (!(memtoreg == other.memtoreg))
            return false;
        if (!(ld == other.ld))
            return false;
        if (!(st == other.st))
            return false;
        if (!(alu_op == other.alu_op))
            return false;
        if (!(alu_src == other.alu_src))
            return false;
        if (!(rs1 == other.rs1))
            return false;
        if (!(rs2 == other.rs2))
            return false;
        if (!(dest_reg == other.dest_reg))
            return false;
        if (!(pc == other.pc))
            return false;
        if (!(imm_u == other.imm_u))
            return false;
        if (!(tag == other.tag))
            return false;
        return true;
    }

    //
    // Assignment operator from de_out_t.
    //
    inline de_out_t & operator = (const de_out_t & other) {
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

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & regwrite;
            m & memtoreg;
            m & ld;
            m & st;
            m & alu_op;
            m & alu_src;
            m & rs1;
            m & rs2;
            m & dest_reg;
            m & pc;
            m & imm_u;
            m & tag;

        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf,
        const de_out_t & object,
            const std::string & in_name) {
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
    inline friend ostream & operator << (ostream & os, const de_out_t & object) {
        os << "(";
        os << object.regwrite;
        os << "," << object.memtoreg;
        os << "," << object.ld;
        os << "," << object.st;
        os << "," << object.alu_op;
        os << "," << object.alu_src;
        os << "," << object.rs1;
        os << "," << object.rs2;
        os << "," << object.dest_reg;
        os << "," << object.pc;
        os << "," << object.imm_u;
        os << "," << object.tag;
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

struct exe_out_t // TODO: fix all sizes
{
    //
    // Member declarations.
    //
    ac_int < 3, false > ld;
    ac_int < 2, false > st;
    ac_int < 1, false > memtoreg;
    ac_int < 1, false > regwrite;
    ac_int < XLEN, false > alu_res;
    ac_int < DATA_SIZE, true > mem_datain;
    ac_int < REG_ADDR, false > dest_reg;
    ac_int < TAG_WIDTH, false > tag;
    ac_int < PC_LEN, false > pc;

    static const int width = 3 + 2 + 1 + 1 + XLEN + DATA_SIZE + REG_ADDR + TAG_WIDTH + PC_LEN;

    //
    // Default constructor.
    //
    exe_out_t() {
        ld = NO_LOAD;
        st = NO_STORE;
        memtoreg = 0;
        regwrite = 0;
        alu_res = 0;
        mem_datain = 0;
        dest_reg = 0;
        tag = 0;
        pc = 0;
    }

    //
    // Copy constructor.
    //
    exe_out_t(const exe_out_t & other) {
        ld = other.ld;
        st = other.st;
        memtoreg = other.memtoreg;
        regwrite = other.regwrite;
        alu_res = other.alu_res;
        mem_datain = other.mem_datain;
        dest_reg = other.dest_reg;
        tag = other.tag;
        pc = other.pc;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const exe_out_t & other) {
        if (!(ld == other.ld))
            return false;
        if (!(st == other.st))
            return false;
        if (!(memtoreg == other.memtoreg))
            return false;
        if (!(regwrite == other.regwrite))
            return false;
        if (!(alu_res == other.alu_res))
            return false;
        if (!(mem_datain == other.mem_datain))
            return false;
        if (!(dest_reg == other.dest_reg))
            return false;
        if (!(tag == other.tag))
            return false;
        if (!(pc == other.pc))
            return false;
        return true;
    }

    //
    // Assignment operator from exe_out_t.
    //
    inline exe_out_t & operator = (const exe_out_t & other) {
        ld = other.ld;
        st = other.st;
        memtoreg = other.memtoreg;
        regwrite = other.regwrite;
        alu_res = other.alu_res;
        mem_datain = other.mem_datain;
        dest_reg = other.dest_reg;
        tag = other.tag;
        pc = other.pc;
        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & ld;
            m & st;
            m & memtoreg;
            m & regwrite;
            m & alu_res;
            m & mem_datain;
            m & dest_reg;
            m & tag;
            m & pc;

        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const exe_out_t & object, const std::string & in_name) {
        sc_trace(tf, object.ld, in_name + std::string(".ld"));
        sc_trace(tf, object.st, in_name + std::string(".st"));
        sc_trace(tf, object.memtoreg, in_name + std::string(".memtoreg"));
        sc_trace(tf, object.regwrite, in_name + std::string(".regwrite"));
        sc_trace(tf, object.alu_res, in_name + std::string(".alu_res"));
        sc_trace(tf, object.mem_datain, in_name + std::string(".mem_datain"));
        sc_trace(tf, object.dest_reg, in_name + std::string(".dest_reg"));
        sc_trace(tf, object.tag, in_name + std::string(".tag"));
        sc_trace(tf, object.pc, in_name + std::string(".pc"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os, const exe_out_t & object) {
        os << "(";
        os << object.ld;
        os << "," << object.st;
        os << "," << object.memtoreg;
        os << "," << object.regwrite;
        os << "," << object.alu_res;
        os << "," << object.mem_datain;
        os << "," << object.dest_reg;
        os << "," << object.tag;
        os << "," << object.pc;
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

struct mem_out_t {
    //
    // Member declarations.
    //
    ac_int < 1, false > regwrite;
    ac_int < REG_ADDR, false > regfile_address;
    ac_int < XLEN, true > regfile_data;
    ac_int < TAG_WIDTH, false > tag;
    ac_int < PC_LEN, false > pc;

    static const int width = 1 + REG_ADDR + XLEN + TAG_WIDTH + PC_LEN;
    //
    // Default constructor.
    //
    mem_out_t() {
        regwrite = 0;
        regfile_address = 0;
        regfile_data = 0;
        tag = 0;
        pc = 0;
    }

    //
    // Copy constructor.
    //
    mem_out_t(const mem_out_t & other) {
        regwrite = other.regwrite;
        regfile_address = other.regfile_address;
        regfile_data = other.regfile_data;
        tag = other.tag;
        pc = other.pc;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const mem_out_t & other) {
        if (!(regwrite == other.regwrite))
            return false;
        if (!(regfile_address == other.regfile_address))
            return false;
        if (!(regfile_data == other.regfile_data))
            return false;
        if (!(tag == other.tag))
            return false;
        if (!(pc == other.pc))
            return false;
        return true;
    }

    //
    // Assignment operator from mem_out_t.
    //
    inline mem_out_t & operator = (const mem_out_t & other) {
        regwrite = other.regwrite;
        regfile_address = other.regfile_address;
        regfile_data = other.regfile_data;
        tag = other.tag;
        pc = other.pc;
        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & regwrite;
            m & regfile_address;
            m & regfile_data;
            m & tag;
            m & pc;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const mem_out_t & object, const std::string & in_name) {
        sc_trace(tf, object.regwrite, in_name + std::string(".regwrite"));
        sc_trace(tf, object.regfile_address, in_name + std::string(".regfile_address"));
        sc_trace(tf, object.regfile_data, in_name + std::string(".regfile_data"));
        sc_trace(tf, object.tag, in_name + std::string(".tag"));
        sc_trace(tf, object.pc, in_name + std::string(".pc"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os, const mem_out_t & object) {
        os << "(";
        os << object.regwrite;
        os << "," << object.regfile_address;
        os << "," << object.regfile_data;
        os << "," << object.tag;
        os << "," << object.pc;
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

struct reg_forward_t {
    //
    // Member declarations.
    //
    ac_int < XLEN, true > regfile_data;
    bool ldst;
    bool sync_fewb;
    ac_int < TAG_WIDTH, false > tag;
    ac_int < PC_LEN, false > pc;

    static
    const int width = XLEN + 1 + 1 + TAG_WIDTH + PC_LEN;
    //
    // Default constructor.
    //
    reg_forward_t() {
        regfile_data = 0;
        ldst = false;
        sync_fewb = false;
        tag = 0;
        pc = 0;
    }

    //
    // Copy constructor.
    //
    reg_forward_t(const reg_forward_t & other) {
        regfile_data = other.regfile_data;
        ldst = other.ldst;
        sync_fewb = other.sync_fewb;
        tag = other.tag;
        pc = other.pc;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const reg_forward_t & other) {
        if (!(regfile_data == other.regfile_data))
            return false;
        if (!(ldst == other.ldst))
            return false;
        if (!(sync_fewb == other.sync_fewb))
            return false;
        if (!(tag == other.tag))
            return false;
        if (!(pc == other.pc))
            return false;
        return true;
    }

    //
    // Assignment operator from reg_forward_t.
    //
    inline reg_forward_t & operator = (const reg_forward_t & other) {
        regfile_data = other.regfile_data;
        ldst = other.ldst;
        sync_fewb = other.sync_fewb;
        tag = other.tag;
        pc = other.pc;
        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & regfile_data;
            m & ldst;
            m & sync_fewb;
            m & tag;
            m & pc;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf,
        const reg_forward_t & object,
            const std::string & in_name) {
        sc_trace(tf, object.regfile_data, in_name + std::string(".regfile_data"));
        sc_trace(tf, object.ldst, in_name + std::string(".ldst"));
        sc_trace(tf, object.sync_fewb, in_name + std::string(".sync_fewb"));
        sc_trace(tf, object.tag, in_name + std::string(".tag"));
        sc_trace(tf, object.pc, in_name + std::string(".pc"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const reg_forward_t & object) {
        os << "(";
        os << std::hex << object.regfile_data.to_uint() << std::dec;
        if (object.ldst)
            os << "," << " mem";
        os << "," << object.tag;
        os << "," << object.sync_fewb;
        os << "," << object.pc;
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

struct imem_in_t {
    //
    // Member declarations.
    //
    ac_int < XLEN, false > instr_addr;

    static const int width = XLEN;
    //
    // Default constructor.
    //
    imem_in_t() {
        instr_addr = 0;
    }

    //
    // Copy constructor.
    //
    imem_in_t(const imem_in_t & other) {
        instr_addr = other.instr_addr;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const imem_in_t & other) {
        if (!(instr_addr == other.instr_addr))
            return false;
        return true;
    }

    //
    // Assignment operator from imem_in_t.
    //
    inline imem_in_t & operator = (const imem_in_t & other) {
        instr_addr = other.instr_addr;
        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & instr_addr;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const imem_in_t & object, const std::string & in_name) {
        sc_trace(tf, object.instr_addr, in_name + std::string(".instr_addr"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os, const imem_in_t & object) {
        os << "(";
        os << object.instr_addr;
        os << ")";
        return os;
    }

};
#endif
// ------------ imem_in_t

// ------------ imem_out_t
#ifndef imem_out_t_SC_WRAPPER_TYPE
#define imem_out_t_SC_WRAPPER_TYPE 1

struct imem_out_t {
    //
    // Member declarations.
    //
    ac_int < ICACHE_LINE, false > instr_data;

    static const int width = ICACHE_LINE;
    //
    // Default constructor.
    //
    imem_out_t() {
        instr_data = 0;
    }

    //
    // Copy constructor.
    //
    imem_out_t(const imem_out_t & other) {
        instr_data = other.instr_data;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const imem_out_t & other) {
        if (!(instr_data == other.instr_data))
            return false;
        return true;
    }

    //
    // Assignment operator from imem_out_t.
    //
    inline imem_out_t & operator = (const imem_out_t & other) {
        instr_data = other.instr_data;
        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & instr_data;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const imem_out_t & object, const std::string & in_name) {
        sc_trace(tf, object.instr_data, in_name + std::string(".instr_data"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const imem_out_t & object) {
        os << "(";
        os << object.instr_data;
        os << ")";
        return os;
    }

};
#endif
// ------------ imem_out_t

// ------------ dmem_in_t
#ifndef dmem_in_t_SC_WRAPPER_TYPE
#define dmem_in_t_SC_WRAPPER_TYPE 1

struct dmem_in_t {
    //
    // Member declarations.
    //
    ac_int < XLEN, false > data_addr;
    ac_int < XLEN, false > write_addr;
    ac_int < DCACHE_LINE, false > data_in;
    bool read_en;
    bool write_en;

    static
    const int width = DCACHE_LINE + 2*XLEN + 2;
    //
    // Default constructor.
    //
    dmem_in_t() {
        data_addr = 0;
        write_addr = 0;
        data_in = 0;
        read_en = false;
        write_en = false;
    }

    //
    // Copy constructor.
    //
    dmem_in_t(const dmem_in_t & other) {
        data_addr = other.data_addr;
        write_addr = other.write_addr;
        data_in = other.data_in;
        read_en = other.read_en;
        write_en = other.write_en;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const dmem_in_t & other) {
        if (!(data_addr == other.data_addr))
            return false;
        if (!(write_addr == other.write_addr))
            return false;
        if (!(data_in == other.data_in))
            return false;
        if (!(read_en == other.read_en))
            return false;
        if (!(write_en == other.write_en))
            return false;
        return true;
    }

    //
    // Assignment operator from dmem_in_t.
    //
    inline dmem_in_t & operator = (const dmem_in_t & other) {
        data_addr = other.data_addr;
        write_addr = other.write_addr;
        data_in = other.data_in;
        read_en = other.read_en;
        write_en = other.write_en;
        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & data_addr;
            m & write_addr;
            m & data_in;
            m & read_en;
            m & write_en;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const dmem_in_t & object, const std::string & in_name) {
        sc_trace(tf, object.data_addr, in_name + std::string(".data_addr"));
        sc_trace(tf, object.data_addr, in_name + std::string(".write_addr"));
        sc_trace(tf, object.data_in, in_name + std::string(".data_in"));
        sc_trace(tf, object.read_en, in_name + std::string(".read_en"));
        sc_trace(tf, object.write_en, in_name + std::string(".write_en"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const dmem_in_t & object) {
        os << "(";
        os << object.data_addr;
        os << object.write_addr;
        os << object.data_in;
        os << object.read_en;
        os << object.write_en;
        os << ")";
        return os;
    }

};
#endif
// ------------ dmem_in_t

// ------------ dmem_out_t
#ifndef dmem_out_t_SC_WRAPPER_TYPE
#define dmem_out_t_SC_WRAPPER_TYPE 1

struct dmem_out_t {
    //
    // Member declarations.
    //
    ac_int < DCACHE_LINE, false > data_out;

    static const int width = DCACHE_LINE;
    //
    // Default constructor.
    //
    dmem_out_t() {
        data_out = 0;
    }

    //
    // Copy constructor.
    //
    dmem_out_t(const dmem_out_t & other) {
        data_out = other.data_out;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const dmem_out_t & other) {
        if (!(data_out == other.data_out))
            return false;
        return true;
    }

    //
    // Assignment operator from dmem_out_t.
    //
    inline dmem_out_t & operator = (const dmem_out_t & other) {
        data_out = other.data_out;
        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & data_out;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const dmem_out_t & object, const std::string & in_name) {
        sc_trace(tf, object.data_out, in_name + std::string(".data_out"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const dmem_out_t & object) {
        os << "(";
        os << object.data_out;
        os << ")";
        return os;
    }

};
#endif

// ------------ dmem_out_t
#ifndef fe_in_t_SC_WRAPPER_TYPE
#define fe_in_t_SC_WRAPPER_TYPE 1

struct fe_in_t {
    //
    // Member declarations.
    //
    bool freeze;
    bool redirect;
    ac_int < PC_LEN, false > address;
    bool btb_update;
    bool branch_taken;
    ac_int < PC_LEN, false > pc;
    ac_int < PC_LEN, false > bta;

    static const int width = 2 + PC_LEN + 2 + PC_LEN + PC_LEN;
    //
    // Default constructor.
    //
    fe_in_t() {
        freeze = false;
        redirect = false;
        address = 0;
        btb_update = false;
        branch_taken = false; 
        pc = 0;
        bta = 0;
    }

    //
    // Copy constructor.
    //
    fe_in_t(const fe_in_t &other) {
        freeze = other.freeze;
        redirect = other.redirect;
        address = other.address;
        btb_update = other.btb_update;
        branch_taken = other.branch_taken;
        pc = other.pc;
        bta = other.bta;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const fe_in_t &other) {
        if (!(freeze == other.freeze))
            return false;
        if (!(redirect == other.redirect))
            return false;
        if (!(address == other.address))
            return false;
        if (!(btb_update == other.btb_update))
            return false;
        if (!(branch_taken == other.branch_taken))
            return false;   
        if (!(pc == other.pc))
            return false;
        if (!(bta == other.bta))
            return false;
        return true;
    }

    //
    // Assignment operator from stall_t.
    //
    inline fe_in_t & operator = (const fe_in_t &other) {
        freeze = other.freeze;
        redirect = other.redirect;
        address = other.address;
        btb_update = other.btb_update;
        branch_taken = other.branch_taken;
        pc = other.pc;
        bta = other.bta;

        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & freeze;
            m & redirect;
            m & address;
            m & btb_update;
            m & branch_taken;
            m & pc;
            m & bta;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const fe_in_t & object, const std::string & in_name) {
        sc_trace(tf, object.freeze, in_name + std::string(".freeze"));
        sc_trace(tf, object.redirect, in_name + std::string(".redirect"));
        sc_trace(tf, object.address, in_name + std::string(".address"));
        sc_trace(tf, object.btb_update, in_name + std::string(".btb_update"));
        sc_trace(tf, object.branch_taken, in_name + std::string(".branch_taken"));
        sc_trace(tf, object.pc, in_name + std::string(".pc"));
        sc_trace(tf, object.bta, in_name + std::string(".bta"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const fe_in_t & object) {
        os << "(";
        os << object.freeze;
        os << object.redirect;
        os << object.address;
        os << object.btb_update;
        os << object.branch_taken;
        os << object.pc;
        os << object.bta;
        os << ")";
        return os;
    }

};
#endif

// ------------ icache_data_t
#ifndef icache_data_t_SC_WRAPPER_TYPE
#define icache_data_t_SC_WRAPPER_TYPE 1

struct icache_data_t {
    //
    // Member declarations.
    //
    ac_int < ICACHE_LINE, false > data;

    static const int width = ICACHE_LINE;
    //
    // Default constructor.
    //
    icache_data_t() {
        //data = 0;
    }

    //
    // Copy constructor.
    //
    icache_data_t(const icache_data_t &other) {
        data = other.data;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const icache_data_t &other) {
        if (!(data == other.data))
            return false;
        return true;
    }

    //
    // Assignment operator from stall_t.
    //
    inline icache_data_t & operator = (const icache_data_t &other) {
        data = other.data;

        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & data;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const icache_data_t & object, const std::string & in_name) {
        sc_trace(tf, object.data, in_name + std::string(".data"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const icache_data_t & object) {
        os << "(";
        os << object.data;
        os << ")";
        return os;
    }

};
#endif

#ifndef icache_tag_t_SC_WRAPPER_TYPE
#define icache_tag_t_SC_WRAPPER_TYPE 1

struct icache_tag_t {
    //
    // Member declarations.
    //
    ac_int < ICACHE_TAG_WIDTH, false > tag;
    bool valid;

    static const int width = ICACHE_TAG_WIDTH + 2;
    //
    // Default constructor.
    //
    icache_tag_t() {
        tag = 0;
        valid = false;
    }

    //
    // Copy constructor.
    //
    icache_tag_t(const icache_tag_t &other) {
        tag = other.tag;
        valid = other.valid;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const icache_tag_t &other) {
        if (!(tag == other.tag))
            return false;
        if (!(valid == other.valid))
            return false;
        return true;
    }

    //
    // Assignment operator from stall_t.
    //
    inline icache_tag_t & operator = (const icache_tag_t &other) {
        tag = other.tag;
        valid = other.valid;

        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & tag;
            m & valid;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const icache_tag_t & object, const std::string & in_name) {
        sc_trace(tf, object.tag, in_name + std::string(".tag"));
        sc_trace(tf, object.valid, in_name + std::string(".valid"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const icache_tag_t & object) {
        os << "(";
        os << object.tag;
        os << object.valid;
        os << ")";
        return os;
    }
};
#endif

// ------------ icache_out_t
#ifndef icache_out_t_SC_WRAPPER_TYPE
#define icache_out_t_SC_WRAPPER_TYPE 1

struct icache_out_t {
    //
    // Member declarations.
    //
    ac_int < ICACHE_LINE, false > data;
    bool hit;

    static const int width = ICACHE_LINE + 1;
    //
    // Default constructor.
    //
    icache_out_t() {
        data = 0;
        hit = false;
    }

    //
    // Copy constructor.
    //
    icache_out_t(const icache_out_t &other) {
        data = other.data;
        hit = other.hit;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const icache_out_t &other) {
        if (!(data == other.data))
            return false;
        if (!(hit == other.hit))
            return false;
        return true;
    }

    //
    // Assignment operator from stall_t.
    //
    inline icache_out_t & operator = (const icache_out_t &other) {
        data = other.data;
        hit = other.hit;

        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & data;
            m & hit;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const icache_out_t & object, const std::string & in_name) {
        sc_trace(tf, object.data, in_name + std::string(".data"));
        sc_trace(tf, object.hit, in_name + std::string(".hit"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const icache_out_t & object) {
        os << "(";
        os << object.data;
        os << object.hit;
        os << ")";
        return os;
    }

};
#endif

// ------------ dcache_data_t
#ifndef dcache_data_t_SC_WRAPPER_TYPE
#define dcache_data_t_SC_WRAPPER_TYPE 1

struct dcache_data_t {
    //
    // Member declarations.
    //
    ac_int < DCACHE_LINE, false > data;

    static const int width = DCACHE_LINE;
    //
    // Default constructor.
    //
    dcache_data_t() {
        //data = 0;
    }

    //
    // Copy constructor.
    //
    dcache_data_t(const dcache_data_t &other) {
        data = other.data;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const dcache_data_t &other) {
        if (!(data == other.data))
            return false;
        return true;
    }

    //
    // Assignment operator from stall_t.
    //
    inline dcache_data_t & operator = (const dcache_data_t &other) {
        data = other.data;

        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & data;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const dcache_data_t & object, const std::string & in_name) {
        sc_trace(tf, object.data, in_name + std::string(".data"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const dcache_data_t & object) {
        os << "(";
        os << object.data;
        os << ")";
        return os;
    }

};
#endif

#ifndef dcache_tag_t_SC_WRAPPER_TYPE
#define dcache_tag_t_SC_WRAPPER_TYPE 1

struct dcache_tag_t {
    //
    // Member declarations.
    //
    ac_int < DCACHE_TAG_WIDTH, false > tag;
    bool valid;
    bool dirty;

    static const int width = DCACHE_TAG_WIDTH + 2;
    //
    // Default constructor.
    //
    dcache_tag_t() {
        tag = 0;
        valid = false;
        dirty = false;
    }

    //
    // Copy constructor.
    //
    dcache_tag_t(const dcache_tag_t &other) {
        tag = other.tag;
        valid = other.valid;
        dirty = other.dirty;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const dcache_tag_t &other) {
        if (!(tag == other.tag))
            return false;
        if (!(valid == other.valid))
            return false;
        if (!(dirty == other.dirty))
            return false;
        return true;
    }

    //
    // Assignment operator from stall_t.
    //
    inline dcache_tag_t & operator = (const dcache_tag_t &other) {
        tag = other.tag;
        valid = other.valid;
        dirty = other.dirty;

        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & tag;
            m & valid;
            m & dirty;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const dcache_tag_t & object, const std::string & in_name) {
        sc_trace(tf, object.tag, in_name + std::string(".tag"));
        sc_trace(tf, object.valid, in_name + std::string(".valid"));
        sc_trace(tf, object.dirty, in_name + std::string(".dirty"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const dcache_tag_t & object) {
        os << "(";
        os << object.tag;
        os << object.valid;
        os << object.dirty;
        os << ")";
        return os;
    }
};
#endif

// ------------ dcache_data_t
#ifndef dcache_out_t_SC_WRAPPER_TYPE
#define dcache_out_t_SC_WRAPPER_TYPE 1

struct dcache_out_t {
    //
    // Member declarations.
    //
    ac_int < DCACHE_LINE, false > data;
    bool hit;

    static const int width = DCACHE_LINE + 1;
    //
    // Default constructor.
    //
    dcache_out_t() {
        data = 0;
        hit = false;
    }

    //
    // Copy constructor.
    //
    dcache_out_t(const dcache_out_t &other) {
        data = other.data;
        hit = other.hit;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const dcache_out_t &other) {
        if (!(data == other.data))
            return false;
        if (!(hit == other.hit))
            return false;
        return true;
    }

    //
    // Assignment operator from stall_t.
    //
    inline dcache_out_t & operator = (const dcache_out_t &other) {
        data = other.data;
        hit = other.hit;

        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & data;
            m & hit;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const dcache_out_t & object, const std::string & in_name) {
        sc_trace(tf, object.data, in_name + std::string(".data"));
        sc_trace(tf, object.hit, in_name + std::string(".hit"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const dcache_out_t & object) {
        os << "(";
        os << object.data;
        os << object.hit;
        os << ")";
        return os;
    }

};
#endif

// ------------ btb_data_t
#ifndef btb_data_t_SC_WRAPPER_TYPE
#define btb_data_t_SC_WRAPPER_TYPE 1

struct btb_data_t {
    //
    // Member declarations.
    //
    ac_int < BTB_TAG_WIDTH, false > tag;
    ac_int < PC_LEN, false > bta;
    ac_int < BTB_PREDICTION_BITS_WIDTH, false > prediction_data;

    static const int width = BTB_TAG_WIDTH + PC_LEN + BTB_PREDICTION_BITS_WIDTH;
    //
    // Default constructor.
    //
    btb_data_t() {
        tag = 0;
        bta = 0;
        prediction_data = 0;
    }

    //
    // Copy constructor.
    //
    btb_data_t(const btb_data_t &other) {
        tag = other.tag;
        bta = other.bta;
        prediction_data = other.prediction_data;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const btb_data_t &other) {
        if (!(tag == other.tag))
            return false;
        if (!(bta == other.bta))
            return false;
        if (!(prediction_data == other.prediction_data))
            return false;
        return true;
    }

    //
    // Assignment operator from btb_data_t.
    //
    inline btb_data_t & operator = (const btb_data_t &other) {
        tag = other.tag;
        bta = other.bta;
        prediction_data = other.prediction_data;

        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & tag;
            m & bta;
            m & prediction_data;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const btb_data_t & object, const std::string & in_name) {
        sc_trace(tf, object.tag, in_name + std::string(".tag"));
        sc_trace(tf, object.bta, in_name + std::string(".bta"));
        sc_trace(tf, object.prediction_data, in_name + std::string(".prediction_data"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const btb_data_t & object) {
        os << "(";
        os << object.tag;
        os << object.bta;
        os << object.prediction_data;
        os << ")";
        return os;
    }

};
#endif

// ------------ btb_data_t
#ifndef btb_out_t_SC_WRAPPER_TYPE
#define btb_out_t_SC_WRAPPER_TYPE 1

struct btb_out_t {
    //
    // Member declarations.
    //
    bool btb_valid;
    ac_int < PC_LEN, false > bta;

    static const int width = 1 + PC_LEN;
    //
    // Default constructor.
    //
    btb_out_t() {
        btb_valid = false;
        bta = 0;
    }

    //
    // Copy constructor.
    //
    btb_out_t(const btb_out_t &other) {
        btb_valid = other.btb_valid;
        bta = other.bta;
    }

    //
    // Comparison operator.
    //
    inline bool operator == (const btb_out_t &other) {
        if (!(btb_valid == other.btb_valid))
            return false;
        if (!(bta == other.bta))
            return false;
        return true;
    }

    //
    // Assignment operator from btb_data_t.
    //
    inline btb_out_t & operator = (const btb_out_t &other) {
        btb_valid = other.btb_valid;
        bta = other.bta;
        
        return *this;
    }

    template < unsigned int Size >
        void Marshall(Marshaller < Size > & m) {
            m & btb_valid;
            m & bta;
        }

    //
    // sc_trace function.
    //
    inline friend void sc_trace(sc_trace_file * tf, const btb_out_t & object, const std::string & in_name) {
        sc_trace(tf, object.btb_valid, in_name + std::string(".btb_valid"));
        sc_trace(tf, object.bta, in_name + std::string(".bta"));
    }

    //
    // stream operator.
    //
    inline friend ostream & operator << (ostream & os,
        const btb_out_t & object) {
        os << "(";
        os << object.btb_valid;
        os << object.bta;
        os << ")";
        return os;
    }

};
#endif

#endif // ------------ hl5_datatypes.h include guard
