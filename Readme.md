## 介绍
C++ webServer学习项目

## 文件详情
chatSrv :
1. select模型实现的并发聊天服务器项目

epoll：
1. 基于epoll模型实现的回显服务器

processpool:
1. 使用线程池优化的epoll回显服务器。
2. 使用ET事件触发模式，提高整体效率。
3. 父进程监听listenfd然后选择空闲子线程，通过管道通信告知使其处理客户端请求。
