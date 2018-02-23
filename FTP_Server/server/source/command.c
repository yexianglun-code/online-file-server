#include "../include/command.h"

void command(MYSQL *conn, int sfd, int user_id) //解析客户端发送过来的命令，并执行相应任务
{
	int ret;
	Data_pac data_pac;

	while(bzero(&data_pac, sizeof(Data_pac)), (ret = recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len))) == 0) //接收命令长度
	{
		recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
		recvn(sfd, data_pac.buf, data_pac.len);	//接收命令内容

		if(data_pac.state == 51 || data_pac.len == 52) //客户端退出下线
		{
			//printf("client is exiting, newfd=%d\n", sfd);
			
			char log_username[64]={0};
			char query[600];
			MYSQL_RES *res;
			MYSQL_ROW row;
			int ret, db_num_rows;
			sprintf(query, "select user_name from User where user_id=%d", user_id);
			db_select(conn, query, &res, &ret);
			db_num_rows = mysql_num_rows(res);
			if(db_num_rows > 0)
			{
				row = mysql_fetch_row(res);
				strcpy(log_username, row[0]);
			}

			syslog(LOG_INFO|LOG_USER, "username=%s 下线\n", log_username);
			return;
		}
		else if(data_pac.state == 101) //命令cd
		{
			cmd_cd(conn, sfd, user_id, data_pac.buf); 
		}
		else if(data_pac.state == 102) //命令ls
		{
			cmd_ls(conn, sfd, user_id, data_pac.buf); 
		}
		else if(data_pac.state == 103) //命令pwd
		{
			cmd_pwd(conn, sfd, user_id, data_pac.buf); 
		}
		else if(data_pac.state == 104) //命令puts
		{
			cmd_puts(conn, sfd, user_id, data_pac.buf); 
		}
		else if(data_pac.state == 105) //命令gets
		{
			cmd_gets(conn, sfd, user_id, data_pac.buf); 
		}
		else if(data_pac.state == 106) //命令remove
		{
			cmd_remove(conn, sfd, user_id, data_pac.buf); 
		}
		else if(data_pac.state == 107) //命令mkdir
		{
			cmd_mkdir(conn, sfd, user_id, data_pac.buf); 
		}
		else //其它命令不响应
		{

		}
	}
	return;
}

