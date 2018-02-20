#ifndef __HEAD_H__
#define __HEAD_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <shadow.h>
#include <crypt.h>
#include <mysql/mysql.h>
#include <sys/sendfile.h>
#include <openssl/md5.h>

#define MY_FILE_DIR "../CLIENT_RESOURCE/myfile"
#define MY_DOWNLOAD_DIR "../CLIENT_RESOURCE/download"
#define DEBUG

typedef struct
{
	int len;	//buf中实际有效数据长度
	short state;	
	//state是状态码
	//  0 表示状态码不起作用
	//	1--100 用于用户逻辑操作
	//		1--50 一般为服务器所用
	//			1:表示用户名不存在
	//			2:用户名存在
	//			3:登陆验证成功
	//			4:用户名或密码错误
	//			5:未知错误
	//			6:允许用户注册
	//			7:不允许用户注册
	//			8:用户注册名重名
	//			9:用户注册名未重名
	//			10:注册成功
	//			11:注册失败
	//			
	//			....
	//
	//		51--100 一般为客户端所用
	//			51:用户正常退出下线
	//			52:用户强制退出下线
	//			53:用户掉线
	//			54:用户想要注册
	//			55:用户不想注册
	//			....
	//
	//	101--1000 用于命令操作
	//		101:cd
	//		102:ls
	//		103:pwd
	//		104:puts
	//		105:gets
	//		106:remove
	//		107:mkdir
	//		....
	//		
	//		
	//  1001--2000 用于命令操作状态
 	//		1001--1020 cd状态
	//			1001:cd--未知错误
	//			1002:cd--成功
	//			1003:cd--失败
 	//			1004:cd--没有那个目录	
 	//			1005:cd--不是目录	
 	//			....
	//
	//		1021--1040 ls状态
 	//			1021:ls--未知错误
	//			1022:ls--失败	
 	//			1023:ls--无权查看此目录下文件	
 	//			1024:ls--没有那个文件或目录	
 	//			....	
 	//			
 	//		1041--1060 pwd状态	
 	//			1041:pwd--未知错误
 	//			1042:pwd--成功
 	//			1043:pwd--失败
 	//			....
 	//			
 	//		1061--1080 gets状态	
	//			(用于发送端)
 	//			1061:gets--未知错误
 	//			1062:gets--成功
 	//			1063:gets--失败
 	//			1064:gets--可以下载此文件
 	//			1065:gets--没有此文件
 	//			1066:gets--无权下载此文件
 	//			1067:gets--无法下载目录
	//
 	//			1068:gets--准备好可以开始接收文件(用于接收端)
 	//			1069:gets--准备好可以开始发送文件(用于发送端)
 	//			1070:gets--不可以开始接收文件(用于接收端)		
 	//			1071:gets--不可以开始发送文件(用于发送端)
 	//			1072:gets--下载完成(用于接收端)
 	//			1073:gets--下载中止(用于接收端)
	//			1074:gets--发送完毕(用于发送端)			
	//			1075:gets--发送中止(用于发送端)
	//			....
	//
 	//		1081--1100 puts状态
	//			(用于接收方)
 	//			1081:puts--未知错误
 	//			1082:puts--成功
 	//			1083:puts--失败
 	//			1084:puts--无权上传到此目录
 	//			1085:puts--剩余空间不足
	//
 	//			1086:puts--可以开始接收文件(用于接收方)
	//			1087:puts--可以开始发送文件	(用于发送方)
 	//			1088:puts--不可以开始接收文件(用于接收方)
 	//			1089:puts--不可以开始发送文件(用于发送方)
 	//			1090:puts--上传完成(用于发送方)
	//			1091:puts--上传中止(用于发送方)
 	//			....
 	//			
 	//		1101--1120 remove状态	
 	//			1101:remove--未知错误
 	//			1102:remove--成功
	//			1103:remove--失败
	//			1104:remove--无权删除此文件或目录
	//			....	
	//
	//		1121--1140 mkdir状态
	//			1121:mkdir--未知错误
	//			1122:mkdir--成功
	//			1123:mkdir--失败
	//			....
	//
	//
	//
	char buf[4090];	//实际有效数据
}Data_pac;	//收发数据所用的数据包结构体

#endif
