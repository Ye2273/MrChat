set(SRC_LIST AccountService.cpp  
                        ../db/MySQL.cpp ../proto/AccountService.pb.cc 
                        ../user/UserService.cpp main.cpp ../offlinemsg/OfflineMsg.cpp ../redis/Redis.cpp)
# 生成可执行文件
add_executable(account ${SRC_LIST})
# 链接库文件
target_link_libraries(account MyRpc protobuf muduo_net muduo_base mysqlclient  zookeeper_mt mysqlclient hiredis pthread)
