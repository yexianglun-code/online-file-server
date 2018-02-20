#include "../include/database.h"

void db_conn(MYSQL **conn)
{
	char * server = "localhost";
	char * user = "root";
	char * password = "Qbs1246";
	char * database = "My_File_Server";

	*conn = mysql_init(NULL);
	if(!mysql_real_connect(*conn, server, user, password, database, 0, NULL, 0))
	{
		printf("Error connecting to databases:%s\n", mysql_error(*conn));
	}
	else
	{
		printf("Connected to database %s...\n", database);
	}
}

void db_select(MYSQL *conn, char *query, MYSQL_RES **result, int *ret)
{
	*ret = mysql_query(conn, query);
	if(*ret)
	{
		printf("Error making select query:%s\n", mysql_error(conn));
	}
	else
	{
		*result = mysql_store_result(conn);
	}
}

void db_insert(MYSQL *conn, char *query, int *ret)
{
	*ret = mysql_query(conn, query);
	if(*ret)
	{
		printf("Error making insert query:%s\n", mysql_error(conn));
	}
	else
	{
		printf("insert success\n");
	}
}

void db_delete(MYSQL *conn, char *query, int *ret)
{
	*ret = mysql_query(conn, query);
	if(*ret)
	{
		printf("Error making delete query:%s\n", mysql_error(conn));
	}
	else
	{
		printf("delete success\n");
	}
}

void db_update(MYSQL *conn, char *query, int *ret)
{
	*ret = mysql_query(conn, query);
	if(*ret)
	{
		printf("Error making update query:%s\n", mysql_error(conn));
	}
	else
	{
		printf("update success\n");
	}
}

int db_get_user_info_by_name(MYSQL *conn, char *user_name, char *dst, MYSQL_RES **res) //根据用户名，从数据库User表获取指定内容
{
	int ret;
	char query[300] = "select ";
	sprintf(query, "%s%s%s'%s'", query, dst, " from User where user_name = ", user_name);
	db_select(conn, query, res, &ret);	//到数据库查询用户信息
	if(ret == 0)
	{
		return 0;
	}
	else
	{
		return -1;	
	}
}

int db_get_user_info_by_id(MYSQL *conn, int user_id, char *dst, MYSQL_RES **res) //根据用户id,从数据库User表获取指定dst内容
{
	int ret;
	char query[300] = "select ";
	sprintf(query, "%s%s%s%d", query, dst, " from User where user_id = ", user_id);
	db_select(conn, query, res, &ret);	//到数据库查询用户信息
	if(ret == 0)
	{
		return 0;
	}
	else
	{
		return -1;	
	}
}

int db_get_file_info_by_filename(MYSQL *conn, char *filename, char *dst, MYSQL_RES **res) //根据文件名，从数据库File表获取指定dst的信息
{
	int ret;
	char query[512] = "select ";
	sprintf(query, "%s%s%s'%s'", query, dst, " from File where filename =", filename);
	db_select(conn, query, res, &ret);
	if(ret == 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}	
}

void get_str_random(char *buf, int n) //用于产生n位随机字符串
{
	char *str = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,.;\"<>?";	//去掉了字符"'","/"
	int i, len;
	len = strlen(str);
	srand((unsigned int)time((time_t*)NULL));
	for(i=0;i<n;i++)
	{
		buf[i] = str[rand()%len];
	}
}

int get_str_md5(char *md5_str,unsigned char *src_str)	//由字符串src_str生成32位MD5码字符串，存入md5_str
{
	unsigned char md5[16];
	char tmp[3] = { 0 };
	MD5(src_str, strlen(src_str), md5);
   	for(int i=0;i < 16;i++)		//目录文件md5值
	{
		sprintf(tmp, "%2.2x", md5[i]);
		strcat(md5_str, tmp);
	}
	return 0;
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

void db_produce_test_data(MYSQL *conn, int index) //生成测试用户和文件
{
	int ret;
	char user_name[7] = "admin";
	char salt[12] = "$6$", buf[9] = { 0 };
	char passwd[4] = "123", *ciphertext;
	char user_dir[256] = "/";
	char query[600] = "insert into User(user_name, salt, ciphertext, user_dir) values";

	unsigned char filename[7] = "admin", type[2] = "d",dst[10] = "user_id";
	char md5_buf[33] = { 0 }; //目录文件md5值
	int pre_file_id = 0; //代表用户根目录
	int db_num_rows, owner_id;
	MYSQL_RES *res;
	MYSQL_ROW row;
	if(index > 0)
	{
		sprintf(user_name, "%s%c", user_name, index+'0');
		sprintf(filename, "%s%c", filename, index+'0');
	}
	get_str_random(buf, 8);	//获取盐值后半部分（8位)
	sprintf(salt, "%s%s", salt, buf);	//盐值
	ciphertext = crypt(passwd, salt);	//密码密文
	sprintf(user_dir, "%s%s", user_dir, user_name);
	sprintf(query, "%s('%s','%s','%s','%s')", query, user_name, salt, ciphertext, user_dir);
	db_insert(conn, query, &ret);	//插入用户数据到数据库

	get_str_md5(md5_buf, filename); //获取目录文件名的md5值
	ret = db_get_user_info_by_name(conn, user_name, dst, &res);
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows > 0)
	{
		row = mysql_fetch_row(res);
		owner_id = atoi(row[0]); //获取刚创建用户的user_id
		bzero(query, sizeof(query));
		strcpy(query, "insert into File(pre_file_id, filename, type, owner, owner_id, md5) values");
		sprintf(query, "%s(%d,'%s','%s','%s',%d,'%s')", query, pre_file_id, filename, type, user_name, owner_id, md5_buf);
		db_insert(conn, query, &ret); //插入用户的根目录文件信息到数据库
	}
	
	bzero(dst, sizeof(dst));
	strcpy(dst,"file_id");
	ret = db_get_file_info_by_filename(conn, filename, dst, &res);
	mysql_free_result(res); //避免内存泄漏
	db_num_rows = 0;
	db_num_rows = mysql_num_rows(res);
	if(db_num_rows > 0)
	{
		row = mysql_fetch_row(res);
		int user_dir_file_id = atoi(row[0]);
		bzero(query, sizeof(query));
		strcpy(query, "update User set user_dir_file_id=");
		sprintf(query, "%s%d%s%d", query, user_dir_file_id, " where user_id=", owner_id);
		db_update(conn, query, &ret); //修改用户的当前目录id
	}
	mysql_free_result(res); //避免内存泄漏
}
