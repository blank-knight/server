#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc,char* argv[])
/*argc是命令行总的参数个数argv[]是argc个参数，其中第0个参数是程序的全名，以后的参数
命令行后面跟的用户输入的参数*/
{
	if (argc <= 2)
	{
		printf( "Usage: %s ip_address portname\n", argv[0] );
		return 0;
	}

	const char* ip = argv[1];
	int port = atoi( argv[2] );
    /* int atoi(const char *str) 把参数 str 所指向的字符串转换为一个整数（类型为 int 型）0：表示根据指定的类型和协议，系统会自动选择合适的协议*/
	int listenfd = socket( PF_INET, SOCK_STREAM, 0 ); //PF_INET和AF_INET在套接字编程中通常被用来指代同一个值：2，它们都表示IPv4地址族（Address Family）没有本质的区别。
	assert( listenfd >= 3 );/*在Unix-like操作系统中，0是标准输入（stdin）的文件描述符，1是标准输出（stdout）的文件描述符，2是标准错误（stderr）的文件描述符。触发 assert 断言失败，导致程序终止，并输出错误信息。*/

	struct sockaddr_in address;
	memset( &address, 0, sizeof( address ) );
	address.sin_family = AF_INET;
	address.sin_port = htons( port );
	assert(inet_pton( AF_INET, ip, &address.sin_addr ) == 1);
	//inet_pton() 将人类可读的 IP 地址字符串转换为网络字节序的二进制格式,函数返回一个整数值，它用于指示转换是否成功。如果转换成功，返回值为1；如果IP地址字符串无效或者不支持的地址族，返回值为0；如果发生错误，返回值为-1。

	int ret = 0;
	ret = bind( listenfd, (struct sockaddr*)( &address ), 
				sizeof( address ) ); // 返回值0表示成功，-1表示失败
	assert( ret != -1 );

	ret = listen( listenfd, 5 );// 返回值0表示成功，-1表示失败
	assert( ret != -1 );
	
	struct sockaddr_in client;
	socklen_t client_addrlength = sizeof( client );
	int sockfd = accept( listenfd, (struct sockaddr*)( &address ), &client_addrlength );
				   	  	 
	char buf_size[1024] = {0};
	int recv_size = 0;
	recv_size = recv( sockfd, buf_size, sizeof( buf_size ) , 0);
	/*  0 是表示接收操作是默认的阻塞操作，即如果没有数据可接收，recv() 函数将会一直阻塞等待
		返回值有以下几种情况：
			大于0，表示接收到了有效的数据字节数。可以通过读取 buf_size 缓冲区来获取接收到的数据。
			等于0，表示连接的对方已经关闭了连接。
			为-1，表示接收数据时发生错误。在这种情况下，通常会设置全局变量 errno 来指示具体的错误原因。*/
	
	int send_size = 0;
	strcpy(buf_size, "jojojo");
	send_size = send( sockfd, buf_size , sizeof(buf_size) , 0 );
	/*
		0 是用于控制发送操作的参数。0，表示发送操作是默认的阻塞操作，即如果发送缓冲区当前不可用（例如缓冲区满），send() 函数将会一直阻塞等待，直到数据可以被发送为止。
		send() 函数的返回值有以下几种情况：
			大于0且小于 recv_size，表示只有部分数据发送成功。这可能是因为发送缓冲区已满而导致的。需要根据具体应用逻辑来处理。
			等于 recv_size，表示所有数据都已成功发送。
			-1，表示发送数据时发生错误。在这种情况下，通常会设置全局变量 errno 来指示具体的错误原因。
	*/
	close( sockfd );
	close( listenfd );

	return 0;
}
