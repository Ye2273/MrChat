#include <myrpc/MyRpcApplication.h>
#include <myrpc/MyRpcConf.h>
#include "Interface.h"
#include "InterfaceService.h"

// 接口服务器异常退出信号处理，业务重置
void ResetHandler(int)
{
    InterfaceService::GetInstance().Reset();
    exit(0);
}   


int main(int argc, char **argv)
{
    MyRpcApplication::Instance()->Init(argc, argv);
    // MyRpcConf configure = MyRpcApplication::Instance()->GetConf();
    // std::string ip = configure.Load("rpcserverip");
    // int port = atoi(configure.Load("rpcserverport").c_str());

    signal(SIGINT, ResetHandler);

    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr("127.0.0.1", 8000);
    Interface server(&loop, addr, "Interface");

    server.start();
    loop.loop();

    return 0;
}