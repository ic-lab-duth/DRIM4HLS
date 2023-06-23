options set Input/CppStandard c++11
set_working_dir /home/dpatsidis/Desktop/clean_repo/core
solution file add ./src/fetch.h
solution file add ./src/drim4hls.h
solution file add ./src/top.cpp
solution file add ./src/writeback.h
solution file add ./src/execute.h
solution file add ./src/decode.h
solution file set /home/dpatsidis/Desktop/clean_repo/core/src/top.cpp -exclude true
go compile
solution library add nangate-45nm_beh -- -rtlsyntool OasysRTL -vendor Nangate -technology 045nm
solution library add ram_nangate-45nm-dualport_beh
solution library add ram_nangate-45nm-separate_beh
solution library add ram_nangate-45nm-singleport_beh
solution library add ram_nangate-45nm-register-file_beh
solution library add rom_nangate-45nm_beh
solution library add rom_nangate-45nm-sync_regin_beh
solution library add rom_nangate-45nm-sync_regout_beh
go libraries
directive set -CLOCKS {clk {-CLOCK_PERIOD 10 -CLOCK_HIGH_TIME 5 -CLOCK_OFFSET 0.000000 -CLOCK_UNCERTAINTY 0.0}}
go assembly
directive set /drim4hls/decode/sentinel.rom:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/decode/decode_th/regfile:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/decode/decode_th/sentinel:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/execute/csr.rom:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/execute/execute_th/csr:rsc -MAP_TO_MODULE {[Register]}
go architect
go allocate
go extract