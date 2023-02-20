options set Input/CppStandard c++11

# Set your working directory pointing to project's folder. Example:
# set_working_dir /home/dpatsidis/Desktop/DRIM4HLS

# Add project files
solution file add ./src/fetch.h
solution file add ./src/decode.h
solution file add ./src/drim4hls.h
solution file add ./src/top.cpp
solution file add ./src/execute.h
solution file add ./src/writeback.h
solution file set /home/dpatsidis/Desktop/DRIM4HLS/src/top.cpp -exclude true
go compile

# Set Catapult library
solution library add nangate-45nm_beh -- -rtlsyntool OasysRTL -vendor Nangate -technology 045nm
solution library add ram_nangate-45nm-dualport_beh
solution library add ram_nangate-45nm-separate_beh
solution library add ram_nangate-45nm-singleport_beh
solution library add ram_nangate-45nm-register-file_beh
solution library add rom_nangate-45nm_beh
solution library add rom_nangate-45nm-sync_regin_beh
solution library add rom_nangate-45nm-sync_regout_beh
go libraries

# Set design clock
directive set -CLOCKS {clk {-CLOCK_PERIOD 2.5 -CLOCK_HIGH_TIME 1.25 -CLOCK_OFFSET 0.000000 -CLOCK_UNCERTAINTY 0.0}}
go assembly

# Set architectural configurations
# The settings below were used for a 2-way data and instruction cache with cache line of 64bit
# If the associativity of the cache changes, interleave must change accordingly
directive set /drim4hls/fetch/fetch_th/btb_data.tag:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/fetch/fetch_th/btb_data.bta:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/fetch/fetch_th/btb_data.prediction_data:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/fetch/fetch_th/FETCH_BODY:for -UNROLL yes
directive set /drim4hls/fetch/fetch_th/FETCH_BODY:for:for -UNROLL yes
directive set /drim4hls/fetch/fetch_th/fetch::icache:for -UNROLL yes
directive set /drim4hls/fetch/fetch_th/FETCH_BODY:for#1 -UNROLL yes
directive set /drim4hls/fetch/fetch_th/FETCH_BODY:if#1:for -UNROLL yes
directive set /drim4hls/fetch/fetch_th/FETCH_BODY:if#1:for:for -UNROLL yes
directive set /drim4hls/fetch/fetch_th/fetch::icache_write:for -UNROLL yes
directive set /drim4hls/fetch/fetch_th/icache_data.data:rsc -MAP_TO_MODULE ram_nangate-45nm-dualport_beh.RAM_dualRW
directive set /drim4hls/fetch/fetch_th/icache_data.data:rsc -GEN_EXTERNAL_ENABLE true
directive set /drim4hls/fetch/fetch_th/icache_data.data:rsc -INTERLEAVE 2
directive set /drim4hls/decode/sentinel.rom:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/decode/decode_th/regfile:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/decode/decode_th/sentinel:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/execute/csr.rom:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/execute/execute_th/csr:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/writeback/writeback_th/writeback::dcache:for -UNROLL yes
directive set /drim4hls/writeback/writeback_th/writeback::dcache_write:for -UNROLL yes
directive set /drim4hls/writeback/writeback_th/dcache_data.data:rsc -INTERLEAVE 2
directive set /drim4hls/writeback/writeback_th/dcache_tags.tag:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/writeback/writeback_th/dcache_tags.valid:rsc -MAP_TO_MODULE {[Register]}
directive set /drim4hls/writeback/writeback_th/dcache_tags.dirty:rsc -MAP_TO_MODULE {[Register]}
go architect

# Remove RD/WR dependencies between memory blocks 
ignore_memory_precedences -from /drim4hls/fetch/fetch_th/icache_data_t::operator=:read_mem(icache_data.data:rsc(0)(0).@) -to /drim4hls/fetch/fetch_th/fetch::icache_write:for:write_mem(icache_data.data:rsc(0)(0).@)
ignore_memory_precedences -from /drim4hls/fetch/fetch_th/fetch::icache_write:for:write_mem(icache_data.data:rsc(0)(0).@) -to /drim4hls/fetch/fetch_th/icache_data_t::operator=:read_mem(icache_data.data:rsc(0)(0).@)
ignore_memory_precedences -from /drim4hls/fetch/fetch_th/icache_data_t::operator=:read_mem(icache_data.data:rsc(0)(1).@)#1 -to /drim4hls/fetch/fetch_th/fetch::icache_write:for:write_mem(icache_data.data:rsc(0)(1).@)#1
ignore_memory_precedences -from /drim4hls/fetch/fetch_th/fetch::icache_write:for:write_mem(icache_data.data:rsc(0)(1).@)#1 -to /drim4hls/fetch/fetch_th/icache_data_t::operator=:read_mem(icache_data.data:rsc(0)(1).@)#1
ignore_memory_precedences -from /drim4hls/writeback/writeback_th/dcache_data_t::operator=:read_mem(dcache_data.data:rsc(0)(0).@) -to /drim4hls/writeback/writeback_th/dcache_data_t::operator=#3:write_mem(dcache_data.data:rsc(0)(0).@)
ignore_memory_precedences -from /drim4hls/writeback/writeback_th/dcache_data_t::operator=#3:write_mem(dcache_data.data:rsc(0)(0).@) -to /drim4hls/writeback/writeback_th/dcache_data_t::operator=:read_mem(dcache_data.data:rsc(0)(0).@)
ignore_memory_precedences -from /drim4hls/writeback/writeback_th/dcache_data_t::operator=:read_mem(dcache_data.data:rsc(0)(1).@)#1 -to /drim4hls/writeback/writeback_th/dcache_data_t::operator=#3:write_mem(dcache_data.data:rsc(0)(1).@)#1
ignore_memory_precedences -from /drim4hls/writeback/writeback_th/dcache_data_t::operator=#3:write_mem(dcache_data.data:rsc(0)(1).@)#1 -to /drim4hls/writeback/writeback_th/dcache_data_t::operator=:read_mem(dcache_data.data:rsc(0)(1).@)#1
go extract

