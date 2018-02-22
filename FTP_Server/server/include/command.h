#ifndef __COMMAND_H__
#define __COMMAND_H__
#include "head.h"
#include "transfile.h"
#include "database.h"

void command(MYSQL *conn, int sfd, int user_id); //解析客户端发送过来的命令，并执行相应任务
void get_cmd(char *cmd_str, char *cmd, char *cmd_content); //剥离客户端发来的命令字符串，命令存入cmd，命令内容存入cmd_content
void cmd_cd(MYSQL *conn, int sfd, int user_id, char *cmd_content);	//进入指定目录
void cmd_ls(MYSQL *conn, int sfd, int user_id, char *cmd_content);	//展示当前目录下文件信息
void cmd_pwd(MYSQL *conn, int sfd, int user_id, char *cmd_content);	//显示用户当前所在路径
void cmd_puts(MYSQL *conn, int sfd, int user_id, char *cmd_content); //用户上传文件
void cmd_gets(MYSQL *conn, int sfd, int user_id, char *cmd_content); //用户下载文件
void cmd_remove(MYSQL *conn, int sfd, int user_id, char *cmd_content); //用户删除文件
void cmd_mkdir(MYSQL *conn, int sfd, int user_id, char *cmd_content); //用户创建目录


bool is_empty(char *cmd_content); //判断命令内容是否为空
bool is_file_exist(MYSQL *conn, int user_id, char *filename, int cur_dir_file_id); //判断当前目录下是否存在此文件
bool is_dir_exist(MYSQL *conn, int user_id, char *dirname, int pre_file_id); //判断目录是否存在
int get_path(MYSQL *conn, int sfd, int user_id, char *src_path, char *abs_path, char *relative_path, int *dir_file_id); //可以获取绝对路径，相对路径，最终目的目录的file_id
int get_dir_id(MYSQL *conn, int sfd, int user_id, char *dirname, int pre_file_id); //获取目录的file_id
void get_dst_dir(char *src_path, char *dst_path); //获取目标目录
void split_path(char *path, char *filename); //从路径提取文件名
void get_dir(char *file_path, char *dir_buf[], int *num_of_dir, int max_num_dir); //获取各级目录名
void my_lltoa(char *dst, off_t filesize); //将文件大小(off_t)转换成字符串

#endif
