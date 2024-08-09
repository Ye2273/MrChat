#include "InterfaceService.h"
#include "muduo/base/Logging.h"
#include "AccountService.pb.h"
#include "myrpc/MyRpcChannel.h"
#include "myrpc/MyRpcController.h"
InterfaceService& InterfaceService::GetInstance()
{
    static InterfaceService instance;
    return instance;
}

InterfaceService::InterfaceService()
{
    _msgHandlerMap["Register"] = std::bind(&InterfaceService::Register, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["Login"] = std::bind(&InterfaceService::Login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

//获得消息对应的处理器
MsgHandler InterfaceService::GetHandler(std::string msgType)
{
    auto it = _msgHandlerMap.find(msgType);
    if (it != _msgHandlerMap.end())
    {
        return it->second;
    }
    else 
    {
        // 如果没有对应的处理器，返回一个默认的处理器
        return [=](const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time) 
        {
            LOG_ERROR << "No handler for msg type: " << msgType;
        };
    }
}


// 处理注册业务.格式：用户名，密码，包含在recv_str中，要进行反序列化传给负责注册业务的服务器
void InterfaceService::Register(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)
{
    // 1. 构造rpc请求
    Ye_AccountService::RegisterRequest register_request;
    register_request.set_name("ye shen");
    register_request.set_password("123456");

    // 2. 调用rpc服务
    Ye_AccountService::RegisterResponse register_response;
    Ye_AccountService::AccountServiceRpc_Stub account_stub(new MyRpcChannel());
    MyRpcController controller;
    account_stub.Register(&controller, &register_request, &register_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "register rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 处理返回结果
    if (register_response.is_success())
    {
        LOG_INFO << "Register success";
        LOG_INFO << "id: " << register_response.id();
    }
    else
    {
        LOG_ERROR << "Register failed: " << register_response.msg();
    }
}

// 处理登录业务
void InterfaceService::Login(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)
{
     // 1. 构造rpc请求
    Ye_AccountService::LoginRequest register_request;
    register_request.set_id(2);
    register_request.set_password("123456");
    
    // 2. 调用rpc服务
    Ye_AccountService::LoginResponse login_response;
    Ye_AccountService::AccountServiceRpc_Stub account_stub(new MyRpcChannel());
    MyRpcController controller;
    account_stub.Login(&controller, &register_request, &login_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "login rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 处理返回结果
    if (login_response.is_success())
    {
        LOG_INFO << "Login success";
    }
    else
    {
        LOG_ERROR << "Login failed: " << login_response.msg();
    }
}


