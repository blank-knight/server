#ifndef __PROCESSPOLL_H_ // ?? 
#define __PROCESSPOLL_H_

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

class process
{
 public:
  int pid;              // 进程的pid
  int pipe[2];          // 父子通信管道 父 pipe[0] 子 pipe[1] socketpair双向通信
 
  process() : pid( -1 ), pipe{ 0, 0 } {}
};

template < typename T > // ??
class processpool
{
 private:
  static const int MAX_EVENTS_NUMBER = 5;
  static const int MAX_USER_PER_PROCESS = 10000; 
  int idx;                  
  int listenfd;
  int epollfd;
  int max_processes_num;
  process* sub_processes; // 通信管道
//   单例模式确保一个类的唯一性，这样可以避免多次创建相同实例的开销，同时也方便在程序的任何地方访问该实例。
  static processpool<T>* instance; 	// 涉及内存泄漏 由于是singleton模型 ？？？

  processpool( int listenfd, int max_processes_num = 8 );
  ~processpool()
  {
	delete [] sub_processes;
  }

 public:
  static processpool<T>* create( int listenfd, int _max_processes_num = 8 )
  {// 为什么不拿到外面写？
      if( instance == nullptr )
	  {
	      instance = new processpool<T>( listenfd, _max_processes_num );
		  return instance;
	  }

	  return instance;
  } 

  void run();
  void run_parent();
  void run_child();
  void setup_up_sig();
};

template < typename T>
processpool< T >* processpool< T > :: instance = nullptr;

template < typename T >
processpool<T>::processpool( int listenfd, int _max_processes_num ):
 						   idx( -1 ), listenfd( listenfd ), epollfd ( 0 ),
						   max_processes_num( _max_processes_num ), sub_processes( nullptr )
{
	sub_processes = new process [ max_processes_num ];
	for( int i = 0; i < max_processes_num; ++i )
	{
		socketpair( PF_UNIX, SOCK_STREAM, 0, sub_processes[i].pipe );//同一主机上的不同进程之间进行双向通信
		sub_processes[i].pid = fork(); // 子进程fork后会从fork处开始执行
		if( sub_processes[i].pid > 0 )	// 父进程 关闭子进程方的pipe
		{
			close( sub_processes[i].pipe[1] );
			continue;
		}
		else
		{
			close( sub_processes[i].pipe[0] );
			idx = i;
			break;
		}
	}
}

static int set_non_blocking( int fd )
{
    int old_state = fcntl( fd, F_GETFL );//查询旧状态
    int new_state = old_state | O_NONBLOCK; // 设为非阻塞
    fcntl( fd, F_SETFL, new_state ); // 设为非阻塞

    return old_state;//返回原始的状态，以便在需要时可以恢复文件描述符的阻塞模式状态
}

static void addfd( int epollfd , int fd )
{
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
	//ET当文件描述符上有新的事件发生时，只会触发一次通知。换句话说，只有在状态发生变化时，才会触发事件通知。相比之下，另一种触发模式是水平触发（Level-Triggered）模式，在该模式下，只要文件描述符处于可读或可写状态，事件就会不断触发，直到相关的数据被处理完毕。
    event.data.fd = fd;
    epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
    set_non_blocking( fd );
}

static void removefd( int epollfd, int fd )
{
	epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, nullptr );
	close( fd );
}

template < typename T >
void processpool< T > :: run()
{
    if( idx == -1 )
	{
	    run_parent();
	}
	else
	{
	    run_child();
	}
}

template < typename T >
void processpool< T > :: setup_up_sig()
{
	epollfd = epoll_create( 5 );
    assert( epollfd != -1 );
}

template < typename T >
void processpool< T > :: run_parent()
{
    epoll_event events[ MAX_EVENTS_NUMBER ];
	setup_up_sig();

    addfd( epollfd, listenfd );

	int pre_idx = 0;
	int has_new_cli = 0;
	int number = 0;
	while( 1 )
	{
		number = epoll_wait( epollfd , events, MAX_EVENTS_NUMBER, -1 );
		
		for( int i = 0; i < number; ++i )
		{
			int sockfd = events[i].data.fd;
			if( sockfd == listenfd ) 
			{
				int pos = pre_idx;
				do
				{ // 负载均衡
					pos = ( pos + 1 ) % max_processes_num;
				}
				while( sub_processes[pos].pid == -1 ); 
				pre_idx = pos;

				send( sub_processes[pos].pipe[0], ( void* )&has_new_cli, sizeof( has_new_cli ), 0 );
				//0: 这是发送消息的标志，通常为 0 表示没有特殊选项。
				printf( "parent processes has sent msg to %d child\n", pos );
			}	
		}
	}

	// close( pipe[0] );
}


template < typename T >
void processpool< T > :: run_child()
{
	epoll_event events[ MAX_EVENTS_NUMBER ];
	setup_up_sig();

	int pipefd = sub_processes[idx].pipe[1];
	addfd( epollfd, pipefd );
	T* users = new T [MAX_USER_PER_PROCESS];

	int number = 0;
	while( 1 )
	{
		number = epoll_wait( epollfd , events, MAX_EVENTS_NUMBER, -1 );
		for( int i = 0; i < number; ++i )
		{
			int sockfd = events[i].data.fd;
			if( sockfd == pipefd && ( events[i].events & EPOLLIN ) )
			{
				struct sockaddr_in client;
                socklen_t client_addrlength = sizeof( client );
                int connfd = accept( listenfd, ( struct sockaddr* )( &client ),
                                     &client_addrlength );
				addfd( epollfd, connfd );
				users[connfd].init( epollfd, connfd, client );
				printf( "child %d is addfding \n", idx );
				continue;
			}
			else if( events[i].events & EPOLLIN )
			{
				printf( "child %d has recv msg\n", idx );
				users[sockfd].process();
			}
		}
	}

	delete [] users;
	users = nullptr;

	close( epollfd );
	close( pipefd );}

#endif
