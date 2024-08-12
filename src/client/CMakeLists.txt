aux_source_directory(. CLIENT_SRC)
aux_source_directory(../server/proto PROTO_SRC)
aux_source_directory(../server/offlinemsg OFFLINEMSG_SRC)
aux_source_directory(../server/db DB_SRC)
add_executable(client ${CLIENT_SRC} ${PROTO_SRC} ${OFFLINEMSG_SRC} ${DB_SRC})

target_link_libraries(client MyRpc protobuf muduo_net muduo_base mysqlclient  zookeeper_mt hiredis pthread )