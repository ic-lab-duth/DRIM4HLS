/* Copyright 2017 Columbia University, SLD Group */

//
// main.cpp - Robert Margelli
// Instantiates the top-level module (system) and
// launches simulation (sc_start).
//
#include <mc_scverify.h>
#include "system.hpp"

TOP *top = NULL;

int sc_main(int argc, char *argv[])
{	

	top = new TOP("top");

	sc_clock        clk("clk", 10, SC_NS);
	sc_signal<bool> rst("rst");

	top->clk(clk);
	top->rst(rst);

	rst.write(false);
	sc_start(51, SC_NS);
	rst.write(true);

	sc_start();

	return 0;
}
