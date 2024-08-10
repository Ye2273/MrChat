#pragma once
#include <mysql/mysql.h>
#include <myrpc/MyRpcChannel.h>
#include <google/protobuf/service.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>
#include "User.h"
#include "GroupService.pb.h"
#include "AccountService.pb.h"
#include "Interface.pb.h"
#include "FriendService.pb.h"
// 记录当前系统登录的用户信息
User _currentUser;
// 记录当前登录用户的好友列表信息
std::vector<User> _currentUserFriendList;
// 记录当前登录用户的群组列表信息
std::vector<Ye_GroupService::GroupInfo&> _currentUserGroupList;

// 控制主菜单页面程序
bool ChatPageFlag = false;

// 用于读写线程之间的通信
sem_t RWsem;
// 记录登录状态
std::atomic_bool isLogin(false);
// 记录注册状态
std::atomic_bool isRegister(false);

// 接收线程
void recvThread(int);
// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime();
// 主聊天页面程序
void chatPage(int);
// 显示当前登录成功用户的基本信息
void showUserInfo();


// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char* argv[]) {
    if(argc != 3) {
        std::cerr << "Usage: ./Client [ip] [port]" << std::endl;
        return -1;
    }

    // 解析命令行参数
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd < 0) {
        std::cerr << "socket error" << std::endl;
        exit(-1);
    }
    // 填写服务器地址信息
    struct sockaddr_in server;
    // 清空server
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);
    // 连接服务器
    if(connect(clientfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "connect error" << std::endl;
        exit(-1);
    }

    // 初始化读写信号量
    sem_init(&RWsem, 0, 0);

    // 创建接收数据的子线程，用于接收服务器发送的数据
    std::thread recv(recvThread, clientfd);
    recv.detach();

    // 主线程用于接收用户输入，负责发送数据
    while (1)
    {
        // 显示首页面菜单 登录、注册、退出
        std::cout << "========================" << std::endl;
        std::cout << "1. Login" << std::endl;
        std::cout << "2. Register" << std::endl;
        std::cout << "3. Quit" << std::endl;
        std::cout << "========================" << std::endl;
        std::cout << "Please Select: ";
        int select = 0;
        std::cin >> select;
        // 清空缓冲区
        std::cin.get();

        switch (select)
        {
            case 1:
            {
                // 登录
                int id = 0;
                char pwd[50] = {0};
                std::cout << "Please Input Your ID: ";
                std::cin >> id;
                std::cin.get();
                std::cout << "Please Input Your Password: ";
                std::cin.getline(pwd, 50);
                // 封装登录请求
                Ye_AccountService::LoginRequest request;
                request.set_id(id);
                request.set_password(pwd);
                request.set_msg("Login");
                std::string requestStr;
                request.SerializeToString(&requestStr);

                isLogin = false;
                // 发送登录请求
                int ret = send(clientfd, requestStr.c_str(), strlen(requestStr.c_str()) + 1, 0);
                if(ret < 0) {
                    std::cerr << "send login request error" << std::endl;
                    break;
                }
                // 等待接收线程处理登录结果
                sem_wait(&RWsem);
                if(isLogin) {
                    // 登录成功
                    ChatPageFlag = true;
                    // 进入聊天页面
                    chatPage(clientfd);
                } else {
                    std::cout << "Login Failed, Please Check Your ID and Password!" << std::endl;
                }
                break;
            }
            case 2:
            {
                // 注册
                char name[50] = {0};
                char pwd[50] = {0};
                std::cout << "Please Input Your Name: ";
                std::cin.getline(name, 50);
                std::cout << "Please Input Your Password: ";
                std::cin.getline(pwd, 50);
                // 封装注册请求
                Ye_AccountService::RegisterRequest request;
                request.set_name(name);
                request.set_password(pwd);
                request.set_msg("Register");
                std::string requestStr;
                request.SerializeToString(&requestStr);
                // 发送注册请求
                int ret = send(clientfd, requestStr.c_str(), strlen(requestStr.c_str()) + 1, 0);
                if(ret < 0) {
                    std::cerr << "send register request error" << std::endl;
                    break;
                }
                // 等待接收线程处理注册结果
                sem_wait(&RWsem);
                
                break;
            }
            case 3:
            {
                // 退出
                close(clientfd);
                sem_destroy(&RWsem);
                exit(0);
                break;
            }
            default:
            {
                std::cout << "Input Error, Please Input Again!" << std::endl;
                break;
            }
        }
    }
    return 0;
}