void cmd_cd(MYSQL *conn, int sfd, int user_id, char *cmd_content)	//进入指定目录
{
	char src_path[256], abs_path[256], relative_path[256], query[600];
   	char log_username[64]={0};
	int user_dir_file_id;
	int ret, db_num_rows;
	MYSQL_RES *res;
	MYSQL_ROW row;
	Data_pac data_pac;
	
	bzero(query, sizeof(query));
	sprintf(query, "select user_name from User where user_id=%d", user_id);
	db_select(conn, query, &res, &ret);
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows > 0)
	{
		row = mysql_fetch_row(res);
		strcpy(log_username, row[0]);
	}

	bzero(src_path, sizeof(src_path));
	bzero(abs_path, sizeof(abs_path));
	bzero(relative_path, sizeof(relative_path));
	
	strcpy(src_path, cmd_content); //原路径
	ret = get_path(conn, sfd, user_id, src_path, abs_path, relative_path, &user_dir_file_id);

	if(ret == 0)
	{
		////修改数据库User表，修改用户当前目录
		bzero(query, sizeof(query));
		strcpy(query, "update User set user_dir=");
		sprintf(query, "%s'%s'%s%d%s%d", query, abs_path, ",user_dir_file_id=", user_dir_file_id, " where user_id=", user_id);
		db_update(conn, query, &ret);
		bzero(&data_pac, sizeof(Data_pac));
		if(ret == 0)
		{
			syslog(LOG_INFO|LOG_USER, "username=%s \"cd %s\" 成功\n", log_username, cmd_content);

			data_pac.state = 1002; //表示cd成功
			strcpy(data_pac.buf, "cd成功\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6);
		}
		else
		{
			syslog(LOG_INFO|LOG_USER, "username=%s \"cd %s\" 失败\n", log_username, cmd_content);
			
			data_pac.state = 1003; //表示cd失败
			strcpy(data_pac.buf, "cd失败\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6);
		}
	}
	else
	{
		syslog(LOG_INFO|LOG_USER, "username=%s \"cd %s\" 失败\n", log_username, cmd_content);
	}
	return;
}

void cmd_ls(MYSQL *conn, int sfd, int user_id, char *cmd_content)	//展示目标目录下文件信息
{
	char query[600], md5_file_path[256];
	char log_username[64]={0}, log_dir[256]={0};
	int i, ret, len;
	int db_num_rows, cur_dir_file_id;
	time_t timep;
	MYSQL_RES *res;
	MYSQL_ROW row;
	Data_pac data_pac;
	struct stat statbuf;
	struct file_info{
		char filename[128];
		char type[2]; //文件类型
		char owner[128]; //文件所用者
		//char md5[34];
		off_t file_size; //文件大小
		char *file_mtime; //文件最后修改时间
	}*file_info_buf; //用来存文件详细信息的结构体数组

	//if(is_empty(cmd_content) == true) //命令内容为空，则默认显示当前目录下的文件信息
	//{
		bzero(query, sizeof(query));
		sprintf(query, "%s%d", "select user_dir_file_id,user_dir from User where user_id=", user_id);
		db_select(conn, query, &res, &ret);
	   	db_num_rows = mysql_num_rows(res);
		if(db_num_rows > 0)
		{
			row = mysql_fetch_row(res);
			cur_dir_file_id = atoi(row[0]);
			strcpy(log_dir, row[1]);
		}
		mysql_free_result(res); //避免内存泄漏
	//}
//	else //否则为指定目标目录
//	{
//		char src_path[256], abs_path[256], relative_path[256];
//		int cur_dir_file_id;
//
//		bzero(src_path, sizeof(src_path));
//		bzero(abs_path, sizeof(abs_path));
//		bzero(relative_path, sizeof(relative_path));
//		strcpy(src_path, cmd_content);
//		ret = get_path(conn, sfd, user_id, src_path, abs_path, relative_path, &cur_dir_file_id);
//	}

	if(ret == 0)
	{
		bzero(query, sizeof(query));
		sprintf(query, "%s%d", "select filename,type,owner,md5 from File where pre_file_id=", cur_dir_file_id);
		db_select(conn, query, &res, &ret);
		db_num_rows = 0;
		db_num_rows = mysql_num_rows(res);
		if(db_num_rows > 0)
		{
			file_info_buf = (struct file_info *)calloc(db_num_rows, sizeof(struct file_info)); //为动态数组开辟内存空间
			for(i=0;i<db_num_rows;i++)
			{
				row = mysql_fetch_row(res);
				strcpy(file_info_buf[i].filename, row[0]);
				strcpy(file_info_buf[i].type , row[1]);
				strcpy(file_info_buf[i].owner , row[2]);
				//strcpy(file_info_buf[i].md5 , row[3]);
	
				if(strcmp(file_info_buf[i].type, "f") == 0)
				{
					bzero(md5_file_path, sizeof(md5_file_path));
					sprintf(md5_file_path, "%s/%s", RESOURCE_FILE_DIR, row[3]);
					
					bzero(&statbuf, sizeof(struct stat));
					ret = stat(md5_file_path, &statbuf); //获取文件信息
					if(-1 == ret)
					{
						perror("stat");
						mysql_free_result(res); //避免内存泄漏
						return;
					}
					file_info_buf[i].file_size = statbuf.st_size; //获取文件大小
					timep = statbuf.st_mtime;
					file_info_buf[i].file_mtime = ctime(&timep);
					len = strlen(file_info_buf[i].file_mtime);
					file_info_buf[i].file_mtime[len-1] = '\0'; 
				}
				else if(strcmp(file_info_buf[i].type, "d") == 0)
				{
					file_info_buf[i].file_size = 4096; //目录大小
					timep = time(NULL);
					file_info_buf[i].file_mtime = ctime(&timep);
					len = strlen(file_info_buf[i].file_mtime);
					file_info_buf[i].file_mtime[len-1] = '\0'; //目录的时间为现在ls的时间
				}

			}
			mysql_free_result(res); //避免内存泄漏
		}
		else if(db_num_rows == 0) //没有文件
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1022; //ls失败，空文件夹
			sendn(sfd, (char *)&data_pac, data_pac.len + 6);
	
			syslog(LOG_INFO|LOG_USER, "username=%s \"ls\" 空文件夹, 当前路径=%s\n", log_username, log_dir);
			return;
		}
	
		bzero(&data_pac, sizeof(Data_pac));
		for(i=0, len=0;i<db_num_rows;i++)
		{
			sprintf(data_pac.buf, "%s%s  %s  %12ld  [%s]  %s\n", data_pac.buf, file_info_buf[i].type, file_info_buf[i].owner, file_info_buf[i].file_size, file_info_buf[i].file_mtime, file_info_buf[i].filename); //拼接结果，最终用于返回给客户端
			//len = strlen(data_pac.buf); //考虑data_pac.buf大小是否足以装下所用信息
			//if(len
		}
		strcpy(log_username, file_info_buf[0].owner);
		data_pac.len = strlen(data_pac.buf);
		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送结果给客户端
	}

	syslog(LOG_INFO|LOG_USER, "username=%s \"ls\" 成功, 当前路径=%s\n", log_username, log_dir);
	
	return;
}

