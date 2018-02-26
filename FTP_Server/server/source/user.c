#include "../include/user.h"

int get_user_info(MYSQL *conn, char *user_name, MYSQL_RES **res)
{
	char dst[3] = "*";
	int ret;
	ret = db_get_user_info_by_name(conn, user_name, dst, res);
	if(ret == 0)
	{
		return 0;
	}
	else
	{
		return -1;	
	}
}

int user_verify(MYSQL *conn, int sfd, int *user_id)
{
	char salt[12] = { 0 }, ciphertext[128] = { 0 };
	char log_username[64]={0};
	int ret, db_num_rows;
	MYSQL_RES *res;
	MYSQL_ROW row;
	Data_pac data_pac;

	res = NULL;
	bzero(&data_pac, sizeof(Data_pac));
	recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
	recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
	recvn(sfd, data_pac.buf, data_pac.len);	//接收用户名

	strcpy(log_username, data_pac.buf);
	if(data_pac.state == 54)
	{
		return -1; //用户要求注册
	}
	else if(data_pac.state == 55)
	{
		return -2; //用户不想注册
	}

	/////用户认证
	ret = get_user_info(conn, data_pac.buf, &res);//获取用户信息
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows > 0)
	{
		bzero(&data_pac, sizeof(Data_pac));
		data_pac.state = 2;	//表示用户名存在
		data_pac.len = strlen(data_pac.buf);
		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送用户存在消息
		row = mysql_fetch_row(res);
		*user_id = atoi(row[0]); //获取用户id
		strcpy(salt, row[2]);	//获取盐值
		strcpy(ciphertext, row[3]);	//获取密文
		bzero(&data_pac, sizeof(Data_pac));
		data_pac.len = strlen(salt);
		strcpy(data_pac.buf, salt);
		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送盐值给客户端
		bzero(&data_pac, sizeof(Data_pac));
		recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
		recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
		recvn(sfd, data_pac.buf, data_pac.len);	//接收客户端密码密文
		if(strcmp(data_pac.buf, ciphertext) == 0)
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 3; //表示验证成功
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6);	//发送登陆成功消息
			mysql_free_result(res);

			syslog(LOG_INFO|LOG_USER, "username=%s login successfully\n", log_username);

			return 1;	//表示验证成功
		}
		else
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 4;	//表示用户名或密码不错误
			strcpy(data_pac.buf, "用户名或密码错误\n");
			data_pac.len = strlen(data_pac.buf);
			sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送错误提示
			mysql_free_result(res);
			
			syslog(LOG_ERR|LOG_USER, "username=%s 登陆失败,用户名或密码错误\n", log_username);
			
			return 0; //表示用户名或密码错误
		}
	}
	else
	{
		mysql_free_result(res);
		bzero(&data_pac, sizeof(Data_pac));
		data_pac.state = 1;	//表示用户名不存在
		strcpy(data_pac.buf, "用户名不存在\n");
		data_pac.len = strlen(data_pac.buf);
		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送用户名不存在消息
			
		syslog(LOG_ERR|LOG_USER, "username=%s 登陆失败,用户名不存在\n", log_username);
		return -1; //表示查询不到该用户，验证失败
	}
}

