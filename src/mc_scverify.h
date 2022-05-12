#ifndef MC_SCVERIFY_H_INC
#define MC_SCVERIFY_H_INC
//
// mc_scverify.h
//
// This header file provides macros useful for creating testbenches
// for standard C++ designs as well as SystemC testbenches.
// Support for either standard C++ or SystemC is controlled by
// the compiler directive '#ifdef CCS_SYSC'.
//


#if defined(CCS_SYSC) && !defined(CCS_SCVERIFY)
  //---------------------------------------------------------
  // SystemC testbench mode
  //
  // Verification with SystemC testbenches requires that the
  // testbench be self-checking and that it return a non-zero
  // exit code to signal failure.
  //
  // A sample SystemC testbench (and sc_main) look like this:
  //
  // TBD
  //
  // SC_MODULE(testbench)
  // {
  // public:
  //   // Clock
  //   sc_clock                clk;
  //
  //   // Interconnect
  //   sc_in_clk               clk;
  //   sc_signal<bool>         rst;
  //   sc_signal<int>          datain;
  //   sc_signal<int>          dataout;
  //
  //   // Design Under Test (DUT)
  //   CCS_DESIGN(adder) adder_INST;
  //
  //   SC_CTOR(testbench) 
  //     : CCS_CLK_CTOR(clk, "clk", 10, SC_NS, 0.5, 0, SC_NS, false) // effectively clk("clk", 10, SC_NS, 0.5, 0, SC_NS, false)
  //     , adder_INST("adder_INST")
  //     , rst("rst"), datain("datain"), dataout("dataout") {
  //
  //     SC_THREAD(reset_driver);
  //   }
  //
  //   // Active-high Asynchronous reset driver
  //   void reset_driver() {
  //     rst.write(1);
  //     wait(clk.period() * 1.5); // wait 1.5 clock cycles
  //     std::ostringstream msg;
  //     msg << "De-asserting reset signal '" << rst.name() << "' @ " << sc_time_stamp();
  //     SC_REPORT_INFO(this->name(),msg.str().c_str());
  //     rst.write(0);
  //   }
  //
  //   // ... BODY OF TESTBENCH GOES HERE. Create methods
  //   // to drive inputs and check outputs. Mismatches
  //   // should report the error using:
  //   // SC_REPORT_ERROR("testbench", "ERROR: some error message\n");
  //
  // };
  //
  // int sc_main(int argc, char *argv[])
  // {
  //   sc_report_handler::set_actions("/IEEE_Std_1666/deprecated", SC_DO_NOTHING);
  //   testbench testbench_INST("testbench_INST");
  //   sc_report_handler::set_actions(SC_ERROR, SC_DISPLAY);
  //   sc_start();
  //   if (sc_report_handler::get_count(SC_ERROR) > 0) {
  //      cout << "Simulation FAILED\n"
  //      return -1;
  //   } else {
  //      cout << "Simulation PASSED\n"
  //   }
  //   return 0;
  // }

  #undef CCS_DESIGN
  #undef CCS_CLK_CTOR

  // Macro expands out to sc_module created in sysc_sim.cpp that
  // encapsulates the RTL instance and corresponding transactors
  #define CCS_DESIGN(a) CCS_RTL::sysc_sim_wrapper
  #define CCS_CLK_CTOR(clkobj,name_,period_v_,period_tu_,duty_cycle_,start_time_v_,start_time_tu_,posedge_first_) clkobj(name_,scverify_lookup_clk(name_,period_v_,period_tu_),duty_cycle_,sc_time(start_time_v_,start_time_tu_),posedge_first_)

  #ifndef SC_USE_STD_STRING
  #define SC_USE_STD_STRING
  #endif

  #ifndef SC_INCLUDE_DYNAMIC_PROCESSES
  #define SC_INCLUDE_DYNAMIC_PROCESSES
  #endif

  #define CCS_PRIVATE public
  #define CCS_PROTECTED public

  // Include DUT and SystemC simulation wrapper
  #include "sysc_sim.h"