void recvThread(int clientfd) {
    while (true) {
        char buffer[1024] = {0};
        int ret = recv(clientfd, buffer, sizeof(buffer), 0);

        if (ret <= 0) {
            close(clientfd);
            std::cerr << "recv error" << std::endl;
            return; // 退出线程而不是调用 exit(-1)
        }

        // 注意：需要根据实际协议处理数据长度和拼接
        std::string data(buffer, ret); // 使用接收到的数据创建一个 string 对象

        // 定义不同的消息类型
        Ye_AccountService::LoginResponse login_response;
        Ye_AccountService::RegisterResponse register_response;
        Ye_AccountService::LogoutResponse logout_response;
        Ye_Interface::ChatMsg chat_msg;
        Ye_GroupService::GroupListResponse group_list_response;

        // 处理不同的消息类型
        if (login_response.ParseFromString(data)) {
            DoLoginResponse(login_response);
        } else if (register_response.ParseFromString(data)) {
            if (register_response.is_success()) {
                std::cout << "Register Success! Your ID is: " << register_response.id() << std::endl;
            } else {
                std::cout << "Register Failed: " << register_response.msg() << std::endl;
            }
            isRegister = true;
            sem_post(&RWsem);
        } else if (logout_response.ParseFromString(data)) {
            if (logout_response.is_success()) {
                std::cout << "Logout Success!" << std::endl;
            } else {
                std::cout << "Logout Failed: " << logout_response.msg() << std::endl;
            }
        } else if (group_list_response.ParseFromString(data)) {
            // 接收群组列表信息
            // _currentUserGroupList.clear();
            // for (int i = 0; i < group_list_response.group_list_size(); i++) {
            //     _currentUserGroupList.push_back(group_list_response.group_list(i));
            // }
        } else if (chat_msg.ParseFromString(data)) {
            std::cout << "========================" << std::endl;
            std::cout << "From: " << chat_msg.from_user_id() << std::endl;
            // std::cout << "Time: " << chat_msg.time() << std::endl;
            std::cout << "Content: " << chat_msg.msg() << std::endl;
            std::cout << "========================" << std::endl;
        } else {
            std::cerr << "Unknown message format" << std::endl;
        }
    }
}

// 登录成功后的处理
void DoLoginResponse(const Ye_AccountService::LoginResponse& login_response) {
    if(login_response.is_success() == true) {
        // 登录成功
        std::cout << "Login Success!" << std::endl;
        // 记录当前登录用户信息
        _currentUser.SetId(login_response.id());
        _currentUser.SetName(login_response.name());
        
        std::cout << "======================login user======================" << std::endl;
        std::cout << "current login user => id:" << login_response.id() << " name:" << login_response.name() << std::endl;
        isLogin = true;
        sem_post(&RWsem);
    } else {
        std::cout << "Login Failed: " << login_response.msg() << std::endl;
    }
}


// "help" command handler
void help(int fd = 0, std::string str = "");
// "chat" command handler
void chat(int, std::string);
// "addfriend" command handler
void addfriend(int, std::string);
// "creategroup" command handler
void creategroup(int, std::string);
// "addgroup" command handler
void addgroup(int, std::string);
// "groupchat" command handler
void groupchat(int, std::string);
// "loginout" command handler
void loginout(int, std::string);

