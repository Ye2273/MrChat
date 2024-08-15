# MrChat-分布式集群聊天服务器

> 开发环境：ubuntu linux环境
>
> 编程语言：C++
>
> 编译工具：CMake
>
> 序列化工具：Protocol Buffers
>
> RPC框架：自研的MyRpc框架
>
> 数据库：MySQL、Redis
>
> 网络库：muduo库
>
> 负载均衡器：nginx


> https://blog.csdn.net/weixin_67134938/article/details/141197140
## 1、项目介绍

### 1.1、整个聊天系统的框架图

![image-20240813204847219](https://github.com/Ye2273/MrChat/blob/master/README.assets/image-20240813204847219.png)

### 1.2、”分布式”，“集群”

整个项目想要体现的核心思想和功能主要是这两个方面

#### 分布式：

一个工程拆分了很多模块，每一个模块独立部署运行在一个服务器主机上，所有服务器协同工作共同提供服务，每一台服务器称作分布式的一个节点，根据节点的并发要求，对一个节点可以再做节点模块集群部署。![image-20240813211034331](https://github.com/Ye2273/MrChat/blob/master/README.assets/image-20240813211034331.png)

##### 业务服务器群

不同的业务逻辑（如注册登录、好友管理、群组管理、聊天）被分布在不同的服务器上。

这种设计实现了业务的分离，使得每个服务器只专注于处理特定类型的任务，降低了单一服务器的负载压力，并提高了系统的可扩展性。

##### ZooKeeper

每个事务服务器在启动时都会将自身信息注册到ZooKeeper中。网关服务器通过ZooKeeper获取每个事务服务器的服务地址，从而实现服务的动态发现和调用。这一机制确保了系统能够在服务变动（如扩展或故障）时自动调整，从而保持系统的稳定性和可用性。

##### 网关服务器

RPC远程调用的核心所在，在业务服务器这边扮演“client”的角色。而在客户端角度则扮演网关接口服务器的角色。

###### 1. **客户端请求的入口**

- **统一接入点**：网关服务器作为客户端与后端服务之间的桥梁，接收来自客户端的所有请求。客户端不需要直接与多个事务服务器交互，而是通过网关服务器来进行统一访问。这简化了客户端的逻辑，也增强了系统的安全性和可维护性。

###### 2. **请求的分发和路由**

- **动态路由**：当网关服务器接收到客户端的请求后，会根据请求的类型，动态选择合适的事务服务器来处理请求。这个选择过程依赖于从ZooKeeper获取的最新服务地址信息，从而实现请求的准确路由。这种动态路由机制可以应对事务服务器的增加或减少，保证了系统的灵活性。



#### 集群

**集群**的设计通过使用负载均衡、消息队列和集群数据库，使得系统在处理大量并发请求时能够保持高效和稳定的运行。

<img src="https://github.com/Ye2273/MrChat/blob/master/README.assets/image-20240813213226421.png" alt="image-20240813213226421" style="zoom: 40%;" />——-<img src="https://github.com/Ye2273/MrChat/blob/master/README.assets/image-20240813213244090.png" alt="image-20240813213304577" width="500" height="500" />

##### **Nginx负载均衡**：

Nginx在项目中扮演了负载均衡器的角色，它可以将来自客户端的请求均匀地分发到后端的网关接口类服务器上。这种设计不仅提高了系统的并发处理能力，还增强了系统的可用性，因为单个接口类服务器的故障不会导致系统不可用。

##### **Redis消息队列**：

Redis被用于处理跨服务器的消息传递问题。通过Redis的发布/订阅机制，不同服务器上的用户可以实现实时通信，这在聊天服务中尤为关键。这种设计使得系统即使在多服务器分布的情况下，仍然能够保证消息的即时传递和处理。

##### **MySQL集群**：

数据的存储被集中在MySQL数据库中，并且通过合理的设计确保了数据的一致性和可靠性。事务服务器通过与数据库的交互，完成对用户数据和群组数据的持久化存储，这保证了数据的安全性和系统的可恢复性。



> 仅供学习之用~~
