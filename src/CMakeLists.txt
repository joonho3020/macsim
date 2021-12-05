set(MACSIM_SRCS
  all_knobs.cc
  all_stats.cc
  allocate.cc
  allocate_smc.cc
  bp.cc
  bp_gshare.cc
  bp_targ.cc
  bug_detector.cc
  cache.cc
  core.cc
  dram.cc
  dram_ctrl.cc
  dram_ramulator.cc
  exec.cc
  factory_class.cc
  fetch_factory.cc
  frontend.cc
  knob.cc
  macsim.cc
  main.cc
  map.cc
  memory.cc
  memreq_info.cc
  port.cc
  pref.cc
  pref_common.cc
  pref_factory.cc
  pref_stride.cc
  process_manager.cc
  readonly_cache.cc
  retire.cc
  rob.cc
  rob_smc.cc
  schedule.cc
  schedule_io.cc
  schedule_ooo.cc
  schedule_smc.cc
  schedule_igpu.cc
  statistics.cc
  sw_managed_cache.cc
  trace_read.cc
  uop.cc
  utils.cc
  network.cc
  network_ring.cc
  network_mesh.cc
  network_simple.cc
  trace_read_cpu.cc
  trace_read_gpu.cc
  trace_read_a64.cc
  trace_read_igpu.cc
  page_mapping.cc
  dyfr.cc
  hmc_process.cc
  trace_gen_a64.cc
  trace_gen_x86.cc
  cs_disas.cc
  resource.cc
  mmu.cc
  tlb.cc
  ramulator_wrapper.cc
  mxp_wrapper.cc
)


set(CXLSIM_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CXLSim/build)
set(CXLSIM_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/CXLSim)
add_custom_target(
  cxlsim_lib
  COMMAND make install
  WORKING_DIRECTORY ${CXLSIM_BUILD_DIR}
  )
link_directories(${CXLSIM_SRC_DIR})


set(RAMULATOR_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ramulator/build)
set(RAMULATOR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ramulator)
add_custom_target(
  ramulator
  COMMAND make install
  WORKING_DIRECTORY ${RAMULATOR_BUILD_DIR}
  )
link_directories(${RAMULATOR_SRC_DIR})


add_executable(${PROJECT_NAME} ${MACSIM_SRCS})
target_compile_definitions(${PROJECT_NAME} PUBLIC CXL RAMULATOR)
target_link_libraries(${PROJECT_NAME} ${CXLSIM_SRC_DIR}/libcxlsim.a ${RAMULATOR_SRC_DIR}/libramulator.a libz.a)
target_include_directories(${PROJECT_NAME} PUBLIC ${CXLSIM_SRC_DIR} ${RAMULATOR_SRC_DIR})
add_dependencies(${PROJECT_NAME} cxlsim_lib ramulator)


set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../bin)
install(TARGETS ${PROJECT_NAME} DESTINATION ${BIN_DIR})
# install(DIRECTORY ${DRAMSIM3_SRC_DIR}/configs DESTINATION ${BIN_DIR})
