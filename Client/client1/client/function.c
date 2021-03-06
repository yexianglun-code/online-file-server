#include "./include/function.h"

int login(int sfd, char *name)	//账户登陆验证
{
	char user_name[64];
	char *passwd, *ciphertext;
	char salt[64] = { 0 };
	Data_pac data_pac;
	
	bzero(user_name, sizeof(user_name));
	printf("请输入用户名:");
	scanf(" %s", user_name); //输入用户名

	strcpy(name, user_name);

	bzero(&data_pac, sizeof(Data_pac));
	strcpy(data_pac.buf, user_name);
	data_pac.len = strlen(data_pac.buf);
	sendn(sfd, (char *)&data_pac, data_pac.len + 6);	//发送用户名
	bzero(&data_pac, sizeof(Data_pac));
	recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
	recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state)); //接收用户名检测结果
	recvn(sfd, data_pac.buf, data_pac.len);

	if(data_pac.state == 1)	//用户名不存在
	{
		printf("登陆失败\n");
		printf("%s\n", data_pac.buf);
		return 0; //提示用户是否要注册
	}
	else if(data_pac.state == 2) //用户名存在
	{
		bzero(&data_pac, sizeof(Data_pac));
		recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));	//接收盐值长度
		recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
		recvn(sfd, data_pac.buf, data_pac.len);	//接收盐值
		strcpy(salt, data_pac.buf);	//获取盐值
		passwd = getpass("请输入密码:");
		ciphertext = crypt(passwd, salt);	//用盐值加密密码
		bzero(&data_pac, sizeof(Data_pac));
		data_pac.len = strlen(ciphertext);
		strcpy(data_pac.buf, ciphertext);
		sendn(sfd, (char *)&data_pac, data_pac.len + 6);	//发送密文
		bzero(&data_pac, sizeof(Data_pac));
		recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
		recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
		recvn(sfd, data_pac.buf, data_pac.len);	//接收登陆提示
		if(data_pac.state == 3)
		{
			printf("登陆成功\n");
			return 1;
		}
		else if(data_pac.state == 4)
		{
			printf("登陆失败\n");
			printf("%s\n", data_pac.buf);
			close(sfd);
			return -1;
		}
	}
	return 0;
}

int signup(int sfd) //用户注册
{
	char ch;
	Data_pac data_pac;
	
	printf("需要注册吗?[y/n]\n");
	scanf(" %c", &ch);
	if(ch == 'n')
	{
		bzero(&data_pac, sizeof(Data_pac));
		data_pac.state = 55; //表示不想注册
		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //通知服务端
		return 0;	
	}
	else if(ch == 'y')
	{
		bzero(&data_pac, sizeof(Data_pac));
		data_pac.state = 54; //表示想要注册
		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //通知服务端
		
		bzero(&data_pac, sizeof(Data_pac));
		recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
		recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
		recvn(sfd, data_pac.buf, data_pac.len);	//接收服务器返回消息

		if(data_pac.state == 6) //服务端允许注册
		{
			char user_name[64], salt[64];
			char *passwd, *passwd2;
			char *ciphertext;
			int OK;

			OK = 0;
	
			while(OK != 1)
			{
				printf("请输入新用户名：");
				bzero(user_name, sizeof(user_name));
				scanf("%s", user_name);
				OK = check_user_signup_name(user_name);
				if(OK == 0)
				{
					printf("用户名超出64个字符的长度限制\n");
				}
				else if(OK == -1)
				{
					printf("用户名只能包含[0-9]、[a-z]、[A-Z]\n");
				}
				else if(OK == 1) //用户名似乎ok
				{
					bzero(&data_pac, sizeof(Data_pac));
					strcpy(data_pac.buf, user_name);
					data_pac.len = strlen(data_pac.buf);
					sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知服务端,查看是否重名
					
					bzero(&data_pac, sizeof(Data_pac));
					recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
					recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
					recvn(sfd, data_pac.buf, data_pac.len);	//接收服务器返回消息
					if(data_pac.state == 8) //重名
					{
						OK = -2; //表示重名
						printf("%s\n", data_pac.buf);
					}
					else if(data_pac.state == 9) //没有重名
					{
						//
					}			
				}
			}
			if(OK == 1) //用户名OK
			{
				bzero(&data_pac, sizeof(Data_pac));
				recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
				recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
				recvn(sfd, data_pac.buf, data_pac.len);	//接收服务器返回盐值
				bzero(salt, sizeof(salt));
				strcpy(salt, data_pac.buf);
			}

			OK = 0;
			while(OK != 1)
			{
				passwd = getpass("请输入密码：");
				passwd2 = getpass("请再次输入密码以确认无误：");
				if(strcmp(passwd, passwd2) == 0)
				{
					OK = 1;		
				}
				else
				{
					free(passwd);
					free(passwd2);
					printf("密码输入不一致\n");
				}
			}
			if(OK == 1) //密码OK
			{
				ciphertext = crypt(passwd, salt); //密码密文

				bzero(&data_pac, sizeof(Data_pac));
				strcpy(data_pac.buf, ciphertext);
				data_pac.len = strlen(data_pac.buf);
				sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知服务端,密码密文
				
				bzero(&data_pac, sizeof(Data_pac));
				recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
				recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
				recvn(sfd, data_pac.buf, data_pac.len);	//接收注册结果
				
				if(data_pac.state == 10) //注册成功
				{
					printf("%s\n", data_pac.buf);
				}
				else if(data_pac.state == 11) //注册失败
				{
					printf("%s\n", data_pac.buf);
				}
			}
			return 1; //表示注册成功
		}
	}
	else
	{
		printf("输入有误\n");
		return -1;
	}
}