void cmd_pwd(MYSQL *conn, int sfd, int user_id, char *cmd_content)	//显示用户当前所在路径
{
	Data_pac data_pac;
	int db_num_rows, ret;
	MYSQL_RES *res;
	MYSQL_ROW row;
	char dst[20] = "user_name,user_dir";
	char log_username[64]={0}, log_dir[256]={0};
	ret = db_get_user_info_by_id(conn, user_id, dst, &res);
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows > 0)
	{
		row = mysql_fetch_row(res);
		strcpy(log_username, row[0]);
		bzero(&data_pac, sizeof(data_pac));
		sprintf(data_pac.buf, "%s\n", row[1]); //用户当前所在路径
		strcpy(log_dir, row[1]);
		data_pac.len = strlen(data_pac.buf);
		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //返回结果给客户端
	}
	mysql_free_result(res); //避免内存泄漏
	
	syslog(LOG_INFO|LOG_USER, "username=%s \"pwd\" 成功, 返回结果=%s\n", log_username, log_dir);
	
	return;
}

void cmd_puts(MYSQL *conn, int sfd, int user_id, char *cmd_content) //用户上传文件
{
	int ret, db_num_rows;
	//int dir_file_id;
	char dst[20];
	char log_username[64]={0}, log_dir[256]={0};
	//char abs_path[256] = {0}, relative_path[256] = {0};
	MYSQL_RES *res;
	MYSQL_ROW row;

	//get_filename_from_path(cmd_content, filename); //获取文件名
	//get_path(conn, sfd, user_id, cmd_content, abs_path, relative_path, &dir_file_id); //路径分析、拆解

	bzero(dst, sizeof(dst));
	strcpy(dst,	"user_name,user_dir,user_dir_file_id");
	ret = db_get_user_info_by_id(conn, user_id, dst, &res);
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows > 0)
	{
		char type[2] = "f", md5_str[33] = { 0 };
		char owner[64] = { 0 };
		char query[600] = { 0 };
   		char filename[128] = { 0 };
		int pre_file_id;
		Data_pac data_pac;

		strcpy(filename, cmd_content); //文件名
		row = mysql_fetch_row(res);
		strcpy(owner, row[0]); //当前用户名，为文件所有者
		strcpy(log_username, row[0]);
		strcpy(log_dir, row[1]);
		pre_file_id = atoi(row[2]); //用户当前所在目录的文件id
		mysql_free_result(res); //避免内存泄漏

		////检查所要上传的文件的名字在当前目录下是否重名
		strcpy(query, "select filename from File where pre_file_id=");	
		sprintf(query, "%s%d%s'%s'%s%d", query, pre_file_id, " and filename=", filename, " and owner_id=", user_id);
		db_select(conn, query, &res, &ret);
		db_num_rows = 0;
		db_num_rows = mysql_num_rows(res);
		if(db_num_rows > 0)
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1088; //表示服务端不能接收此上传的文件
			strcpy(data_pac.buf, "此目录下文件重名,请修改文件名字或上传到别的目录\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知客户端
			mysql_free_result(res); //避免内存泄漏
				
			syslog(LOG_INFO|LOG_USER, "username=%s \"puts %s\" 文件重名, 当前路径=%s\n", log_username, cmd_content, log_dir);
			
			return;
		}

		char tmp_filename[256] = { 0 };//临时文件名
		char random_str[64] = { 0 }; 		
		int fd;

		get_str_random(random_str, 63);
		sprintf(tmp_filename, "%s/%s_%s_%s", RESOURCE_FILE_DIR, owner, filename, random_str); //拼接路径，生成临时文件名

		fd = open(tmp_filename, O_RDWR | O_CREAT, 0664); //创建文件
		if(-1 == fd)
		{
			perror("open");
			exit(-1);
		}

		bzero(&data_pac, sizeof(Data_pac));
		data_pac.state = 1086; //表示可以开始接收客户端上传文件
		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //通知客户端
		bzero(&data_pac, sizeof(Data_pac));
		recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
		recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
		recvn(sfd, data_pac.buf, data_pac.len);
		if(data_pac.state == 1087) //表示客户端可以开始发送文件
		{
			off_t filesize = atoll(data_pac.buf); //上传的文件大小
			////////////////////////*接收客户端上传的文件*///////////////////////////
			if(filesize <= FILE_LIMIT) //普通上传模式
			{
				while(bzero(&data_pac, sizeof(Data_pac)), (ret = recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len))) == 0)
				{
					recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
					if(data_pac.state == 1090 || data_pac.state == 1091)
					{ //收到上传结束的信号
						break;
					}
					recvn(sfd, data_pac.buf, data_pac.len);
					write(fd, data_pac.buf, data_pac.len); //把从客户端接收的文件内容写入本地磁盘中
				}
			}
			else //快速上传模式
			{
				int recv_buf_len = 0, file_len = 0;
				char *recv_buf = (char *)calloc(FILE_LIMIT, sizeof(char)); 
				while(1)
				{
					recv_buf_len = recv(sfd, recv_buf, FILE_LIMIT, 0);
					if(-1 == recv_buf_len)
					{
						perror("recv");
						syslog(LOG_PERROR|LOG_USER, "username=%s \"puts %s\" 失败,recv错误 当前路径=%s\n", log_username, cmd_content, log_dir);
						return;
					}
					file_len +=  write(fd, recv_buf, recv_buf_len);
					bzero(recv_buf, sizeof(recv_buf));
					
					if(file_len == filesize)
					{
						bzero(&data_pac, sizeof(Data_pac));
						recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
						recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
						recvn(sfd, data_pac.buf, data_pac.len); //接收服务器的通知信号
						break;
					}
				}
			}
			if(data_pac.state == 1090) //传输文件结束
			{
				char md5_str[33] = { 0 }, new_filename[256] = { 0 };

				get_file_md5(tmp_filename, md5_str); //获取文件的客户端上传的文件的md5码值
				sprintf(new_filename, "%s/%s", RESOURCE_FILE_DIR, md5_str);
				rename(tmp_filename, new_filename); //将客户端上传的文件重命名
				bzero(query, sizeof(query));
				strcpy(query, "insert into File(pre_file_id, filename, type, owner, owner_id, md5) values");
				sprintf(query, "%s(%d,'%s','%s','%s',%d,'%s')", query, pre_file_id, filename, type, owner, user_id, md5_str); 
				db_insert(conn, query, &ret); //将文件信息插入数据库
				//printf("ret= %d\n", ret);
				if(ret == 0)
				{
					bzero(&data_pac, sizeof(Data_pac));
					data_pac.state = 1082; //表示客户端puts文件成功
					data_pac.len = 0;
					sendn(sfd, (char *)&data_pac, data_pac.len + 6);  //通知客户端上传文件成功
				
					syslog(LOG_INFO|LOG_USER, "username=%s \"puts %s\" 成功, 当前路径=%s\n", log_username, cmd_content, log_dir);
				
				}
				else
				{
					bzero(&data_pac, sizeof(Data_pac));
					data_pac.state = 1083; //表示客户端puts文件失败
					data_pac.len = 0;
					sendn(sfd, (char *)&data_pac, data_pac.len + 6);  //通知客户端上传文件失败
					
					syslog(LOG_INFO|LOG_USER, "username=%s \"puts %s\" 失败, 当前路径=%s\n", log_username, cmd_content, log_dir);
				
				}
				close(fd);
			}
			else if(data_pac.state == 1091) //客户端传输异常
			{
				close(fd);
				unlink(tmp_filename);
				syslog(LOG_INFO|LOG_USER, "username=%s \"puts %s\" 异常, 当前路径=%s\n", log_username, cmd_content, log_dir);
				//客户端传输异常
			}
		}
		else
		{
			close(fd);
			unlink(tmp_filename);
		}
	}
}

