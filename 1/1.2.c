#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define MAX_EVENTS_NUMBER 5

int set_non_blocking(int fd)
{
    int old_state = fcntl(fd, F_GETFL);
    int new_state = old_state | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_state);

    return old_state;
}

// 向 epoll 实例中添加要监听的文件描述符，并设置为非阻塞模式
void addfd(int epollfd, int fd)
{
    struct epoll_event event;
    event.events = EPOLLIN; // 监听读事件
    event.data.fd = fd;     // 要监听的文件描述符
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event); //epoll_ctl 函数用于向 epoll 实例中添加或修改事件 EPOLL_CTL_ADD表示添加
    set_non_blocking(fd); // 设置文件描述符为非阻塞模式
}

int main(int argc, char* argv[])
{
    if (argc <= 2)
    {
        printf("Usage: %s ip_address portname\n", argv[0]);
        return 0;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int ret = bind(listenfd, (struct sockaddr*)(&address), sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENTS_NUMBER];
    int epollfd = epoll_create(5);
       /*返回一个整数值，表示 epoll 实例的文件描述符。在失败时，该函数会返回 -1。*/
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    while (1)
    {
        int number = epoll_wait(epollfd, events, MAX_EVENTS_NUMBER, -1);
        //epoll_wait 函数会将触发的事件信息填(epollfd)充到 events 数组中，而不会修改 events 数组本身
//       maxevents：events 数组的大小，即能够存储的最大事件数量。
//       timeout：等待事件发生的超时时间，如果传递 -1，表示无限期   等         待，直到有事件触发。
        if (number < 0)
        {
            printf("epoll_wait failed\n");
            return -1;
        }

        for (int i = 0; i < number; ++i)
        {
            const auto& event = events[i];
            const auto eventfd = event.data.fd;

            if (eventfd == listenfd)
            {// 区分套接字，不然也可以用事件判断
                struct sockaddr_in client;
                socklen_t client_addrlength = sizeof(client);
                int sockfd = accept(listenfd, (struct sockaddr*)(&client), &client_addrlength);
                if (sockfd < 0)
                {
                    perror("accept error");
                    continue;
                }
                addfd(epollfd, sockfd);
            }
            else if (event.events & EPOLLIN)
            { // EPOLLIN：读命令
                char buf[1024] = {0};
                while (1)
                {
                    memset(buf, '\0', sizeof(buf));
                    int recv_size = recv(eventfd, buf, sizeof(buf) - 1, 0); // 0表示默认接收方式，-1是为了'\0'表示字符串终止
                    if (recv_size < 0)
                    {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            //                             EAGAIN：表示 "Try again"，即当前操作没有立即就绪，需要稍后再试。这个错误码通常出现在非阻塞 I/O 操作时，当数据暂时不可用时会返回该错误码，而不会阻塞等待数据就绪。
                            // EWOULDBLOCK：在很多系统EWOULDBLOCK 和 EAGAIN 实际上是相同的错误码。它们用来表示非阻塞操作无法立即完成的情况
                            break;
                        }
                        printf("sockfd %d，接收消息失败\n", eventfd);
                        close(eventfd);
                        break;
                    }
                    else if (recv_size == 0)
                    {
                        // 连接被客户端关闭。
                        close(eventfd);
                        break;
                    }
                    else
                    {
                        // 在此处理接收到的数据。
                        // ...
                        printf("Received message: %s\n", buf);
                        send(eventfd, buf, recv_size, 0);
                    }
                }
            }
        }
    }

    close(listenfd);

    return 0;
}
