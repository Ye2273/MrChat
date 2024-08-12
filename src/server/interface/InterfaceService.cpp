#include "InterfaceService.h"
#include "Interface.pb.h"
InterfaceService& InterfaceService::GetInstance()
{
    static InterfaceService instance;
    return instance;
}

InterfaceService::InterfaceService()
{
    _msgHandlerMap["Register"] = std::bind(&InterfaceService::Register, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["Login"] = std::bind(&InterfaceService::Login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["Logout"] = std::bind(&InterfaceService::Logout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["Chat"] = std::bind(&InterfaceService::Chat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["AddFriend"] = std::bind(&InterfaceService::AddFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["GetFriendList"] = std::bind(&InterfaceService::GetFriendList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["CreateGroup"] = std::bind(&InterfaceService::CreateGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["AddGroup"] = std::bind(&InterfaceService::AddGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["GroupList"] = std::bind(&InterfaceService::GroupList, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    _msgHandlerMap["GroupChat"] = std::bind(&InterfaceService::GetGroupUsers, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
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
    // 1. 反序列化recv_str
    Ye_Interface::UnifiedMessage register_request;
    register_request.ParseFromString(recv_str);
    // 1.1 构造rpc请求
    Ye_AccountService::RegisterRequest request;
    request.set_name(register_request.name());
    request.set_password(register_request.password());

    // 2. 调用rpc服务
    Ye_AccountService::RegisterResponse register_response;
    Ye_AccountService::AccountServiceRpc_Stub account_stub(new MyRpcChannel());
    MyRpcController controller;
    account_stub.Register(&controller, &request, &register_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "register rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 处理返回结果
    Ye_Interface::UnifiedMessage response;
    response.set_id(register_response.id());
    response.set_type("Register");
    response.set_is_success(register_response.is_success());
    response.set_msg(register_response.msg());
    // 序列化
    string send_str = response.SerializeAsString();
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
    Ye_Interface::UnifiedMessage register_request;
    register_request.ParseFromString(recv_str);
    // 1.1 构造rpc请求
    Ye_AccountService::LoginRequest request;
    request.set_id(register_request.id());
    request.set_password(register_request.password());
    request.set_msg("login");
    
    // 2. 调用rpc服务
    Ye_AccountService::LoginResponse login_response;
    Ye_AccountService::AccountServiceRpc_Stub account_stub(new MyRpcChannel());
    MyRpcController controller;
    account_stub.Login(&controller, &request, &login_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "login rpc call failed: " << controller.ErrorText();
        return;
    }
    // 构建统一的消息响应
    Ye_Interface::UnifiedMessage response;
    response.set_id(login_response.id());
    response.set_name(login_response.name());
    response.set_type("Login");
    response.set_is_success(login_response.is_success());
    // 数据的转换非常麻烦，改成直接在客户端实现。
    // for(int i = 0; i < login_response.offline_msg_size(); ++i)
    // {
    //     response.add_offline_msg(login_response.offline_msg(i));
    // }
    response.set_msg(login_response.msg());
    // 序列化
    string send_str = response.SerializeAsString();
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
        conn->send(send_str);
    }
    else
    {
        LOG_ERROR << "Login failed: " << login_response.msg();
        conn->send(send_str);

    }
}

// 注销业务
void InterfaceService::Logout(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)
{
    // 1.反序列化recv_str
    Ye_Interface::UnifiedMessage logout_msg;
    logout_msg.ParseFromString(recv_str);
    int id = logout_msg.id();
    // 1.判断用户是否在线
    {
        std::lock_guard<std::mutex> lock(_connMutex);
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 1.1用户在线，删除用户连接
            _userConnMap.erase(it);
        }
    }
    // 2.更新用户状态为离线
    User user;
    user.SetId(id);
    user.SetState("offline");
    _userService.UpdateState(user);
}


void InterfaceService::Chat(const muduo::net::TcpConnectionPtr &conn, std::string &recv_str, muduo::Timestamp time)
{
    // 1.反序列化recv_str
    Ye_Interface::UnifiedMessage chat_msg;
    chat_msg.ParseFromString(recv_str);
    int to_id = chat_msg.to_user_id();
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
    // 1. 反序列化recv_buf
    Ye_Interface::UnifiedMessage add_request;
    add_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    Ye_FriendService::AddFriendRequest request;
    request.set_myid(add_request.id());
    request.set_friendid(add_request.friendid());

    // 2. 调用rpc服务
    Ye_FriendService::AddFriendResponse add_response;
    Ye_FriendService::FriendServiceRpc_Stub friend_stub(new MyRpcChannel());
    MyRpcController controller;
    friend_stub.AddFriend(&controller, &request, &add_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "add friend rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 序列化并发送响应给客户端
    Ye_Interface::UnifiedMessage response;
    response.set_friendid(add_response.friendid());
    response.set_type("AddFriend");
    response.set_is_success(add_response.is_success());
    response.set_msg(add_response.msg());
    string send_str = response.SerializeAsString();

    if (add_response.is_success())
    {
        LOG_INFO << "Add friend success";
        conn->send(send_str);
    }
    else
    {
        LOG_ERROR << "Add friend failed: " << add_response.msg();
        conn->send(send_str);
    }
}

// 处理获取好友列表业务
void InterfaceService::GetFriendList(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf
    Ye_Interface::UnifiedMessage friend_request;
    friend_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    Ye_FriendService::FriendListRequest request;
    request.set_id(friend_request.id());

    // 2. 调用rpc服务
    Ye_FriendService::FriendListResponse friend_response;
    Ye_FriendService::FriendServiceRpc_Stub friend_stub(new MyRpcChannel());
    MyRpcController controller;
    friend_stub.GetFriendList(&controller, &request, &friend_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "get friend list rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 序列化并发送响应给客户端
    Ye_Interface::UnifiedMessage response;
    response.set_is_success(friend_response.is_success());
    response.set_type("GetFriendList");
    response.set_msg(friend_response.msg());
    if (friend_response.friends_size() > 0)
    {
        for (int i = 0; i < friend_response.friends_size(); ++i)
        {
            Ye_Interface::UnifiedMessage::FriendInfo *friend_info = response.add_friends_info();
            friend_info->set_id(friend_response.friends(i).id());
            friend_info->set_name(friend_response.friends(i).name());
            friend_info->set_state(friend_response.friends(i).state());
        }
        string send_str = response.SerializeAsString();
        LOG_INFO << "Get friend list success";
        conn->send(send_str);

    }
    else
    {
        string send_str = response.SerializeAsString();
        LOG_ERROR << "Get friend list failed";
        conn->send(send_str);
    }
}

// 处理创建群组业务
void InterfaceService::CreateGroup(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf
    Ye_Interface::UnifiedMessage create_request;
    create_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    Ye_GroupService::CreateGroupRequest request;
    request.set_group_name(create_request.group_name());
    request.set_group_desc(create_request.group_desc());
    request.set_userid(create_request.id());

    // 2. 调用rpc服务
    Ye_GroupService::CreateGroupResponse create_response;
    Ye_GroupService::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.CreateGroup(&controller, &request, &create_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "create group rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 序列化并发送响应给客户端
    Ye_Interface::UnifiedMessage response;
    response.set_group_id(create_response.group_id());
    response.set_type("CreateGroup");
    response.set_is_success(create_response.success());
    response.set_msg(create_response.msg());
    string send_str = response.SerializeAsString();
    if (create_response.success())
    {
        LOG_INFO << "Create group success";
        LOG_INFO << "group id: " << create_response.group_id();
        conn->send(send_str);
    }
    else
    {
        LOG_ERROR << "Create group failed: " << create_response.msg();
        conn->send(send_str);
    }

}
// 处理加入群组业务
void InterfaceService::AddGroup(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf
    Ye_Interface::UnifiedMessage add_request;
    add_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    Ye_GroupService::AddGroupRequest request;
    request.set_userid(add_request.id());
    request.set_group_id(add_request.group_id());
    request.set_role(add_request.role());
    
    // 2. 调用rpc服务
    Ye_GroupService::AddGroupResponse add_response;
    Ye_GroupService::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.AddGroup(&controller, &request, &add_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "add group rpc call failed: " << controller.ErrorText();
        return;
    }
    // 3. 序列化并发送响应给客户端
    Ye_Interface::UnifiedMessage response;
    response.set_group_id(add_response.group_id());
    response.set_type("AddGroup");
    response.set_is_success(add_response.success());
    response.set_msg(add_response.msg());
    string send_str = response.SerializeAsString();
    if (add_response.success())
    {
        LOG_INFO << "Add group success";
        conn->send(send_str);
    }
    else
    {
        LOG_ERROR << "Add group failed: " << add_response.msg();
        conn->send(send_str);
    }

    // 4.序列化并发送响应给客户端
    // string send_str = add_response.SerializeAsString();
    // conn->send(send_str);
}

// 处理获取群组列表业务
void InterfaceService::GroupList(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf
    Ye_Interface::UnifiedMessage group_request;
    group_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    Ye_GroupService::GroupListRequest request;
    request.set_userid(group_request.id());
    
    // 2. 调用rpc服务
    Ye_GroupService::GroupListResponse group_response;
    Ye_GroupService::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.GroupList(&controller, &request, &group_response, nullptr);
    if (controller.Failed())
    {
        LOG_ERROR << "group list rpc call failed: " << controller.ErrorText();
        return;
    }

    // 3. 序列化并发送响应给客户端
    Ye_Interface::UnifiedMessage response;
    response.set_type("GroupList");
    response.set_is_success(group_response.success());
    response.set_msg(group_response.msg());
    if (group_response.groups_size() > 0)
    {
        for (int i = 0; i < group_response.groups_size(); ++i)
        {
            Ye_Interface::UnifiedMessage::GroupInfo *group_info = response.add_groups();
            group_info->set_group_id(group_response.groups(i).group_id());
            group_info->set_group_name(group_response.groups(i).group_name());
            group_info->set_group_desc(group_response.groups(i).group_desc());
            for (int j = 0; j < group_response.groups(i).users_size(); ++j)
            {
                Ye_Interface::UnifiedMessage::UserInfo *user_info = group_info->add_users();
                user_info->set_id(group_response.groups(i).users(j).id());
                user_info->set_name(group_response.groups(i).users(j).name());
                user_info->set_state(group_response.groups(i).users(j).state());
                user_info->set_role(group_response.groups(i).users(j).role());
            }
        }
        string send_str = response.SerializeAsString();
        LOG_INFO << "Get group list success";
        conn->send(send_str);
    }
    else
    {
        string send_str = response.SerializeAsString();
        LOG_ERROR << "Get group list failed";
        conn->send(send_str);
    }
}
// 处理获取群组用户id列表业务,同时转发消息
void InterfaceService::GetGroupUsers(const muduo::net::TcpConnectionPtr &conn, std::string &recv_buf, muduo::Timestamp time)
{
    // 1. 反序列化recv_buf
    Ye_Interface::UnifiedMessage group_request;
    group_request.ParseFromString(recv_buf);
    // 1.1 构造rpc请求
    Ye_GroupService::GetGroupUsersRequest request;
    request.set_group_id(group_request.group_id());
    request.set_userid(group_request.id());


    // 2. 调用rpc服务
    Ye_GroupService::GetGroupUsersResponse group_response;
    Ye_GroupService::GroupServiceRpc_Stub group_stub(new MyRpcChannel());
    MyRpcController controller;
    group_stub.GetGroupUsers(&controller, &request, &group_response, nullptr);
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
                _offlineMsg.OfflineMsgInsert(user_info.id(), group_request.msg());
            }
        }
    }
    else
    {
        LOG_ERROR << "Get group users failed";
    }
}

