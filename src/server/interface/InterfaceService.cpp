#include "InterfaceService.h"

InterfaceService& InterfaceService::GetInstance()
{
    static InterfaceService instance;
    return instance;
}

InterfaceService::InterfaceService()
{
    _msgHandlerMap["register"] = std::bind(&InterfaceService::Register, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["login"] = std::bind(&InterfaceService::Login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["chat"] = std::bind(&InterfaceService::Chat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["addfriend"] = std::bind(&InterfaceService::AddFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["getfriendlist"] = std::bind(&InterfaceService::GetFriendList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["creategroup"] = std::bind(&InterfaceService::CreateGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["addgroup"] = std::bind(&InterfaceService::AddGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["grouplist"] = std::bind(&InterfaceService::GroupList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["groupchat"] = std::bind(&InterfaceService::GetGroupUsers, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
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

// 处理客户端异常退出
void InterfaceService::ClientCloseException(const muduo::net::TcpConnectionPtr &conn)
{
    
    User user;
    // 1.从在线用户列表中删除
    {
        // 2.保证线程安全
        std::lock_guard<std::mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.SetId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 3.更新用户状态为离线
    if (user.GetId() != -1)
    {
        user.SetState("offline");
        _userService.UpdateState(user);
    }
}

// 服务器异常退出，重置服务器
void InterfaceService::Reset()
{
    // 1.重置用户状态,全部设置为离线
    _userService.ResetState();
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
    string send_str = register_response.SerializeAsString();
    if (register_response.is_success())
    {
        LOG_INFO << "Register success";
        LOG_INFO << "id: " << register_response.id();
        //序列化并发送响应给客户端
        conn->send(send_str);
    }
    else
    {
        LOG_ERROR << "Register failed: " << register_response.msg();
        //序列化并发送响应给客户端
        conn->send(send_str);
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
        // 登录成功
        LOG_INFO << "Login success";
        // 保存用户连接, 保证线程安全
        {
            std::lock_guard<std::mutex> lock(_connMutex);
            _userConnMap[login_response.id()] = conn;
        }
        
        // //序列化并发送响应给客户端
        // string send_str = login_response.SerializeAsString();
        // conn->send(send_str);
        
    }
    else
    {
        LOG_ERROR << "Login failed: " << login_response.msg();
    }
}


void InterfaceService::Chat(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)
{
    int to_id = 2;
    // 1.判断对方是否在线
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        // 2.先在在线用户列表中查找（同一个服务器上的用户）
        auto it = _userConnMap.find(to_id);
        if (it != _userConnMap.end())
        {
            // 2.1对方在线，直接转发消息
            it->second->send(recv_str);
            return;
        }
    }
    // 3.对方不在在线用户表中，再去数据库中查找（可能是在另一个服务器上的用户）

    // 4.如果对方不在线，存储离线消息
    _offlineMsg.OfflineMsgInsert(to_id, recv_str);
}


// 处理添加好友业务
void InterfaceService::AddFriend(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 构造rpc请求
    Ye_FriendService::AddFriendRequest add_request;
    add_request.set_myid(2);
    add_request.set_friendid(1);

    // 2. 调用rpc服务
    Ye_FriendService::AddFriendResponse add_response;
    Ye_FriendService::FriendServiceRpc_Stub friend_stub(new MyRpcChannel());
    MyRpcController controller;
    friend_stub.AddFriend(&controller, &add_request, &add_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "add friend rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 处理返回结果
    if (add_response.is_success())
    {
        LOG_INFO << "Add friend success";
    }
    else
    {
        LOG_ERROR << "Add friend failed: " << add_response.msg();
    }
}

// 处理获取好友列表业务
void InterfaceService::GetFriendList(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 构造rpc请求
    Ye_FriendService::FriendListRequest friend_request;
    friend_request.set_id(2);

    // 2. 调用rpc服务
    Ye_FriendService::FriendListResponse friend_response;
    Ye_FriendService::FriendServiceRpc_Stub friend_stub(new MyRpcChannel());
    MyRpcController controller;
    friend_stub.GetFriendList(&controller, &friend_request, &friend_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "get friend list rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 处理返回结果
    if (friend_response.friends_size() > 0)
    {
        LOG_INFO << "Get friend list success";
        for (int i = 0; i < friend_response.friends_size(); ++i)
        {
            const Ye_FriendService::FriendInfo &friend_info = friend_response.friends(i);
            LOG_INFO << "id: " << friend_info.id() << " name: " << friend_info.name() << " state: " << friend_info.state();
        }
    }
    else
    {
        LOG_ERROR << "Get friend list failed";
    }
}

// 处理创建群组业务
void InterfaceService::CreateGroup(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 构造rpc请求
    Ye_GroupService::CreateGroupRequest create_request;
    create_request.set_group_name("group1");
    create_request.set_group_desc("group1 test");
    create_request.set_userid(2);
    // 2. 调用rpc服务
    Ye_GroupService::CreateGroupResponse create_response;
    Ye_GroupService::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.CreateGroup(&controller, &create_request, &create_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "create group rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 处理返回结果
    if (create_response.success())
    {
        LOG_INFO << "Create group success";
        LOG_INFO << "group id: " << create_response.group_id();
    }
    else
    {
        LOG_ERROR << "Create group failed: " << create_response.msg();
    }

}
// 处理加入群组业务
void InterfaceService::AddGroup(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 构造rpc请求
    Ye_GroupService::AddGroupRequest add_request;
    add_request.set_userid(2);
    add_request.set_group_id(1);
    add_request.set_role("member");

    // 2. 调用rpc服务
    Ye_GroupService::AddGroupResponse add_response;
    Ye_GroupService::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.AddGroup(&controller, &add_request, &add_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "add group rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 处理返回结果
    if (add_response.success())
    {
        LOG_INFO << "Add group success";
    }
    else
    {
        LOG_ERROR << "Add group failed: " << add_response.msg();
    }

    // 4.序列化并发送响应给客户端
    // string send_str = add_response.SerializeAsString();
    // conn->send(send_str);
}

// 处理获取群组列表业务
void InterfaceService::GroupList(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 构造rpc请求
    Ye_GroupService::GroupListRequest group_request;
    group_request.set_userid(2);

    // 2. 调用rpc服务
    Ye_GroupService::GroupListResponse group_response;
    Ye_GroupService::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.GroupList(&controller, &group_request, &group_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "group list rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 处理返回结果
    if (group_response.groups_size() > 0)
    {
        LOG_INFO << "Get group list success";
        for (int i = 0; i < group_response.groups_size(); ++i)
        {
            const Ye_GroupService::GroupInfo &group_info = group_response.groups(i);
            LOG_INFO << "group id: " << group_info.group_id() << " group name: " << group_info.group_name() << " group desc: " << group_info.group_desc();
            for (int j = 0; j < group_info.users_size(); ++j)
            {
                const Ye_GroupService::UserInfo &user_info = group_info.users(j);
                LOG_INFO << "user id: " << user_info.id() << " user name: " << user_info.name() << " user state: " << user_info.state() << " user role: " << user_info.role();
            }
        }
    }
    else
    {
        LOG_ERROR << "Get group list failed";
    }
}
// 处理获取群组用户id列表业务,同时转发消息
void InterfaceService::GetGroupUsers(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 构造rpc请求(要反序列化recv_buf)
    Ye_GroupService::GetGroupUsersRequest group_request;
    group_request.set_group_id(1);
    group_request.set_userid(2);

    // 2. 调用rpc服务
    Ye_GroupService::GetGroupUsersResponse group_response;
    Ye_GroupService::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.GetGroupUsers(&controller, &group_request, &group_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "group users rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 处理返回结果
    if (group_response.success())
    {
        LOG_INFO << "Get group users success";
        lock_guard<mutex> lock(_connMutex);
        // 4.序列化并发送响应给客户端
        // string send_str = add_response.SerializeAsString();
        // conn->send(send_str);
        for (int i = 0; i < group_response.users_size(); ++i)
        {
            const Ye_GroupService::UserId &user_info = group_response.users(i);
            LOG_INFO << "user id: " << user_info.id();
            auto it = _userConnMap.find(user_info.id());
            if (it != _userConnMap.end())
            {
                // 3.1对方在线，直接转发消息
                it->second->send(recv_buf);
            }
            else
            {
                // 3.2对方不在线，存储离线消息
                _offlineMsg.OfflineMsgInsert(user_info.id(), recv_buf);
            }
        }
    }
    else
    {
        LOG_ERROR << "Get group users failed";
    }
}








