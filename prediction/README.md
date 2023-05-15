# DRIM4HLS: A 32-bit RISC-V processor designed for High Level Synthesis

DRIM4HLS is the first model of a 32b RISC-V processor designed with SystemC and [Matchlib's Connections](https://github.com/hlslibs/matchlib_connections "Connections"). The SystemC model is synthesized with High-level synthesis achieving an full-throughput pipeline dataflow. The baseline functionality of the processor is based on the [HL5 core](https://github.com/sld-columbia/hl5 "HL5") from Columbia University. DRIM4HLS was synthesized using Mentor's Catapult 2021 and verified with QuestaSim 2019.3_1. The RTL produced was also validated in an FPGA prototype using Xilinx Nexy 7 board.

The organization of the SystemC model of DRIM4HLS is shown in the following figure

![overview](./images/drim4hls_prediction.png)

The blue blocks are part of the processor while the instruction and data memories are used only for simulation. No specific latency is assumed by the memories. All blocks of the processor communicate using the Connections flow-controlled channels (ready/valid). In this way the stalling of instruction execution that naturally appears in all pipelined processors can be smoothly handled by the flow control mechanism inherent in the operation of Connections.

DRIM4HLS can be simulated using open-source libraries without requiring any other tools.

In the future the baseline pipelined processor will be enhanced with multiple architectural features such as branch prediction and caches that will improve its performance.