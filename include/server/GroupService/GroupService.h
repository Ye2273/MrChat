#pragma once
#include <mysql/mysql.h>
#include <myrpc/MyRpcChannel.h>
#include <google/protobuf/service.h>
#include <string>
#include "UserService.h"
#include "User.h"
#include "GroupService.pb.h"
#include "OfflineMsg.h"
class GroupService : public Ye_GroupService::GroupServiceRpc
{
public:
    // 加入群组
    void AddGroup(::google::protobuf::RpcController *controller,
               const ::Ye_GroupService::AddGroupRequest *request,
               ::Ye_GroupService::AddGroupResponse *response,
               ::google::protobuf::Closure *done);
    // 创建群组
    void CreateGroup(::google::protobuf::RpcController *controller,
                const ::Ye_GroupService::CreateGroupRequest *request,
                ::Ye_GroupService::CreateGroupResponse *response,
                ::google::protobuf::Closure *done);
    // 获取用户加入的所有群组
    void GroupList(::google::protobuf::RpcController *controller,
                  const ::Ye_GroupService::GroupListRequest *request,
                  ::Ye_GroupService::GroupListResponse *response,
                  ::google::protobuf::Closure *done);
    // 获取群组所有用户id, 用于转发群组消息
    void GetGroupUsers(::google::protobuf::RpcController *controller,
                  const ::Ye_GroupService::GetGroupUsersRequest *request,
                  ::Ye_GroupService::GetGroupUsersResponse *response,
                  ::google::protobuf::Closure *done);

private:
    // 数据操作类对象
    UserService _userService;
    // 离线消息对象
    OfflineMsg _offlineMsg;
};