void cmd_gets(MYSQL *conn, int sfd, int user_id, char *cmd_content) //用户下载文件
{
	int ret;
	int db_num_rows;
	//int dir_file_id;
	char dst[20] = {0};
	char filename[128] = {0};
	char log_username[64]={0}, log_dir[256]={0};
	//char src_path[256] = {0};
	//char abs_path[256] = {0}, relative_path[256] = {0};
	MYSQL_RES *res;
	MYSQL_ROW row;

	strcpy(dst,	"user_name,user_dir_file_id, user_dir");
	//strcpy(src_path, cmd_content);
	
//	split_path(src_path, filename);
//	ret = get_path(conn, sfd, user_id, src_path, abs_path, relative_path, &dir_file_id);

	strcpy(filename, cmd_content);
	ret = db_get_user_info_by_id(conn, user_id, dst, &res);
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows > 0)
	{
		char query[600];
		int pre_file_id;
		Data_pac data_pac;

		row = mysql_fetch_row(res);
		strcpy(log_username, row[0]);
		strcpy(log_dir, row[2]);
		pre_file_id = atoi(row[1]); //当前目录的id
		//pre_file_id = dir_file_id;
		mysql_free_result(res); //避免内存泄漏
		
		/////检查此目录下是否有此文件
		bzero(query, sizeof(query));
		strcpy(query, "select md5,type from File where pre_file_id=");	
		sprintf(query, "%s%d%s'%s'", query, pre_file_id, " and filename=", filename);
		db_select(conn, query, &res, &ret);
		db_num_rows = -11;
		db_num_rows = mysql_num_rows(res);
		if(db_num_rows == 0) //当前目录下没有此文件		
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1065; //表示没有此文件
			strcpy(data_pac.buf, "当前目录下没有此文件\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知客户端错误
			mysql_free_result(res); //避免内存泄漏
					
			syslog(LOG_INFO|LOG_USER, "username=%s \"gets %s\" 当前目录没有此文件, 当前路径=%s\n", log_username, cmd_content, log_dir);
			
			return;
		}
		else if(db_num_rows > 0)//有此文件，可以下载
		{
			char md5_filepath[256] = {0};
			char type[2]={0};
			int fd;
			struct stat filestat;

			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1064; //表示客户端可以下载此文件
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知客户端可以下载此文件

			row = mysql_fetch_row(res);

			sprintf(md5_filepath, "%s/%s", RESOURCE_FILE_DIR, row[0]);
			strcpy(type, row[1]); //文件类型
			if(strcmp(type, "d") == 0) //不支持下载目录
			{
				bzero(&data_pac, sizeof(Data_pac));
				data_pac.state = 1067; //表示是目录不可下载
				strcpy(data_pac.buf, "不能下载目录\n");
				data_pac.len = strlen(data_pac.buf);
				sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知客户端
			
				syslog(LOG_INFO|LOG_USER, "username=%s \"gets %s\" 不能下载目录, 当前路径=%s\n", log_username, cmd_content, log_dir);
			
				return;
			}

			fd = open(md5_filepath, O_RDONLY);
			if(-1 == fd)
			{
				perror("open");
				return;
			}
			bzero(&filestat, sizeof(filestat));
			fstat(fd, &filestat);

			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1069; //表示服务器端可以开始好发送文件
			my_lltoa(data_pac.buf, filestat.st_size); //文件大小
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知客户端

			bzero(&data_pac, sizeof(Data_pac));
			recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
			recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
			recvn(sfd, data_pac.buf, data_pac.len);
			if(data_pac.state == 1068) //客户端也准备好可以开始接收文件了
			{
				/////////////*传送文件给客户端*///////////////////
				if(filestat.st_size <= FILE_LIMIT) //如果文件小于等于FILE_LIMIT
				{
					ret = transfile(sfd, fd, 1); //普通下载模式 
				}
				else //文件大小大于FILE_LIMIT
				{
					ret = transfile(sfd, fd, 2); //用sendfile方式进行传送，快速下载模式 
					usleep(100000);
				}
				if(ret == 1)
				{
					bzero(&data_pac, sizeof(Data_pac));
					data_pac.state = 1074; // 表示服务端已经传送完毕
					sendn(sfd, (char *)&data_pac, data_pac.len + 6);
					bzero(&data_pac, sizeof(Data_pac));
					recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
					recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
					recvn(sfd, data_pac.buf, data_pac.len);
					if(data_pac.state == 1072)
					{
						syslog(LOG_INFO|LOG_USER, "username=%s \"gets %s\" 成功, 当前路径=%s\n", log_username, cmd_content, log_dir);
					}
				}
				else if(ret == -1)
				{
					bzero(&data_pac, sizeof(Data_pac));
					data_pac.state = 1075; // 表示服务端出错，发送中止
					sendn(sfd, (char *)&data_pac, data_pac.len + 6);

					syslog(LOG_INFO|LOG_USER, "username=%s \"gets %s\" 失败, 当前路径=%s\n", log_username, cmd_content, log_dir);
				}
				close(fd);
			}
			mysql_free_result(res); //避免内存泄漏
		}
	}
}

