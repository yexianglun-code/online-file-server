#include "./include/client.h"

int fds_quit[2];
void sig_exit(int signum); //客户端退出下线处理函数

////客户端
int main(int argc, char *argv[])	//argv[1]是ip，argv[2]是端口号
{
	if(3 != argc)
	{
		printf("argc errors!./client [IP] [PORT]\n");
		return -1;
	}
	int ret, sfd, fd, len;
	struct sockaddr_in server_addr;
	char buf[1000];
	Data_pac data_pac;

	////数据初始化
	sfd = socket(AF_INET, SOCK_STREAM, 0);	//生成一个套接口描述符
	if(-1 == sfd)
	{
		perror("socket");
		return -1;
	}
	bzero(&server_addr, sizeof(server_addr));	//初始化服务端的相关数据
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	ret = connect(sfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));	//连接服务端
	if(-1 == ret)
	{
		perror("connect");
		return -1;
	}

	/////客户端退出机制
	pipe(fds_quit);
	signal(SIGQUIT, sig_exit);
	//signal(SIGINT, sig_exit);


	int serv_num;
	char user_name[64]={0};
		
	//char tmp_ch;
	printf("1.登陆\n");
	printf("2.注册\n");
	printf("请输入号码:");
	fflush(stdout);
	scanf("%d", &serv_num);
	if(serv_num == 1)
	{
		//system("clear");
		/////账户验证登陆
		ret = login(sfd, user_name);	
		if(ret == 0) //提示用户注册
		{
			ret = signup(sfd);
			if(ret == -1 || ret == 0)
			{
				close(sfd);
				exit(-1);
			}
			else if(ret == 1)
			{
				printf("请重新登陆\n");
				close(sfd);
				exit(0);
			}
		}
	}
	else if(serv_num == 2)
	{
		ret = signup(sfd);
		if(ret == -1 || ret == 0)
		{
			close(sfd);
			exit(-1);
		}
		else if(ret == 1)
		{
			printf("请重新登陆\n");
			close(sfd);
			exit(0);
		}
	}


	//////账户通过验证后，开始向服务器请求服务
	if(ret == 1)
	{
		if(!fork())
		{	//子进程
			ret = command(sfd, user_name);
			if(ret == 0)
			{
				exit(0); //子进程退出
			}
		}
		else
		{	//父进程
			wait(NULL);
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 51;
			data_pac.len = 0;
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //通知服务器正常下线
			exit(0);

		//	short state;
		//	read(fds_quit[0], (char *)&state, sizeof(state));
		//	if(state == 51)
		//	{
		//		bzero(&data_pac, sizeof(Data_pac));
		//		data_pac.state = 51;
		//		data_pac.len = 0;
		//		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //通知服务器正常下线
		//		exit(0);
		//	}
		//	else if(state == 52)
		//	{
		//		bzero(&data_pac, sizeof(Data_pac));
		//		data_pac.state = 52;
		//		data_pac.len = 0;
		//		printf("1\n");
		//		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //通知服务器强制退出下线
		//		printf("2\n");
		//		exit(0);
		//	}
		}
	}
	return 0;
}

void sig_exit(int signum) //客户端退出下线处理函数
{
	short state;
	if(signum == SIGQUIT) //正常退出下线
	{
	//	printf("%d is coming\n", SIGQUIT);
		state = 51;
		write(fds_quit[1], (char *)&state, sizeof(state));
	}
//	else if(signum == SIGINT) //强制退出下线
//	{
//		printf("%d is coming\n", SIGINT);
//		state = 52;
//		write(fds_quit[1], (char *)&state, sizeof(state));
//	}
}
