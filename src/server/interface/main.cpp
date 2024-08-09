#include <myrpc/MyRpcApplication.h>
#include <myrpc/MyRpcConf.h>
#include "Interface.h"
#include "InterfaceService.h"


int main(int argc, char **argv)
{
    MyRpcApplication::Instance()->Init(argc, argv);
    // MyRpcConf configure = MyRpcApplication::Instance()->GetConf();
    // std::string ip = configure.Load("rpcserverip");
    // int port = atoi(configure.Load("rpcserverport").c_str());

    // signal(SIGINT, reset_handler);

    muduo::net::EventLoop loop;
    muduo::net::InetAddress addr("127.0.0.1", 8000);
    Interface server(&loop, addr, "Interface");

    server.start();
    loop.loop();

    return 0;
}