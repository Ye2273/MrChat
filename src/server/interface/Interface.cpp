#include "Interface.h"
#include "Interface.pb.h"
#include "InterfaceService.h"
// 初始化服务器群接口类对象
Interface::Interface(muduo::net::EventLoop* loop, 
                        const muduo::net::InetAddress& addr, 
                        const std::string& name)
                        : server_(loop, addr, name), loop_(loop)
{
    // 设置连接回调函数
    server_.setConnectionCallback(std::bind(&Interface::onConnection, this, std::placeholders::_1));
    // 设置消息回调函数
    server_.setMessageCallback(std::bind(&Interface::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置线程数量
    server_.setThreadNum(4);
}


void Interface::start()
{
    // 启动服务器
    server_.start();
}

void Interface::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
    // 客户端断开链接
    if (!conn->connected())
    {
        InterfaceService::GetInstance().ClientCloseException(conn);
        conn->shutdown();
    }
}

void Interface::onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf, muduo::Timestamp time)
{
    std::string msg = buf->retrieveAllAsString();
    Ye_Interface::InterfaceRequest re;
    re.set_request_msg("hello");
    re.set_type("Chat");
    std::string request_str = re.SerializeAsString();
    Ye_Interface::InterfaceRequest request;
    request.ParseFromString(request_str);

    auto msg_handler = InterfaceService::GetInstance().GetHandler(request.type());

    std::string recv_str = request.request_msg();
    // std::cout << "recv_str: " << recv_str << std::endl;
    msg_handler(conn, recv_str, time);
    
}
