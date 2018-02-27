#include "../include/factory.h"

int fds_quit[2];
MYSQL *conn;	//数据库连接句柄

void * th_func(void *p);	//子线程入口函数
void sig_exit(int signum);	//退出程序函数
//void get_server_conf(char *conf_file, char argv[]);

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		//printf("argc errors! ./thread_pool_server [IP] [PORT] [NUM_OF_THREAD] [CAPACITY]\n");
		printf("argc errors! It needs [../conf/server.conf] to initialize the ftpserver\n");
		return -1;
	}
	FILE  *fp_conf = fopen(argv[1], "r"); //打开服务器配置文件
	if(NULL == fp_conf)
	{
		perror("fopen");
		return -1;
	}

	pfac pfactory;
	pque_t pq;
	pnode_t pnew;
	char server_ip[16], server_port[6];
	int i, j;
	int ret, reuse;
	int thread_num, capacity;
	int epfd, sfd, newfd, num_of_act_fd;
	int client_addr_len;
	struct epoll_event event, *evs;
	struct sockaddr_in server_addr, client_addr;
	
	bzero(server_ip, sizeof(server_ip));
	bzero(server_port, sizeof(server_port));
	fscanf(fp_conf, "%s%s%d%d",  server_ip, server_port, &thread_num, &capacity); //读取服务器配置参数
	fclose(fp_conf);

	////初始化线程池
	pfactory = (pfac)calloc(1, sizeof(factory));
	//thread_num = atoi(argv[3]);
	//capacity = atoi(argv[4]);
	factory_init(pfactory, thread_num, capacity, th_func); //初始化线程池
	factory_start(pfactory);	//启动线程池
	pq = &pfactory->que;	//获取任务队列
	
	////服务端数据初始化
	sfd = socket(AF_INET, SOCK_STREAM, 0);	//生成一个套接口描述符
	if(-1 == sfd)
	{
		perror("socket");
		return -1;
	}
	printf("sfd = %d\n", sfd);
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	//server_addr.sin_port = htons(atoi(argv[2]));	//服务器端口
	//server_addr.sin_addr.s_addr = inet_addr(argv[1]);	//服务器IP
	server_addr.sin_port = htons(atoi(server_port));	//服务器端口
	server_addr.sin_addr.s_addr = inet_addr(server_ip);	//服务器IP
	reuse = 1;
	ret = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));//设置sfd属性
	if(-1 == ret)
	{
		perror("setsockopt");
		close(sfd);
		return -1;
	}
	ret = bind(sfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));	//用指定ip和端口号绑定套接口
	if(-1 == ret)
	{
		perror("bind");
		close(sfd);
		return -1;
	}

	epfd = epoll_create(1);
	bzero(&event, sizeof(struct epoll_event));
	evs = (struct epoll_event *)calloc(2, sizeof(struct epoll_event));
	event.events = EPOLLIN;
	event.data.fd = sfd;
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &event);	//监听sfd
	if(-1 == ret)
	{
		perror("epoll_ctl add sfd");
		return -1;
	}
 
	/////退出机制初始化
	pipe(fds_quit);	//创建用于退出机制的管道
	event.events = EPOLLIN;
	event.data.fd = fds_quit[0];
	ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fds_quit[0], &event);	//监听fds_quit[0]
	if(-1 == ret)
	{
		perror("epoll_ctl add fds_quit[0]");
		return -1;
	}
	signal(SIGQUIT, sig_exit);	//注册退出函数

	/////连接数据库
	db_conn(&conn);
///*测试数据库*/
////char query[300] = "insert into User(user_name,salt,ciphertext,user_dir) VALUES('admin', 'wq.a2A', 'Axzca214Xas', '/admin')";
////char query[300] = "insert into File(pre_file_id, filename, type, owner, owner_id) value(0,'a.txt','f','admin',1)";
////db_insert(conn, query, &ret);
////char query[300] = "UPDATE File SET filename = 'b.txt' WHERE owner_id=1";
////db_update(conn, query, &ret);
////char query[300] = "DELETE from File WHERE filename='b.txt'";
////db_delete(conn, query, &ret);
////printf("query insert ret = %d\n", ret);
////while(1);