int check_user_signup_name(char *user_name) //检查注册用户名
{
	int i, n;
	n = strlen(user_name);
	if(n > 64)
	{
		return 0; //用户名超出长度
	}
	for(i=0;user_name[i] != '\0';i++)
	{
		if(!(user_name[i]>='0'&&user_name[i]<='9' || user_name[i]>='a'&&user_name[i]<='z' || user_name[i]>='A'&&user_name[i]<='Z'))
		{
			return -1; //用户名不符合规定
		}
	}
	return 1;
}

int command(int sfd, char *user_name) //客户端命令接口界面
{
	int i;
	char cmd_str[600], cmd[10], cmd_content[512];
	char tmpc;
	Data_pac data_pac;

	user_help(); //一开始登陆，默认弹出帮助

	int j=0;
	system("stty erase ^H"); //为了实现退格删除输入
cmd_start:
	while(1) //用户输入命令
	{
		if(j++ > 0)
		{
			printf("[%s@dd网盘]> ", user_name);
			fflush(stdout);
		}
		i = 0;
		bzero(cmd_str, sizeof(cmd_str));
		fflush(stdin);
		while((tmpc = getchar()) != '\n' && i < 600)
		{
			if(tmpc == '\b')
			{
				printf("\b \b");
				fflush(stdout);
			}
			else
			{
				cmd_str[i++] = tmpc;
			}
		}
		
		bzero(cmd, sizeof(cmd));
		bzero(cmd_content, sizeof(cmd_content));
		get_cmd(cmd_str, cmd, cmd_content);	//剥离出具体命令

		bzero(&data_pac,sizeof(Data_pac));
		strcpy(data_pac.buf, cmd_content); //命令内容
		data_pac.len = strlen(data_pac.buf);

		////////////////////////////////////////////////////////
		/////////////////////*执行命令*/////////////////////////
		////////////////////////////////////////////////////////

		/*cd命令*/
		//用户切换目录
		if(strcmp(cmd, "cd") == 0)   
		{
			int ret;
			data_pac.state = 101; //表示cd命令
			check_path(data_pac.buf, &ret);	//检测路径是否合法
			if(ret == 1)
			{
				sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送命令
				bzero(&data_pac, sizeof(data_pac));
				recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
				recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
				recvn(sfd, data_pac.buf, data_pac.len); //接收服务器返回消息
				if(data_pac.state == 1002)
				{
					printf("%s\n", data_pac.buf);
				}
				else if(data_pac.state == 1003)
				{
					printf("%s\n", data_pac.buf);
				}
				else if(data_pac.state ==1004)
				{
					printf("%s\n", data_pac.buf);
				}
				else if(data_pac.state ==1005)
				{
					printf("%s\n", data_pac.buf);
				}
			}
			else
			{
				printf("输入有误:路径中不能含有字符'\'， '\n'\n");
			}
		}


		/*ls命令*/
		//查看用户当前所在目录下的文件信息
		else if(strcmp(cmd, "ls") == 0)   
		{
			data_pac.state = 102; //表示ls命令
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送命令

			bzero(&data_pac, sizeof(data_pac));
			recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
			recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
			recvn(sfd, data_pac.buf, data_pac.len); //接收服务器返回消息

			printf("%s\n", data_pac.buf);
		}


		/*pwd命令*/
		//请求显示用户当前所在路径
		else if(strcmp(cmd, "pwd") == 0)   
		{
			data_pac.state = 103; //表示pwd命令
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送命令
			bzero(&data_pac, sizeof(data_pac));
			recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
			recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
			recvn(sfd, data_pac.buf, data_pac.len); //接收服务器返回消息
			printf("%s\n", data_pac.buf); //显示用户当前路径
		}


		/*puts命令*/
		//用户上传文件///
		else if(strcmp(cmd, "puts") == 0)    
		{
			int ret;
			char filename[128]={0};
			char file_path[256] = { 0 };
			check_filename(data_pac.buf, &ret); //检查文件名是否合法
			if(ret == 1)
			{
				strcpy(filename, data_pac.buf);
				sprintf(file_path, "%s/%s", MY_FILE_DIR, data_pac.buf); //要上传的文件的路径
				data_pac.state = 104; //表示puts命令
				sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送命令(发送文件名)
				bzero(&data_pac, sizeof(data_pac));
				recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
				recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
				recvn(sfd, data_pac.buf, data_pac.len); //接收服务器的通知信号

				if(data_pac.state == 1088) //文件重名
				{
					printf("%s\n", data_pac.buf); //打印服务端返回错误
				}
				else if(data_pac.state == 1086) //服务端可以开始接收文件了
				{
					int fd = open(file_path, O_RDONLY);
					char md5_str[33] = {0};
					struct stat filestat;

					if(-1 == fd)
					{
						perror("open");
						return -1;
					}
					bzero(&filestat, sizeof(filestat));
					ret = fstat(fd, &filestat);
					get_file_md5(file_path, md5_str);
					if(-1 == ret)
					{
						perror("fstat");
						return -1;
					}
					bzero(&data_pac, sizeof(Data_pac));
					data_pac.state = 1087; //表示客户端可以开始上传文件
					strcpy(data_pac.buf, md5_str); //文件MD5码值
					data_pac.len = strlen(data_pac.buf);
					sendn(sfd, (char *)&data_pac, data_pac.len + 6);

					bzero(&data_pac, sizeof(Data_pac));
					my_lltoa(data_pac.buf, filestat.st_size); //要上传的文件大小
					data_pac.len = strlen(data_pac.buf);
					sendn(sfd, (char *)&data_pac, data_pac.len + 6);

					bzero(&data_pac, sizeof(data_pac));
					recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
					recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
					recvn(sfd, data_pac.buf, data_pac.len); //接收服务器的通知信号
					
					if(1093 == data_pac.state) //非秒传
					{
						/////////////////////*传送文件*////////////////////					
						if(filestat.st_size <= FILE_LIMIT) //普通上传模式
						{
							ret = transfile(sfd, fd, 1); 
						}
						else //快速上传模式（sendfile方式）
						{
							ret = transfile(sfd, fd, 2);
							//usleep(100000);
						}
						//if(ret == 1)
						//{
							//bzero(&data_pac, sizeof(Data_pac));
							//data_pac.state = 1090; //表示客户端发送完毕
							//sendn(sfd, (char *)&data_pac, data_pac.len + 6); 

						//}
						//else if(ret == -1)
						//{
						//	bzero(&data_pac, sizeof(Data_pac));
						//	data_pac.state = 1091; //表示客户端出错,发送中止
						//	sendn(sfd, (char *)&data_pac, data_pac.len + 6); 
						//}
					}
					else if(1092 == data_pac.state) //秒传
					{
						printf("%s ", filename);
						print_progress_bar(1, 1);
					}
					bzero(&data_pac, sizeof(Data_pac));
					recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
					recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
					recvn(sfd, data_pac.buf, data_pac.len);
					if(data_pac.state == 1082)
					{
						printf("puts 成功\n");
					}
					else if(data_pac.state == 1083)
					{
						printf("put 失败\n");
					}
					close(fd);
				}
				else
				{
					//服务器异常
				}
			}
			else
			{
				printf("输入有误:文件名中不能含有字符'\'， '\n'，'/'\n");
			}
		}


		/*gets命令*/
		//用户下载文件
		else if(strcmp(cmd, "gets") == 0)   
		{
			int ret;
			char file_path[256] = { 0 };
			char filename[128]={0};
			off_t filesize; //文件大小

			check_filename(data_pac.buf, &ret); //检查输入文件名是否合法
			if(ret != 1)
			{
				printf("输入有误:文件名中不能含有字符'\'， '\n'，'/'\n");
			}
			else
			{
				int fd;
				struct stat filestat;

				strcpy(filename, data_pac.buf);
				sprintf(file_path, "%s/%s", MY_DOWNLOAD_DIR, filename);//下载文件的存放位置
				fd = open(file_path, O_RDWR|O_CREAT, 0664);
				if(-1 == fd)
				{
					perror("open");
					goto cmd_start;
				}
				bzero(&filestat, sizeof(filestat));
				fstat(fd, &filestat);
				lseek(fd, filestat.st_size, SEEK_SET); //偏移到上次下载的地方
				
				bzero(&data_pac, sizeof(Data_pac));
				data_pac.state = 105; //表示gets命令
				sprintf(data_pac.buf, "%s %ld", filename, filestat.st_size);
				printf("data_pac.buf = %s\n", data_pac.buf);
				data_pac.len = strlen(data_pac.buf);
				sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送命令(发送文件名和大小)

				bzero(&data_pac, sizeof(Data_pac));
				recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
				recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
				recvn(sfd, data_pac.buf, data_pac.len); //接收服务器的通知信号
				if(data_pac.state == 1065) //服务器没有此文件
				{
					printf("%s\n", data_pac.buf);
					close(fd);
					unlink(file_path); //删除文件
				}
				else if(data_pac.state == 1064) //可以下载此文件
				{
					bzero(&data_pac, sizeof(Data_pac));
					recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
					recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
					recvn(sfd, data_pac.buf, data_pac.len); //接收服务器的通知信号	
					filesize = atoll(data_pac.buf); //要下载的文件大小

					if(data_pac.state == 1069) //服务端准备好了
					{
						bzero(&data_pac, sizeof(Data_pac));
						data_pac.state = 1068; //表示客户端准备好了
						sendn(sfd, (char *)&data_pac, data_pac.len + 6);

						off_t download_len_of_file = 0;
						///////////////////////*从服务端下载文件*//////////////////////////
						if(filesize <= FILE_LIMIT) //普通下载方式
						{
							while(bzero(&data_pac, sizeof(Data_pac)), (ret = recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len))) == 0)
							{
								recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
								//if(data_pac.state == 1074 || data_pac.state == 1075)
								//{ //收到服务端传送完毕的信号
								//	break;
								//}
								recvn(sfd, data_pac.buf, data_pac.len);
								download_len_of_file += write(fd, data_pac.buf, data_pac.len); //把从客户端接收的文件内容写入本地磁盘中
								
								printf("%s ", filename);
								print_progress_bar(download_len_of_file, filesize);	//打印进度条
								if(download_len_of_file == filesize)
								{
									break;
								}
							}	
						}
						else //服务端用sendfile方式提供下载
						{
							int recv_buf_len = 0;
							char *recv_buf = (char *)calloc(FILE_LIMIT, sizeof(char)); 
							while(1)
							{
								recv_buf_len = recv(sfd, recv_buf, FILE_LIMIT, 0);
								if(-1 == recv_buf_len)
								{
									perror("recv");
									break;
								}
								download_len_of_file +=  write(fd, recv_buf, recv_buf_len);
								bzero(recv_buf, sizeof(recv_buf));
								
								printf("%s ", filename);
								print_progress_bar(download_len_of_file, filesize);	//打印进度条
								if(download_len_of_file == filesize)
								{
								//	bzero(&data_pac, sizeof(Data_pac));
								//	recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
								//	recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
								//	recvn(sfd, data_pac.buf, data_pac.len); //接收服务器的通知信号
									break;
								}
							}
						}
						//if(data_pac.state == 1074) //服务端发送完毕
						//{
							bzero(&data_pac, sizeof(Data_pac));
							data_pac.state = 1072; //表示客户端下载完成
							sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知服务端
							printf("下载完毕\n");
						//}
						//else if(data_pac.state == 1075) //服务端出错，发送中止
						//{
						//	printf("服务端出错，下载中止\n");
						//	unlink(file_path); //删除文件
						//}
						close(fd);
					}	
				}
				else
				{
					printf("%s\n", data_pac.buf);
					close(fd);
					unlink(file_path); //删除文件
				}
			}
		}


		/*remove命令*/
		//用户删除文件
		else if(strcmp(cmd, "remove") == 0)   
		{
			data_pac.state = 106; //表示remove命令
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送命令

			bzero(&data_pac, sizeof(Data_pac));
			recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
			recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
			recvn(sfd, data_pac.buf, data_pac.len); //接收服务器的通知信号
			if(data_pac.state == 1102)
			{
				printf("%s\n", data_pac.buf);
			}
			else if(data_pac.state == 1103)
			{
				printf("%s\n", data_pac.buf);
			}
		}


		/*mkdir命令*/
		//用户创建目录命令
		else if(strcmp(cmd, "mkdir") == 0)   
		{
			data_pac.state = 107; //表示mkdir命令
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送命令

			bzero(&data_pac, sizeof(Data_pac));
			recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
			recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
			recvn(sfd, data_pac.buf, data_pac.len); //接收服务器的通知信号
			if(data_pac.state == 1122)
			{
				printf("%s\n", data_pac.buf);
			}
			else if(data_pac.state == 1123)
			{
				printf("%s\n", data_pac.buf);
			}
		}


		/*exit命令*/
		//用户退出下线
		else if(strcmp(cmd, "exit") == 0)
		{
			return 0;
		}


		/*help命令*/
		//用户退出下线
		else if(strcmp(cmd, "help") == 0)
		{
			int len = strlen(data_pac.buf);
			if(len == 0)	
			{
				user_help();
			}
			else if(len > 0)
			{
				if(strcmp(data_pac.buf, "--clear") == 0) //清屏
				{
					system("clear");
				}
			}
		}


		/*其他命令*/
		//其它命令不响应
		else 
		{

		}
	}
}

