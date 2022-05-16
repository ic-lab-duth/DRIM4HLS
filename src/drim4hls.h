/*	
	@author VLSI Lab, EE dept., Democritus University of Thrace

	@brief 
	Header file for the drim4hls CPU container.
	This module instantiates the stages and interconnects them.

	@note Changes from HL5

		- Use of HLSLibs connections for communication with the rest of the processor.

		- Connection with memories outside of the processor.


*/

#ifndef __DRIM4HLS__H
#define __DRIM4HLS__H

#include "drim4hls_datatypes.h"
#include "defines.h"
#include "globals.h"
#include "fetch.h"
#include "decode.h"
#include "execute.h"
#include "writeback.h"

#include <mc_connections.h>

#pragma hls_design_top
SC_MODULE(drim4hls) {
    public:
    // Declaration of clock and reset signals
    sc_in < bool > clk;
    sc_in < bool > rst;

    //End of simulation signal.
    sc_out < bool > CCS_INIT_S1(program_end);

    // Instruction counters
    sc_out < long int > CCS_INIT_S1(icount);
    sc_out < long int > CCS_INIT_S1(j_icount);
    sc_out < long int > CCS_INIT_S1(b_icount);
    sc_out < long int > CCS_INIT_S1(m_icount);
    sc_out < long int > CCS_INIT_S1(o_icount);

    // Inter-stage Channels and ports.
    Connections::Combinational < fe_out_t > CCS_INIT_S1(fe2de_ch);
    Connections::Combinational < de_out_t > CCS_INIT_S1(de2exe_ch);
    Connections::Combinational < fe_in_t > CCS_INIT_S1(de2fe_ch);
    Connections::Combinational < mem_out_t > CCS_INIT_S1(wb2de_ch); // Writeback loop
    Connections::Combinational < exe_out_t > CCS_INIT_S1(exe2mem_ch);
    Connections::Combinational < imem_out_t > CCS_INIT_S1(fe2de_imem_ch);

    Connections::In < imem_out_t > CCS_INIT_S1(imem2de_data);
    Connections::Out < imem_in_t > CCS_INIT_S1(fe2imem_data);

    Connections::In < dmem_out_t > CCS_INIT_S1(dmem2wb_data);
    Connections::Out < dmem_in_t > CCS_INIT_S1(wb2dmem_data);

    // Forwarding
    Connections::Combinational < reg_forward_t > CCS_INIT_S1(fwd_exe_ch);

    // Instantiate the modules
    fetch CCS_INIT_S1(fe);
    decode CCS_INIT_S1(dec);
    execute CCS_INIT_S1(exe);
    writeback CCS_INIT_S1(wb);

    SC_CTOR(drim4hls): clk("clk"),
    rst("rst"),
    program_end("program_end"),
    fe2de_ch("fe2de_ch"),
    de2exe_ch("de2exe_ch"),
    de2fe_ch("de2fe_ch"),
    exe2mem_ch("exe2mem_ch"),
    wb2de_ch("wb2de_ch"),
    fwd_exe_ch("fwd_exe_ch"),
    imem2de_data("imem2de_data"),
    fe2imem_data("fe2imem_data"),
    dmem2wb_data("dmem2wb_data"),
    wb2dmem_data("wb2dmem_data"),
    fe("Fetch"),
    dec("Decode"),
    exe("Execute"),
    wb("Writeback") {
        // FETCH
        fe.clk(clk);
        fe.rst(rst);
        fe.dout(fe2de_ch);
        fe.fetch_din(de2fe_ch);
        fe.imem_de(fe2de_imem_ch);
        fe.imem_din(fe2imem_data);
        fe.imem_dout(imem2de_data);

        // DECODE
        dec.clk(clk);
        dec.rst(rst);
        dec.dout(de2exe_ch);
        dec.feed_from_wb(wb2de_ch);
        dec.fetch_din(fe2de_ch);
        dec.fetch_dout(de2fe_ch);
        dec.program_end(program_end);
        dec.fwd_exe(fwd_exe_ch);
        dec.icount(icount);
        dec.j_icount(j_icount);
        dec.b_icount(b_icount);
        dec.m_icount(m_icount);
        dec.o_icount(o_icount);
        dec.imem_out(fe2de_imem_ch);

        // EXE
        exe.clk(clk);
        exe.rst(rst);
        exe.din(de2exe_ch);
        exe.dout(exe2mem_ch);
        exe.fwd_exe(fwd_exe_ch);

        // MEM
        wb.clk(clk);
        wb.rst(rst);
        wb.din(exe2mem_ch);
        wb.dout(wb2de_ch);

        wb.dmem_in(wb2dmem_data);
        wb.dmem_out(dmem2wb_data);
    }

};

#endif // end __DRIM4HLS__H