#else
#if !defined(CCS_SYSC) && defined(CCS_SCVERIFY)
  //---------------------------------------------------------
  // Standard C++ testbench mode
  //
  // For standard C++ design testbenches, this file provides macros to 
  // simplify the creation of C++ testbench files that are SCVerify-ready.
  //
  // A sample testbench file looks like this:
  //
  // #include <mc_scverify.h>
  // // Include C++ design function
  // #include "my_func.h"
  //
  // CCS_MAIN(int argc, char *argv[])
  // {
  //   for (int iteration=0; iteration<5; iteration++) {
  //      int arg1 = iteration;     // 1st arg to my_func
  //      int arg2 = iteration+1;   // 2nd arg to my_func
  //      int arg3;                 // result
  //      CCS_DESIGN(my_func)(arg1,arg2,&arg3);
  //      cout << arg3 << endl;
  //   }
  //   return 0;  // return success
  // }

  #undef CCS_RETURN
  #undef CCS_DESIGN
  #undef CCS_MAIN
  #undef CCS_CLK_CTOR

  #define CCS_PRIVATE public
  #define CCS_PROTECTED public
  #define CCS_RETURN(a) return(a)
  #define CCS_DESIGN(a) testbench::exec_##a
  #define CCS_MAIN(a,b) int testbench::main()
  #define CCS_CLK_CTOR(clkobj,name_,period_v_,period_tu_,duty_cycle_,start_time_v_,start_time_tu_,posedge_first_) clkobj(name_,scverify_lookup_clk(name_,period_v_,period_tu_),duty_cycle_,sc_time(start_time_v_,start_time_tu_),posedge_first_)

  #if defined(CCS_SCVERIFY) && !defined(CCS_SCVERIFY_USE_CCS_BLOCK)
    #include <mc_testbench.h>
  #endif

#else
  //---------------------------------------------------------
  // No special verification mode
  #define CCS_PRIVATE private
  #define CCS_PROTECTED protected
  #define CCS_DESIGN(a) a
  #define CCS_MAIN(a,b) int main(a,b)
  #define CCS_RETURN(a) return(a)
  #define CCS_CLK_CTOR(clkobj,name_,period_v_,period_tu_,duty_cycle_,start_time_v_,start_time_tu_,posedge_first_) clkobj(name_,sc_time(period_v_,period_tu_),duty_cycle_,sc_time(start_time_v_,start_time_tu_),posedge_first_)

#endif
#endif

#if (defined(CCS_SCVERIFY) && !defined(AC_REPLAY_STAND_ALONE)) || defined(CCS_SYSC)
#include <sstream>
#include <systemc.h>

inline sc_time scverify_lookup_clk(const char *clkname, double default_period, sc_time_unit default_unit) {
   std::ostringstream warnmsg;
   sc_time default_clkp(default_period,default_unit);
   std::string env_var_name;
   env_var_name = "CCS_SCVERIFY_CLK_";
   env_var_name += clkname;
   char *val_str = getenv(env_var_name.c_str());
   if (val_str != NULL) {

      std::string str(val_str);
      size_t pos = str.find(" ");
      double period = atof(str.substr(0,pos).c_str());
      if ( period != 0) {
         std::string unitstr(str.substr(++pos));
         sc_time_unit units = SC_NS;
         if (unitstr == "SC_NS")  units = SC_NS;
         if (unitstr == "SC_FS")  units = SC_FS;
         if (unitstr == "SC_PS")  units = SC_PS;
         if (unitstr == "SC_US")  units = SC_US;
         if (unitstr == "SC_MS")  units = SC_MS;
         if (unitstr == "SC_SEC") units = SC_SEC;

         sc_time clkp(period,units);
         warnmsg << "Clock '" << clkname << "' - Overriding default clock period '" << default_clkp << "' with value '" << clkp << "'";
         SC_REPORT_WARNING("CCS_CLK_CTOR", warnmsg.str().c_str());
         return clkp;
      } else {
         warnmsg << "Clock '" << clkname << "' - Could not parse clock period override value '" << val_str << "'";
         SC_REPORT_ERROR("CCS_CLK_CTOR", warnmsg.str().c_str());
         warnmsg.clear();
      }
   }
   warnmsg << "Clock '" << clkname << "' - Using default clock period '" << default_clkp << "'";
   SC_REPORT_INFO("CCS_CLK_CTOR", warnmsg.str().c_str());
   return default_clkp;
}
#endif
#endif // MC_SCVERIFY_H_INC

//---------------------------------------------------------
#ifdef __SYNTHESIS__
  #define CCS_BLOCK(fn) fn
#else
  #if !defined(CCS_SCVERIFY) || !defined(CCS_SCVERIFY_USE_CCS_BLOCK)
    #define CCS_BLOCK(fn) fn
  #else
    #undef CCS_DESIGN
    #define CCS_DESIGN(fn) fn

    #include "ccs_block_macros.h"
    #define CCS_BLOCK_TOKENPASTE(x,y) ccs_intercept_ ## x ##_## y
    #define CCS_BLOCK_TOKENPASTE2(x,y) CCS_BLOCK_TOKENPASTE(x,y)
    #define CCS_BLOCK(a) CCS_BLOCK_TOKENPASTE2(a,__LINE__)

    #ifdef __AC_ASSERT_H__
    // Make sure 'ac_assert' overrides assert.h redefines
    #include <ac_assert.h>
    #endif
  #endif
#endif
//---------------------------------------------------------