void get_cmd(char *cmd_str, char *cmd, char *cmd_content) //剥离命令字符串，命令存入cmd，命令内容存入cmd_content
{
	int i = 0;
	char *left = cmd_str;
	while(*left == ' ')
	{
		left++;	//跳过开头空格
	}
	while(*left != ' ' && *left != '\n' && *left != '\0' && i < 10)
	{
		*cmd = *left;
		cmd++;
		left++;
		i++;	//计算cmd字符串数组空闲位置，以防越界
	}
	if(*left == '\0' || *left == '\n'  || i == 10)
	{
		return;
	}
	while(*left == ' ')
	{
		left++;	//跳过空格
	}
	i = 0;
	while(*left != '\0' && *left != '\n' && i < 512)
	{
		*cmd_content = *left;
		cmd_content++;
		left++;
		i++;	//计算cmd字符串数组空闲位置，以防越界
	}
}



void check_path(char *path, int *ret) //检查路径是否合法
{
	char *left = path;
	*ret = 1;
	while(*left != '\0')
	{
		if(*left == '\\' || *left == '\n')
		{
			*ret = -1;
			return;
		}
		left++;
	}
}

void check_filename(char *path, int *ret) //检查文件名是否合法
{
	char *left = path;
	*ret = 1;
	while(*left != '\0')
	{
		if(*left == '\\' || *left == '/' || *left == '\n')
		{
			*ret = -1;
			return;
		}
		left++;
	}
}

