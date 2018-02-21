#ifndef __FUNCTION_H__
#define __FUNCTION_H__
#include "head.h"
#include "transfile.h"

int login(int sfd, char *user_name); //账户登陆验证
int signup(int sfd); //用户注册
int check_user_signup_name(char *user_name); //检查注册用户名
int command(int sfd, char *user_name); //客户端命令接口界面
void get_cmd(char *cmd_str, char *cmd, char *cmd_content); //剥离命令字符串，命令存入cmd，命令内容存入cmd_content
void check_path(char *path, int *ret); //检查路径是否合法
void check_filename(char *path, int *ret); //检查文件名是否合法


void user_help(); //用户帮助函数

#endif
