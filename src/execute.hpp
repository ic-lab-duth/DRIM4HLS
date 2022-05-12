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

#define BIT(_N) (1 << _N)

#include <systemc.h>
#include <mc_connections.h>

#include "defines.hpp"
#include "globals.hpp"
#include "drim4hls_datatypes.hpp"

// Signed division quotient and remainder struct.
struct div_res_t{
	sc_int<XLEN> quotient;
	sc_int<XLEN> remainder;
};

// Unsigned division quotient and remainder struct.
struct u_div_res_t{
	sc_uint<XLEN> quotient;
	sc_uint<XLEN> remainder;
};

SC_MODULE(execute)
{
	// FlexChannel initiators
	Connections::In< de_out_t > din;
	Connections::Out< exe_out_t > dout;

	// Forward
	Connections::Out< reg_forward_t > fwd_exe;
	// Clock and reset signals
	sc_in_clk clk;
	sc_in<bool> rst;

	// Thread prototype
	void execute_th(void);

	// Support functions
	sc_bv<XLEN> sign_extend_imm_s(sc_bv<12> imm);       // Sign extend the S-type immediate field.
	sc_bv<XLEN> zero_ext_zimm(sc_bv<ZIMM_SIZE> zimm);   // Zero extend the zimm field for CSRRxI instructions.
	sc_uint<CSR_IDX_LEN> get_csr_index(sc_bv<CSR_ADDR> csr_addr); // Get csr index given the 12-bit CSR address.
	void set_csr_value(sc_uint<CSR_IDX_LEN> csr_index, sc_bv<XLEN> rs1, sc_uint<LOG2_CSR_OP_NUM> operation, sc_bv<2> rw_permission);  // Perform requested CSR operation (write/set/clear).

	// Divider functions
	u_div_res_t udiv_func(sc_uint<XLEN> num, sc_uint<XLEN> den);
	div_res_t div_func(sc_int<XLEN> num, sc_int<XLEN> den);

	// Constructor
	SC_CTOR(execute)
		: din("din")
		, dout("dout")
		, fwd_exe("fwd_exe")
		, clk("clk")
		, rst("rst")
	{
		SC_CTHREAD(execute_th, clk.pos());
		async_reset_signal_is(rst, false);
	}

	// Member variables
	de_out_t data_in;
	de_out_t input;
	exe_out_t output;
	dmem_in_t dmem_din;

	sc_uint<XLEN> csr[CSR_NUM]; // Control and status registers.
	
	reg_forward_t forward;

	bool dmem_freeze;
	bool freeze;
	sc_bv< XLEN > forward_data;
	sc_uint< TAG_WIDTH > forward_tag;
};


#endif
