#ifndef __DATABASE_H__
#define __DATABASE_H__
#include "head.h"

void db_conn(MYSQL **conn);
void db_select(MYSQL *conn, char *query, MYSQL_RES **result, int *ret);
void db_insert(MYSQL *conn, char *query, int *ret);
void db_delete(MYSQL *conn, char *query, int *ret);
void db_update(MYSQL *conn, char *query, int *ret);
int db_get_user_info_by_name(MYSQL *conn, char *user_name, char *dst, MYSQL_RES **res); //根据用户名，从数据库User表获取指定dst内容
int db_get_user_info_by_id(MYSQL *conn, int user_id, char *dst, MYSQL_RES **res); //根据用户id,从数据库User表获取指定dst内容
int db_get_file_info_by_filename(MYSQL *conn, char *filename, char *dst, MYSQL_RES **res); //根据文件名，从数据库File表获取指定dst的信息

void get_str_random(char *buf, int n); //用于产生n位随机字符串
int get_str_md5(char *md5_str,unsigned char *src_str);	//由字符串src_str生成32位MD5码字符串，存入md5_str
int get_file_md5(const char *file_path, char *md5_str); //由路径file_path中的目标文件的内容生成32位MD5码字符串，存入md5_str

void db_produce_test_data(MYSQL *conn, int index);	//生成测试数据，分别存入User表和File表

#endif