void print_progress_bar(off_t download_len_of_file, off_t filesize) //打印下载进度条
{
	int rate = 0;
	char str[102] = {0};
	const char * ptr = "|/-\\";
	if(download_len_of_file <= filesize)
	{
		rate = (int)((double)download_len_of_file/(double)filesize * 100);
		for(int i=0;i<=rate;i++)
		{
			str[i] = '='; 
		}
		printf("[%-100s] [%d\%][%c]\r", str, rate, ptr[rate%4]);
		fflush(stdout);
		if(rate == 100)
		{
			printf("\n");
		}
	}
}

void my_lltoa(char *dst, off_t filesize) //将文件大小转换成字符串
{
	int i, digit;
	int left, right;
	char tmpc;

	i = digit = 0;
	while(filesize > 0)
	{
		digit = filesize % 10;
		filesize /= 10;
		dst[i++] = digit + '0';
	}
	
	left = 0;
	right = i-1;
	while(left < right)
	{
		tmpc = dst[right];
		dst[right] = dst[left];
		dst[left] = tmpc;
		left++;
		right--;
	}
}

int get_file_md5(const char *file_path, char *md5_str) //由路径file_path中的目标文件的内容生成32位MD5码字符串，存入md5_str
{
	int i, ret;
	int fd;
	unsigned char data[1024];
	unsigned char md5_value[16];
	MD5_CTX md5;

	fd = open(file_path, O_RDONLY);
	if(-1 == fd)
	{
		perror("open");
		return -1;
	}
	MD5_Init(&md5);
	while(1)
	{
		ret = read(fd, data, 1024);
		if(-1 == ret)
		{
			perror("read");
			return -1;
		}
		MD5_Update(&md5, data, ret);
		if(0 == ret || ret < 1024)
		{
			break;
		}
	}
	close(fd);
	MD5_Final(md5_value, &md5);
	for(i=0; i < 16;i++)
	{
		snprintf(md5_str + i*2, 2+1, "%02x", md5_value[i]);
	}
	md5_str[32] = '\0';
	return 0;
}