int user_signup(MYSQL *conn, int sfd)
{
	Data_pac data_pac;

	bzero(&data_pac, sizeof(Data_pac));
	data_pac.state = 6; //表示允许用户注册
	sendn(sfd, (char *)&data_pac, data_pac.len + 6);

	char user_name[64] = {0}, buf[9] = {0};
	char salt[12] = "$6$";
	char *ciphertext;
	unsigned char *filename;
	char user_dir[256] = {0}, type[2] = "d", dst[10] = "user_id";
	char md5_buf[33] = { 0 };
	int pre_file_id = 0;
	int db_num_rows, owner_id;
	int ret, OK;
	char query[600];
	MYSQL_RES *res;
	MYSQL_ROW row;

	//////用户名查重
	OK = 0;
	while(OK != 1)
	{
		bzero(&data_pac, sizeof(Data_pac));
		recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
		recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
		recvn(sfd, data_pac.buf, data_pac.len); //接收客户端返回用户名

		strcpy(user_name, data_pac.buf); //用户名
		bzero(query, sizeof(query));
		sprintf(query, "%s'%s'", "select user_id from User where user_name=", user_name);
		db_select(conn, query, &res, &ret);
		db_num_rows = mysql_num_rows(res);
		if(db_num_rows > 0) //重名
		{
			bzero(&data_pac, sizeof(Data_pac));
			data_pac.state = 8; //表示用户注册名重名
			sendn(sfd, (char *)&data_pac.len, data_pac.len + 6);
			
			syslog(LOG_INFO|LOG_USER, "username=%s 注册失败，重名\n", user_name);
		}
		else if(db_num_rows == 0)
		{
			OK = 1;
		}
		mysql_free_result(res); //避免内存泄漏
	}

	if(OK == 1) //没有重名
	{
		filename = (unsigned char *)calloc(data_pac.len, sizeof(unsigned char));
		strcpy(filename, user_name); //用户根目录名字

		bzero(&data_pac, sizeof(Data_pac));
		data_pac.state = 9; //表示用户注册名未重名
		sendn(sfd, (char *)&data_pac.len, data_pac.len + 6);

		get_str_random(buf, 8);	//获取盐值后半部分（8位)
		sprintf(salt, "%s%s", salt, buf);	//盐值

		bzero(&data_pac, sizeof(Data_pac));
		data_pac.len = strlen(salt);
		strcpy(data_pac.buf, salt);
		sendn(sfd, (char *)&data_pac, data_pac.len + 6); //发送盐值给客户端

		bzero(&data_pac, sizeof(Data_pac));
		recvn(sfd, (char *)&data_pac.len, sizeof(data_pac.len));
		recvn(sfd, (char *)&data_pac.state, sizeof(data_pac.state));
		recvn(sfd, data_pac.buf, data_pac.len); //接收客户端返回密码密文

		ciphertext = (char *)calloc(data_pac.len, sizeof(char));
		strcpy(ciphertext, data_pac.buf); //用户密码密文

		bzero(query, sizeof(query));
		sprintf(user_dir, "/%s", user_name); //用户根目录
		sprintf(query, "%s('%s','%s','%s','%s')", "insert into User(user_name, salt, ciphertext, user_dir) values", user_name, salt, ciphertext, user_dir);
		db_insert(conn, query, &ret);	//插入用户数据到数据库

		get_str_md5(md5_buf, filename); //获取目录文件名的md5值
		ret = db_get_user_info_by_name(conn, user_name, dst, &res);
		db_num_rows = mysql_num_rows(res);
		if(db_num_rows > 0)
		{
			row = mysql_fetch_row(res);
			owner_id = atoi(row[0]); //获取刚创建用户的user_id
			mysql_free_result(res); //避免内存泄漏
			bzero(query, sizeof(query));
			strcpy(query, "insert into File(pre_file_id, filename, type, owner, owner_id, md5) values");
			sprintf(query, "%s(%d,'%s','%s','%s',%d,'%s')", query, pre_file_id, filename, type, user_name, owner_id, md5_buf);
			db_insert(conn, query, &ret); //插入用户的根目录文件信息到数据库
		}

		bzero(dst, sizeof(dst));
		strcpy(dst,"file_id");
		ret = db_get_file_info_by_filename(conn, filename, dst, &res);
		db_num_rows = 0;
		db_num_rows = mysql_num_rows(res);
		if(db_num_rows > 0)
		{
			row = mysql_fetch_row(res);
			int user_dir_file_id = atoi(row[0]);
			mysql_free_result(res); //避免内存泄漏
			bzero(query, sizeof(query));
			strcpy(query, "update User set user_dir_file_id=");
			sprintf(query, "%s%d%s%d", query, user_dir_file_id, " where user_id=", owner_id);
			db_update(conn, query, &ret); //修改用户的当前目录id
		}
	}

	bzero(&data_pac, sizeof(Data_pac));
	data_pac.state = 10; //表示注册成功
	strcpy(data_pac.buf, "注册成功\n");
	data_pac.len = strlen(data_pac.buf);
	sendn(sfd, (char *)&data_pac, data_pac.len + 6);
	
	syslog(LOG_INFO|LOG_USER, "username=%s 注册成功\n", user_name);

	return 1; //表示注册成功
}
