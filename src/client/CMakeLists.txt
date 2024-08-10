aux_source_directory(. CLIENT_SRC)

add_executable(client ${CLIENT_SRC})

target_link_libraries(client MyRpc protobuf pthread)