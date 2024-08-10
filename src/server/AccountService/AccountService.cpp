
#include "AccountService.h"
// 注册业务
void AccountService::Register(::google::protobuf::RpcController *controller,
                const ::Ye_AccountService::RegisterRequest *request,
                ::Ye_AccountService::RegisterResponse *response,
                ::google::protobuf::Closure *done)
{
    const std::string &name = request->name();
    const std::string &password = request->password();

    User user;
    user.SetName(name);
    user.SetPwd(password);
    bool state = _userService.Insert(user);

    if (state)
    {
        response->set_is_success(true);
        response->set_id(user.GetId());
        // std::cout << "name: " << name << " password: " << password << std::endl;
    }
    else
    {
        response->set_is_success(false);
        response->set_msg("AccountService -- Register service failed");
    }
    done->Run();

}
// 登录业务
void AccountService::Login(::google::protobuf::RpcController *controller,
            const ::Ye_AccountService::LoginRequest *request,
            ::Ye_AccountService::LoginResponse *response,
            ::google::protobuf::Closure *done)
{
    const int id = request->id();
    const std::string &password = request->password();

    User user = _userService.Query(id);
    if (user.GetId() == id && user.GetPwd() == password)
    {
        // 用户已经登录
        if(user.GetState() == "online")
        {
            response->set_is_success(false);
            response->set_msg("AccountService -- Login service failed, user already login");
        }
        else
        {
            response->set_is_success(true);
            response->set_id(user.GetId());
            response->set_name(user.GetName());
            user.SetState("online");
            _userService.UpdateState(user);

            // 查询离线消息
            std::vector<std::string> offlineMsg = _offlineMsg.OfflineMsgQuery(id);
            if(!offlineMsg.empty())
            {
                for(auto &msg : offlineMsg)
                {
                    response->add_offline_msg(msg);
                }
                // 删除离线消息
                _offlineMsg.OfflineMsgRemove(id);
            }
        }
    }
    else if(user.GetId() == id && user.GetPwd() != password)
    {
        response->set_is_success(false);
        response->set_msg("AccountService -- Login service failed, password error");
    }
    else
    {
        response->set_is_success(false);
        response->set_msg("AccountService -- Login service failed, user not exist");
    }

    done->Run();
}