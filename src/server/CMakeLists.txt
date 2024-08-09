# 查找当前目录下的所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./interface INT_LIST)
aux_source_directory(./proto ACC_LIST)
# 生成可执行文件
add_executable(MrChat ${SRC_LIST} ${DB_LIST} ${INT_LIST} ${ACC_LIST})

# 链接库
target_link_libraries(MrChat MyRpc protobuf muduo_net muduo_base mysqlclient  zookeeper_mt hiredis pthread )
add_subdirectory(account)