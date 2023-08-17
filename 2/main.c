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

#include "processpool.h"
#include "echo.h"
// nc是netcat的简写，被设计为一个简单、可靠的网络工具。比如大家很熟悉使用telnet测试tcp端口，而nc可以支持测试linux的tcp和udp端口，而且也经常被用于端口扫描，甚至把nc作为server以TCP或UDP方式侦听指定端口做简单的模拟测试。
int main( int argc , char* argv[] )
{
	if (argc <= 2)
	{
		printf( "Usage: %s ip_address portname\n", argv[0] );
		return 0;
	}

	const char* ip = argv[1];
	int port = atoi( argv[2] );
    
	int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
	assert( listenfd >= 1 );

	struct sockaddr_in address;
	memset( &address, 0, sizeof( address ) );
	address.sin_family = AF_INET;
	address.sin_port = htons( port );
	inet_pton( AF_INET, ip, &address.sin_addr );

	int ret = 0;
	ret = bind( listenfd, (struct sockaddr*)( &address ), 
				sizeof( address ) );
	assert( ret != -1 );

	ret = listen( listenfd, 5 );
	assert( ret != -1 );
	
	processpool<echo>* pool = processpool<echo>::create( listenfd, 8 );
    // 创建了一个进程池 pool，用于处理通过 listenfd 监听的套接字接收的客户端请求。进程池中有 8 个子进程并发地处理这些客户端请求，具体的任务处理逻辑由模板参数 echo 指定
	pool->run();

	close( listenfd );

	return 0;
}
