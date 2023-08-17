#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>


#define MAXLINE 80
#define SERV_PORT 8000

#define CL_CMD_REG 'r'
#define CL_CMD_LOGIN 'l'
#define CL_CMD_CHAT 'c'

void GetName(char str[],char szName[]) 
{ 
	// 从一个字符串中提取出特定的信息，并将结果存储在szName数组中
	//char str[] ="a,b,c,d*e"; 
	const char * split = ","; 
	char * p; 
	p = strtok (str,split); 
	int i=0;
	while(p!=NULL) 
	{ 
		printf ("%s\n",p); 
	    if(i==1) sprintf(szName,p);
		i++;
		p = strtok(NULL,split); 
	}
}

//查找字符串中某个字符出现的次数
int countChar(const char *p, const char chr)
{	
	int count = 0,i = 0;
	while(*(p+i))
	{
		if(p[i] == chr)//字符数组存放在一块内存区域中，按索引找字符，指针本身不变
			++count;
		++i;// 按数组的索引值找到对应指针变量的值
	}
	//printf("字符串中w出现的次数：%d",count);
	return count;
}
 

int main(int argc, char *argv[])
{
	int i, maxi, maxfd;
	int listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE];
	ssize_t n;
 
	char szName[255]="",szPwd[128]="",repBuf[512]="";
	
	
	//两个集合
	fd_set rset, allset;
	
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN]; /* #define INET_ADDRSTRLEN 16 */
	socklen_t cliaddr_len;
	struct sockaddr_in cliaddr, servaddr;
	
	//创建套接字
	listenfd = socket(AF_INET, SOCK_STREAM, 0); // socket还能用于一台主机间的进程通信
	// 地址协议族，通信传输形式，使用协议（一个socket同时只能用于一个协议和功能）
	
	int val = 1;
	int ret = setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,(void *)&val,sizeof(int));
	// (void *)&val 强行转换为万能指针

	//绑定
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //？？ 为什么不是ntohl
	//表示监听0.0.0.0地址，socket只绑定端口，不绑定本主机的某个特定ip，让路由表决定传到哪个ip
	//（0.0.0.0地址表示所有地址、不确定地址、任意地址）（一台主机中如果有多个网卡就有多个ip地址）
	//（路由表应该能知道这个端口正在由哪个ip监听）
	servaddr.sin_port = htons(SERV_PORT);


	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	
	//监听
	listen(listenfd, 20); /* 默认最大128 */
	//需要接收最大文件描述符

	maxfd = listenfd; 
	
	//数组初始化为-1
	maxi = -1; 
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;
	
	//集合清零
	FD_ZERO(&allset);
	
	//将listenfd加入allset集合
	FD_SET(listenfd, &allset);
	
	puts("Chat server is running...");

	
	for (; ;) 
	{
		//关键点3
		rset = allset; /* 每次循环时都重新设置select监控信号集 */
		
		//select返回rest集合中发生读事件的总数  参数1：最大文件描述符+1
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
		/*maxfd：监视对象文件描述符数量。
		readset：将所有关注“是否存在待读取数据”的文件描述符注册到fd_set变量，并传递其地址值。
		writeset： 将所有关注“是否可传输无阻塞数据”的文件描述符注册到fd_set变量，并传递其地址值。
		exceptset：将所有关注“是否发生异常”的文件描述符注册到fd_set变量，并传递其地址值。
		timeout：调用select后，为防止陷入无限阻塞状态，传递超时信息。
		返回值：错误返回-1，超时返回0。当关注的事件返回时，返回大于0的值，该值是发生事件的文件描述符数。s*/ 

		if (nready < 0)
			puts("select error");
		
		//listenfd是否在rset集合中
		if (FD_ISSET(listenfd, &rset))
		{
			//accept接收
			cliaddr_len = sizeof(cliaddr);
			//accept返回通信套接字，当前非阻塞，因为select已经发生读写事件
			connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
 
			
			
			printf("received from %s at PORT %d\n",
				inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str)),
				ntohs(cliaddr.sin_port));
			
		 //关键点1	
			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) 
				{
					client[i] = connfd; /* 保存accept返回的通信套接字connfd存到client[]里 */
					break;
				}
				
            /* 是否达到select能监控的文件个数上限 1024 */
			if (i == FD_SETSIZE) {
				fputs("too many clients\n", stderr);
				exit(1);
			}
			
			//关键点2
			FD_SET(connfd, &allset); /*添加一个新的文件描述符到监控信号集里 */
			
			//更新最大文件描述符数
			if (connfd > maxfd)
				maxfd = connfd; /* select第一个参数需要 */
			if (i > maxi) // maxi用于寻找哪些有数据
				maxi = i; /* 更新client[]最大下标值 */
			
			/* 如果没有更多的就绪文件描述符继续回到上面select阻塞监听,负责处理未处理完的就绪文件描述符 */
			if (--nready == 0)
				continue; 
		}
		
		for (i = 0; i <= maxi; i++) 
		{ 
			//检测clients 哪个有数据就绪
			if ((sockfd = client[i]) < 0)
				continue;
			
			//sockfd（connd）是否在rset集合中
			if (FD_ISSET(sockfd, &rset)) 
			{

				//进行读数据 不用阻塞立即读取（select已经帮忙处理阻塞环节）
				if ((n = read(sockfd, buf, MAXLINE)) == 0)// 等于0时，是特殊的EOF数据包，长度为0
				{
                    /* 无数据情况 client关闭链接，服务器端也关闭对应链接 */
					close(sockfd);
					FD_CLR(sockfd, &allset); /*解除select监控此文件描述符 */
					client[i] = -1;
				}
				else 
				{
					char code= buf[0];
					switch(code)
					{
					case CL_CMD_REG:   //注册命令处理
						if(1!=countChar(buf,','))
						{
							puts("invalid protocal!");
							break;
						}
						printf("++++++++++++++++++");
						GetName(buf,szName);
						
						//判断名字是否重复
			
						if(IsExist(szName))
						{
							printf("+++++%d",IsExist(szName));
							sprintf(repBuf,"r,exist");
						}
						else
						{
							printf("++++++%d",IsExist(szName));
							insert(szName);
							showTable();
							sprintf(repBuf,"r,ok");
							printf("reg ok,%s\n",szName);
						}
						write(sockfd, repBuf, strlen(repBuf));//回复客户端

						break;
					case CL_CMD_LOGIN: //登录命令处理
						if(1!=countChar(buf,','))
						{
							puts("invalid protocal!");
							break;
						}

						GetName(buf,szName);
						
						//判断是否注册过，即是否存在
						if(IsExist(szName))
						{
							sprintf(repBuf,"l,ok");
							printf("login ok,%s\n",szName);
						}
						else sprintf(repBuf,"l,noexist");
						write(sockfd, repBuf, strlen(repBuf));//回复客户端
						break;
					case CL_CMD_CHAT://聊天命令处理
						puts("send all");

						//群发
						for(i=0;i<=maxi;i++)
							if(client[i]!=-1)
								 write(client[i], buf+2, n);//写回客户端，+2表示去掉命令头(c,)，这样只发送聊天内容
						break;
					}//switch



					
					
				}
				if (--nready == 0)
					break;
			}
		} 
			
	}
	close(listenfd);
	return 0;
}
 