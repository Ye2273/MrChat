set(SRC_LIST FriendService.cpp  
                        ../db/MySQL.cpp ../proto/FriendService.pb.cc 
                        ../user/UserService.cpp main.cpp ../offlinemsg/OfflineMsg.cpp ../redis/Redis.cpp)
# 生成可执行文件
add_executable(friend ${SRC_LIST})
# 链接库文件
target_link_libraries(friend MyRpc protobuf muduo_net muduo_base mysqlclient  zookeeper_mt mysqlclient hiredis pthread)
