add_subdirectory(enclave)

add_executable(data_provider main.cpp hosting_data.cpp check_data.cpp)
target_link_libraries(data_provider stbox_common_u ypc_core ypc_datahub_module ${ypc_dep_libs})
install(TARGETS data_provider DESTINATION bin COMPONENT c_datahub OPTIONAL)