// 系统支持的客户端命令列表
std::unordered_map<std::string, std::string> commandMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}};

// 注册系统支持的客户端命令处理
std::unordered_map<std::string, std::function<void(int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
};


void ChatPage(int clientfd) {
    help();
    char buffer[1024] = {0};
    while(ChatPageFlag) {
        std::cin.getline(buffer, 1024);
        std::string commandbuf(buffer);
        std::string command; // 存储命令
        // 解析命令
        int idx = commandbuf.find(":");
        if (idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if(it != commandHandlerMap.end()) {
            // 执行命令处理函数
            it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
        } else {
            std::cerr << "Invalid Command, Please Input Again!" << std::endl;
        }
    }
}


// "help" command handler
void help(int, std::string)
{
    std::cout << "show command list >>> " << std::endl;
    for (auto &p : commandMap)
    {
        std::cout << p.first << " : " << p.second << std::endl;
    }
    std::cout << std::endl;
}

// "addfriend" command handler
void addfriend(int clientfd, std::string str)
{
    int friendid = atoi(str.c_str());

    Ye_FriendService::AddFriendRequest request;
    request.set_myid(_currentUser.GetId());
    request.set_friendid(friendid);
    // request.set_msg("AddFriend");
    std::string requestStr;

    int len = send(clientfd, requestStr.c_str(), strlen(requestStr.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addfriend msg error -> " << requestStr << std::endl;
    }
}
// "chat" command handler
void chat(int clientfd, std::string str)
{
    int idx = str.find(":"); // friendid:message
    if (-1 == idx)
    {
        std::cerr << "chat command invalid!" << std::endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    Ye_Interface::ChatMsg chat_msg;
    chat_msg.set_from_user_id(_currentUser.GetId());
    chat_msg.set_to_user_id(friendid);
    chat_msg.set_msg(message);
    // chat_msg.set_time(getCurrentTime());
    std::string buffer;

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send chat msg error -> " << buffer << std::endl;
    }
}
// "creategroup" command handler  groupname:groupdesc
void creategroup(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "creategroup command invalid!" << std::endl;
        return;
    }

    std::string groupname = str.substr(0, idx);
    std::string groupdesc = str.substr(idx + 1, str.size() - idx);

    Ye_GroupService::CreateGroupRequest request;
    request.set_userid(_currentUser.GetId());
    request.set_group_name(groupname);
    request.set_group_desc(groupdesc);
    std::string buffer;
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send creategroup msg error -> " << buffer << std::endl;
    }
}
// "addgroup" command handler
void addgroup(int clientfd, std::string str)
{
    int groupid = atoi(str.c_str());

    Ye_GroupService::AddGroupRequest request;
    request.set_userid(_currentUser.GetId());
    request.set_group_id(groupid);
    std::string buffer;

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send addgroup msg error -> " << buffer << std::endl;
    }
}
// "groupchat" command handler   groupid:message
void groupchat(int clientfd, std::string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        std::cerr << "groupchat command invalid!" << std::endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    std::string message = str.substr(idx + 1, str.size() - idx);

    // json js;
    // js["msgid"] = GROUP_CHAT_MSG;
    // js["id"] = g_currentUser.getId();
    // js["name"] = g_currentUser.getName();
    // js["groupid"] = groupid;
    // js["msg"] = message;
    // js["time"] = getCurrentTime();
    // string buffer = js.dump();
    Ye_Interface::ChatMsg chat_msg;
    

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send groupchat msg error -> " << buffer << std::endl;
    }
}
// "loginout" command handler
void loginout(int clientfd, std::string)
{
    Ye_AccountService::LogoutRequest request;
    request.set_id(_currentUser.GetId());
    std::string buffer;

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        std::cerr << "send loginout msg error -> " << buffer << std::endl;
    }
    else
    {
        ChatPageFlag = false;
    }   
}

// 获取系统时间（聊天信息需要添加时间信息）
std::string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}