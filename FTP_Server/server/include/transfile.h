#ifndef __TRANSFILE_H__
#define __TRANSFILE_H__
#include "head.h"

int transfile(int newfd, int fd);	//传送文件给客户端
int sendn(int newfd, char *buf, int len);	//发送字符
int recvn(int newfd, char *buf, int len);	//接收字符

#endif
