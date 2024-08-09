#pragma once
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "UserService.h"
using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)>;

class InterfaceService
{
public:
    // 获取单例对象
    static InterfaceService &GetInstance();
    InterfaceService &operator=(const InterfaceService &) = delete;
    InterfaceService(const InterfaceService &) = delete;
    // 获取消息处理器
    MsgHandler GetHandler(std::string msgType);

    // 处理登录业务
    void Login(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);
    // 处理注册业务
    void Register(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time);

    //处理客户端异常退出
    void ClientCloseException(const muduo::net::TcpConnectionPtr &conn);

private:
    InterfaceService();
    // 存储消息处理器
    std::unordered_map<std::string, MsgHandler> _msgHandlerMap;
    // 互斥锁,保证msgHandlerMap_线程安全
    std::mutex _connMutex;

    // 数据操作类对象
    UserService _userService;
};