void cmd_remove(MYSQL *conn, int sfd, int user_id, char *cmd_content) //用户删除文件
{
	char filename[128], md5_filepath[256], query[600];
	char log_username[64]={0}, log_dir[256]={0};
	int ret;
	int db_num_rows;
	int pre_file_id;
	MYSQL_RES *res;
	MYSQL_ROW row;
	Data_pac data_pac;

	bzero(filename, sizeof(filename));
	bzero(query, sizeof(query));
	strcpy(filename, cmd_content); //要删除的文件的文件名
	sprintf(query, "%s%d", "select user_dir_file_id,user_dir,user_name from User where user_id=", user_id);
	db_select(conn, query, &res, &ret);
	db_num_rows = 0;
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows > 0)
	{
		row = mysql_fetch_row(res);
		pre_file_id = atoi(row[0]);
		strcpy(log_dir, row[1]);
		strcpy(log_username, row[2]);
		mysql_free_result(res); //避免内存泄漏
	}
	bzero(query, sizeof(query));
	sprintf(query, "%s%d%s'%s'", "select file_id,md5 from File where pre_file_id=", pre_file_id, " and filename=", filename); //同一目录下文件不重名
	db_select(conn, query, &res, &ret);
	db_num_rows = 0;
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows > 0)
	{
		int file_id;
		row = mysql_fetch_row(res);
		file_id = atoi(row[0]); //要删除的文件的file_id
		bzero(md5_filepath, sizeof(md5_filepath));
		sprintf(md5_filepath, "%s/%s", RESOURCE_FILE_DIR, row[1]); //实际文件的路径
		mysql_free_result(res); //避免内存泄漏
		
		////先删除数据库File表中的文件的记录，再去文件夹中删除实际文件
		bzero(query, sizeof(query));
		sprintf(query, "%s%d", "delete from File where file_id=", file_id); 
		db_delete(conn, query, &ret); //删除数据库File表中文件的对应记录
		if(ret == 0) //数据库中已删除成功
		{
			ret = unlink(md5_filepath); //再删除实际文件
			if(ret == 0) //实际文件删除成功
			{
				bzero(&data_pac, sizeof(Data_pac));
				data_pac.state = 1102; //表示服务器删除文件成功
				strcpy(data_pac.buf, "remove:删除文件成功\n");
				data_pac.len = strlen(data_pac.buf);
				sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知客户端
						
				syslog(LOG_INFO|LOG_USER, "username=%s \"remove %s\" 成功, 当前路径=%s\n", log_username, cmd_content, log_dir);
			}
			else if(ret == -1)
			{
				perror("unlink");
				bzero(&data_pac, sizeof(Data_pac));
				data_pac.state = 1103; //表示服务器删除文件失败
				strcpy(data_pac.buf, "remove:删除文件失败\n");
				data_pac.len = strlen(data_pac.buf);
				sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知客户端
				
				syslog(LOG_INFO|LOG_USER, "username=%s \"remove %s\" 失败, 当前路径=%s\n", log_username, cmd_content, log_dir);
				
				return;
			}
		}
		else
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1103; //表示服务器删除文件失败
			strcpy(data_pac.buf, "remove:删除文件失败\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //告知客户端
				
			syslog(LOG_INFO|LOG_USER, "username=%s \"remove %s\" 失败, 当前路径=%s\n", log_username, cmd_content, log_dir);
			
			return;
		}
	}
}

