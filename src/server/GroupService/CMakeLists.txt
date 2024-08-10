set(SRC_LIST GroupService.cpp  
                        ../db/MySQL.cpp ../proto/GroupService.pb.cc 
                        ../user/UserService.cpp main.cpp )
# 生成可执行文件
add_executable(group ${SRC_LIST})
# 链接库文件
target_link_libraries(group MyRpc protobuf muduo_net muduo_base mysqlclient  zookeeper_mt mysqlclient hiredis pthread)