//	/////生成测试用户
//	for(int i=6;i<=6;i++)
//	{
//		db_produce_test_data(conn, i);
//		sleep(1);
//	}
	

	/////服务端监听客户端连接请求
	ret = listen(sfd, capacity);	
	printf("开始监听\n");
	if(-1 == ret)
	{
		perror("listen");
		close(sfd);
		return -1;
	}


	openlog("ftpserver", LOG_PID | LOG_NOWAIT | LOG_CONS, LOG_USER); //日志
	////服务端开始工作，接受客户端连接，安排线程接客
	while(1)
	{
		bzero(evs, sizeof(evs));
		num_of_act_fd = epoll_wait(epfd, evs, 2, -1);	//等待相应描述符的读事件出现；永久阻塞
		for(i=0;i<num_of_act_fd;i++)
		{
			if(evs[i].data.fd == fds_quit[0])	//监测到要退出
			{
				printf("exiting...\n");
				char quit;
				read(fds_quit[0], &quit, sizeof(quit));	//读取退出通知
				event.events = EPOLLIN;
				event.data.fd = sfd;
			   	epoll_ctl(epfd, EPOLL_CTL_DEL, sfd, &event);	//解注册sfd
				close(sfd);	//断绝新客户端连接请求
				
				while(1)	//等待任务队列中的任务全部完成，再退出
				{
					pthread_mutex_lock(&pq->que_mutex);
					if(pq->que_size == 0)
					{
						pthread_mutex_unlock(&pq->que_mutex);
						break;
					}
					pthread_mutex_unlock(&pq->que_mutex);
				}
				factory_stop(pfactory);	//停止线程池
				pthread_cond_broadcast(&pfactory->cond);	//唤醒所用睡眠的子线程，再回收
				for(j=0;j<thread_num;j++)
				{
					pthread_join(pfactory->pthid[j], NULL);	//回收子线程
				}
				printf("main thread exit...pid=%d\n", getpid());
				mysql_close(conn);
				closelog(); //日志
				exit(0); //主线程最后退出
			}
			if(evs[i].data.fd == sfd)	//客户端有连接请求
			{
				char client_addr_ip[16] = {0};
				
				bzero(&client_addr, sizeof(client_addr));
				client_addr_len = sizeof(client_addr);
				newfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);
				pnew = (pnode_t)calloc(1, sizeof(node_t));
				pnew->nd_sockfd = newfd; //新连接的socket fd
				strcpy(client_addr_ip, inet_ntoa(client_addr.sin_addr));
				strcpy(pnew->nd_IP, client_addr_ip); //新连接的IP地址

				pthread_mutex_lock(&pq->que_mutex);	//上锁，互斥操作队列
				que_set(pq, pnew);	//新任务入队			
				pthread_mutex_unlock(&pq->que_mutex);	//解锁
				pthread_cond_signal(&pfactory->cond);	//唤醒子线程
		
				syslog(LOG_INFO, "IP=%s%s\n", client_addr_ip, " is connected");
			}
		}
	}
	return 0;
}

void * th_func(void *p)	//线程入口函数
{
	pfac pf = (pfac)p;	//强制转换类型为线程池结构体指针
	pque_t pq = &pf->que;	//获取队列
	pnode_t pnode;
	int ret, user_id;
	while(1)
	{
		if(pf->flag == 1)	//线程池是启动的
		{
			pthread_mutex_lock(&pq->que_mutex);	//上锁，互斥访问队列
			if(pq->que_size == 0)	//队列为空，没有任务
			{	
				pthread_cond_wait(&pf->cond, &pq->que_mutex);	//等待激发
			}
			if(pf->flag == 0)	//等待激发过程中，主线程关闭了线程池
			{
				pthread_mutex_unlock(&pq->que_mutex);	//解锁
				printf("child thread exit\n");
				pthread_exit(NULL);
			}
			//队列不空，有任务
			que_get(pq, &pnode);	//获取队首结点，取得任务
			pthread_mutex_unlock(&pq->que_mutex);	//解锁
			printf("child thread:I am busy, newfd=%d\n", pnode->nd_sockfd);
			
			ret = user_verify(conn, pnode->nd_sockfd, &user_id, pnode->nd_IP);	//账户验证
			if(ret == 1)	//ret==1,表示验证成功
			{
				printf("账户验证成功, newfd=%d\n", pnode->nd_sockfd);
				command(conn, pnode->nd_sockfd, user_id); //开始服务
			}
			else
			{
				printf("账户验证失败, newfd=%d\n", pnode->nd_sockfd);
				if(ret == -1)
				{
					ret = user_signup(conn, pnode->nd_sockfd, pnode->nd_IP); //注册
				}
			}
			close(pnode->nd_sockfd);
			free(pnode);	//释放空间
		}
		else	//线程池停止，子线程要退出
		{
			printf("child thread exit\n");
			pthread_exit(NULL);
		}
	}
}

void sig_exit(int signum)
{
	char q = 'q';
	printf("%d is coming\n", signum);
	write(fds_quit[1],&q, sizeof(q)); 
}