void cmd_mkdir(MYSQL *conn, int sfd, int user_id, char *cmd_content) //用户创建目录
{
	char query[600], dirname[128] = {0}, md5_buf[64] = {0};
	char log_username[64]={0}, log_dir[256]={0};
	unsigned char src_str[128] = {0};
	int ret, db_num_rows;
	int pre_file_id;
	MYSQL_RES *res;
	MYSQL_ROW row;
	Data_pac data_pac;	

	//char src_path[256] = {0}, abs_path[256] = {0}, relative_path[256] = {0};
	//int dir_file_id;

	strcpy(dirname, cmd_content);
	strcpy(src_str, cmd_content);
	get_str_md5(md5_buf, src_str);
	bzero(query, sizeof(query));
	sprintf(query, "%s%d", "select user_name,user_dir_file_id,user_dir from User where user_id=", user_id);
	db_select(conn, query, &res, &ret);
	db_num_rows = -11;
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows > 0)
	{
		row = mysql_fetch_row(res);
		pre_file_id = atoi(row[1]);
		strcpy(log_username, row[0]);
		strcpy(log_dir, row[2]);

		bzero(query, sizeof(query));
		sprintf(query, "%s(%d,'%s','%s','%s',%d,'%s')", "insert into File(pre_file_id, filename, type, owner, owner_id, md5) values", pre_file_id, dirname, "d", row[0], user_id, md5_buf);
		mysql_free_result(res); //避免内存泄漏

		db_insert(conn, query, &ret);
		if(ret == 0)
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1122;
			strcpy(data_pac.buf, "mkdir:成功\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6);
				
			syslog(LOG_INFO|LOG_USER, "username=%s \"mkdir %s\" 成功, 当前路径=%s\n", log_username, cmd_content, log_dir);
		}
		else
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1123;
			strcpy(data_pac.buf, "mkdir:失败\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6);
			
			syslog(LOG_INFO|LOG_USER, "username=%s \"mkdir %s\" 失败, 当前路径=%s\n", log_username, cmd_content, log_dir);
		}
	}
}

bool is_empty(char *cmd_content) //判断命令内容是否为空
{
	if(strlen(cmd_content) == 0)
	{
		return true;
	}
	else if(strlen(cmd_content) > 0)
	{
		return false;
	}
}

bool is_file_exist(MYSQL *conn, int user_id, char *filename, int cur_dir_file_id) //判断当前目录下目标文件是否存在
{
	int ret;
	int db_num_rows;
	char query[600];
	MYSQL_RES *res;
	MYSQL_ROW row;

	db_num_rows = -11;
	bzero(query, sizeof(query));
	sprintf(query, "%s%d%s'%s'%s%d", "select md5 from File where pre_file_id=", cur_dir_file_id, " and filename=", filename, " and owner_id=", user_id);
	db_select(conn, query, &res, &ret);
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows == 0) //当前目录下没有此文件
	{
		return false;
	}
	else if(db_num_rows > 0) //当前目录下有此文件
	{
		return true;
	}
}