void user_help() //用户帮助函数
{
	printf("------------------------------------------------------------------------------------------------------------------\n");
	printf("---------------------------------------------------膜法指南-------------------------------------------------------\n");
	printf("------------------------------------------------------------------------------------------------------------------\n\n");
	
	printf("命令:\n");
	
	printf("	  help:\n");
    printf("		用法:help [参数]\n");
	printf("		功能:参数为空，直接呼出本函数\n");
	printf("			 参数 --clear，清屏\n");
	printf("		注意:不支持路径分析，[服务器文件名]中不要包含路径\n");
	printf("\n");
	
	printf("	  exit:\n");
    printf("		用法:exit\n");
	printf("		功能:退出下线\n");
	printf("\n");
	
	printf("	  cd:\n");
    printf("		用法:cd [路径]\n");
	printf("		功能:进入指定路径中的目录\n");
	printf("		注意:虽然支持简单的路径分析，但不要尝试进入不存在的路径或别人的文件夹\n");
	printf("\n");

	printf("	  ls:\n");
    printf("		用法:ls\n");
	printf("		功能:显示当前目录下的文件信息\n");
	printf("		注意:不支持路径分析，ls后面不要加任何东西，所以不要尝试进入不存在的路径或别人的文件夹\n");
	printf("\n");
	
	printf("	  pwd:\n");
    printf("		用法:pwd\n");
	printf("		功能:显示用户当前所在绝对路径\n");
	printf("\n");

	printf("	  puts:\n");
    printf("		用法:puts [本地文件名]\n");
	printf("		功能:上传客户端本地文件到服务器中的当前目录\n");
	printf("		注意:不支持路径分析，[本地文件名]中不要包含路径，要上传的文件请提前放在CLIENT_RESOURCE/myfile中\n");
	printf("\n");

	printf("	  gets:\n");
    printf("		用法:gets [服务器文件名]\n");
	printf("		功能:从服务器的用户当前目录下载文件到客户端的默认文件夹\n");
	printf("		注意:不支持路径分析，[服务器文件名]中不要包含路径，下载存放的默认文件夹是CLIENT_RESOURCE/download\n");
	printf("\n");

	printf("	  remove:\n");
    printf("		用法:remove [服务器文件名或目录名]\n");
	printf("		功能:将服务器的用户当前目录的指定文件删除\n");
	printf("		注意:不支持路径分析，[服务器文件名]中不要包含路径，不能删除非空目录\n");
	printf("\n");

	printf("	  mkdir:\n");
    printf("		用法:mkdir [目录名]\n");
	printf("		功能:在服务器的用户当前目录中创建目录\n");
	printf("		注意:不支持路径分析，[目录名]中不要包含路径\n");
	printf("\n");
}
