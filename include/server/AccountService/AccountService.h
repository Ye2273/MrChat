#pragma once
#include <mysql/mysql.h>
#include <myrpc/MyRpcChannel.h>
#include <google/protobuf/service.h>
#include <string>
#include "UserService.h"
#include "User.h"
#include "AccountService.pb.h"
#include "OfflineMsg.h"
class AccountService : public Ye_AccountService::AccountServiceRpc
{
public:
    void Login(::google::protobuf::RpcController *controller,
               const ::Ye_AccountService::LoginRequest *request,
               ::Ye_AccountService::LoginResponse *response,
               ::google::protobuf::Closure *done);
    void Register(::google::protobuf::RpcController *controller,
                const ::Ye_AccountService::RegisterRequest *request,
                ::Ye_AccountService::RegisterResponse *response,
                ::google::protobuf::Closure *done);
    // void Logout(::google::protobuf::RpcController *controller,
    //               const ::Ye_AccountService::LogoutRequest *request,
    //               ::Ye_AccountService::LogoutRespons *response,
    //               ::google::protobuf::Closure *done);

private:
    // 数据操作类对象
    UserService _userService;
    // 离线消息对象
    OfflineMsg _offlineMsg;
};