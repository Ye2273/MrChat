# 查找当前目录下的所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./interface INT_LIST)
aux_source_directory(./proto ACC_LIST)
aux_source_directory(./user USER_LIST)
aux_source_directory(./offlinemsg OFFLINE_LIST)
# 生成可执行文件
add_executable(MrChat ${SRC_LIST} ${DB_LIST} ${INT_LIST} ${ACC_LIST} ${USER_LIST} ${OFFLINE_LIST} )

# 链接库
target_link_libraries(MrChat MyRpc protobuf muduo_net muduo_base mysqlclient  zookeeper_mt hiredis pthread )
add_subdirectory(AccountService)
add_subdirectory(FriendService)
add_subdirectory(GroupService)