bool is_dir_exist(MYSQL *conn, int user_id, char *dirname, int pre_file_id)//判断目录是否存在
{
	char query[600];
	int ret, db_num_rows;
	MYSQL_RES *res;
	MYSQL_ROW row;

	bzero(query, sizeof(query));
	strcpy(query, "select file_id,type from File where pre_file_id=");
	sprintf(query, "%s%d%s'%s'%s%d", query, pre_file_id, " and filename=", dirname, " and owner_id=", user_id);
	db_select(conn, query, &res, &ret);
	db_num_rows = 0;
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows == 0) //没有那个目录
	{
		mysql_free_result(res); //避免内存泄漏
		return false; //没有那个目录
	}
	else
	{
		mysql_free_result(res); //避免内存泄漏
		return true; //有那个目录
	}
}

int get_path(MYSQL *conn, int sfd, int user_id, char *src_path, char *abs_path, char *relative_path, int *dir_file_id) //可以获取绝对路径，相对路径，最终目的目录的file_id
{
	char *dir_buf[256]; //最多256层目录
	char file_path[256], query[600];
	char tmp_path[256];
	int i, j, finish_flag;
	int ret, num_of_dir, db_num_rows;
	int pre_file_id;
	Data_pac data_pac;
	MYSQL_RES *res;
	MYSQL_ROW row;

	bzero(file_path, sizeof(file_path));
	bzero(tmp_path, sizeof(tmp_path));
	strcpy(file_path, src_path); //用户命令中指定的路径
	finish_flag = 0;
	
	//printf("file_path=%s\n", file_path); 
	///////////////路径判断//////////////////////
	///找出绝对路径和相对路径
	if(file_path[0] != '/') //相对路径，从当前目录下开始
	{
		if(strlen(file_path)>2 && file_path[0] == '.' && file_path[1] == '.' && file_path[2] != '\0')
		{
			printf("sadasd\n");
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1003;
			strcpy(data_pac.buf, "路径错误\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6);
			return -1; //路径错误
		}

		if(strcmp(file_path, "..") == 0) //上一级目录
		{
			int cur_file_id;
			bzero(query, sizeof(query));
			sprintf(query, "%s%d", "select user_dir,user_dir_file_id from User where user_id=", user_id);
			db_select(conn, query, &res, &ret);
			db_num_rows = mysql_num_rows(res);
			if(db_num_rows > 0)
			{
				row = mysql_fetch_row(res);
				bzero(file_path, sizeof(file_path));
				strcpy(file_path, row[0]);
				cur_file_id = atoi(row[1]);
			}
			mysql_free_result(res); //避免内存泄漏
			bzero(query, sizeof(query));
			sprintf(query, "%s%d", "select pre_file_id from File where file_id=", cur_file_id);
			db_select(conn, query, &res, &ret);
			db_num_rows = mysql_num_rows(res);
			if(db_num_rows > 0)
			{
				row = mysql_fetch_row(res);
				pre_file_id = atoi(row[0]);
				if(pre_file_id != 0) //表示不在用户根目录
				{
					i = strlen(file_path);
					while(i > 1 && file_path[i] != '/') //把当前目录截去
					{
						i--;
						if(file_path[i] == '/')
						{
							file_path[i] = '\0';
							break;
						}
					}
					mysql_free_result(res); //避免内存泄漏
					finish_flag = 1; //已经完成, 找到了pre_file_id
					//printf("2.file_path=%s\n", file_path);
				}
				else //已经在用户的根目录，不能再进入上一级目录
				{
					bzero(&data_pac, sizeof(Data_pac));
					data_pac.state = 1003;
					strcpy(data_pac.buf, "当前目录为根目录，不能再进入上一级目录\n");
					data_pac.len = strlen(data_pac.buf);
					sendn(sfd, (char *)&data_pac, data_pac.len + 6);
					mysql_free_result(res); //避免内存泄漏
					return -1; //当前目录为根目录，不能再进入上一级目录
				}
			}
		}
		else if(file_path[0] == '.') //当前目录
		{
			i = j =0;
			while(file_path[i] != '/' && file_path[i] != '\0')
			{
				i++;
			}
			if(file_path[i] != '\0')
			{
				i++;
			}
			while(file_path[i] != '\0')
			{
				file_path[j++] = file_path[i++];
			}
			file_path[j] = '\0';
			
			sprintf(tmp_path, "/%s", file_path);
			bzero(query, sizeof(query));
			sprintf(query, "%s%d", "select user_dir,user_dir_file_id from User where user_id=", user_id);
			db_select(conn, query, &res, &ret);
			db_num_rows = mysql_num_rows(res);
			if(db_num_rows > 0)
			{
				row = mysql_fetch_row(res);
				bzero(file_path, sizeof(file_path));
				sprintf(file_path, "%s%s", row[0], tmp_path);
				pre_file_id = atoi(row[1]);
			}
			mysql_free_result(res); //避免内存泄漏
		}
		else //也是当前目录
		{
			sprintf(tmp_path, "/%s", file_path);
			bzero(query, sizeof(query));
			sprintf(query, "%s%d", "select user_dir,user_dir_file_id from User where user_id=", user_id);
			db_select(conn, query, &res, &ret);
			db_num_rows = mysql_num_rows(res);
			if(db_num_rows > 0)
			{
				row = mysql_fetch_row(res);
				bzero(file_path, sizeof(file_path));
				sprintf(file_path, "%s%s", row[0], tmp_path);
				pre_file_id = atoi(row[1]);
			}
			mysql_free_result(res); //避免内存泄漏
		}
	}
	else //绝对路径
	{
		if(strcmp(file_path, "/") == 0)
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1003;
			strcpy(data_pac.buf, "路径错误\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6);
			return -1; //路径错误
		}
		pre_file_id = 0;
		strcpy(tmp_path, file_path);
	}
	strcpy(abs_path, file_path); //获得绝对路径
	strcpy(relative_path, tmp_path); //获得相对路径

	///////////////////////找dir_file_id//////////////////////////////////
	if(finish_flag == 0)
	{
		get_dir(tmp_path, dir_buf, &num_of_dir, 256); //获取各级目录名
		if(num_of_dir == -1) //超出目录最大深度
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1003;
			strcpy(data_pac.buf, "超出目录最大深度\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //通知客户端出错
			return -1; //超出目录最大深度
		}
		for(i = 0;i < num_of_dir;i++)
		{
			pre_file_id = get_dir_id(conn, sfd, user_id, dir_buf[i], pre_file_id);
			if(pre_file_id == -1) //出错
			{
				return -1;
			}
		}
	}
	*dir_file_id = pre_file_id; //要找的目录file_id
	return 0;
}

int get_dir_id(MYSQL *conn, int sfd, int user_id, char *dirname, int pre_file_id)//获取目录的file_id
{
	char query[600];
	int ret, db_num_rows;
	MYSQL_RES *res;
	MYSQL_ROW row;
	Data_pac data_pac;

	bzero(query, sizeof(query));
	strcpy(query, "select file_id,type from File where pre_file_id=");
	sprintf(query, "%s%d%s'%s'%s%d", query, pre_file_id, " and filename=", dirname, " and owner_id=", user_id);
	db_select(conn, query, &res, &ret);
	db_num_rows = 0;
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows == 0) //没有那个目录
	{
		bzero(&data_pac, sizeof(Data_pac));
		data_pac.state = 1004;
		sprintf(data_pac.buf, "%s:%s",  dirname, "没有那个目录\n");
		data_pac.len = strlen(data_pac.buf);
		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //通知客户端出错
		mysql_free_result(res); //避免内存泄漏
		return -1; //没有那个目录
	}
	else if(db_num_rows > 0)
	{
		row = mysql_fetch_row(res);
		if(strcmp(row[1], "f") == 0) //是文件
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 1005;
			sprintf(data_pac.buf, "%s:%s", dirname, "不是目录\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //通知客户端出错
			mysql_free_result(res); //避免内存泄漏
			return -1; //不是目录
		}
		else //是目录
		{
			pre_file_id = atoi(row[0]); //最终为了获得当前目录的file_id
			return pre_file_id;
		}
	}
	mysql_free_result(res); //避免内存泄漏
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

void split_path(char *path, char *filename) //将包含普通文件的路径拆分成目录路径和文件名
{
	int len;
	char *right;
	len = strlen(path);
	right = path + len;
	while(*right != '/' && len > 0)
	{
		right--;
		len--;
	}
	if(*right == '/' && len > 0)
	{
		*right = '\0';
		right++;
		strcpy(filename, right);
	}
	if(len == 0)
	{
		strcpy(filename, right);
	}
}

void get_dir(char *file_path, char *dir_buf[], int *num_of_dir, int max_num_dir) //获取各级目录
{
	int i;
	char *left, *right, tmpc;

	i = *num_of_dir = 0;
	right = file_path;
	while(*right != '/' && *right != '\0') //找到根目录
	{
		right++;
	}
	right++;
	left = right;
	while(*right != '\0' && i < max_num_dir)
	{
		while(*right != '/' && *right != '\0')
		{
			right++;
		}
		if(*right == '/')
		{
			tmpc = *right;
			*right = '\0';
			dir_buf[i] = (char *)calloc(strlen(left), sizeof(char));
			strcpy(dir_buf[i++], left); //将目录名拷贝到dir_buf[]中
			*right = tmpc;
			right++;
			left = right;
		}
		else if(*right == '\0')
		{
			dir_buf[i] = (char *)calloc(strlen(left), sizeof(char));
			strcpy(dir_buf[i++], left); //将最后一个目录名拷贝到dir_buf[]中
		}
	}
	if(i == max_num_dir && *right != '\0')
	{
		*num_of_dir = -1; //表示超出最大目录深度
	}
	else if(i < max_num_dir)
	{
		*num_of_dir = i;
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
