#ifndef __TRANSFILE_H__
#define __TRANSFILE_H__
#include "head.h"
#include "function.h"

int transfile(int newfd, char *file_path);
int sendn(int newfd, char *buf, int len);
int recvn(int newfd, char *buf, int len);

#endif
