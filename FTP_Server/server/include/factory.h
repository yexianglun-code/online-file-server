#ifndef __FACTORY_H__
#define __FACTORY_H__
#include "head.h"
#include "queue.h"
#include "transfile.h"
#include "database.h"
#include "user.h"
#include "command.h"

typedef void *(*pfunc)(void *);	//定义线程函数指针类型别名
typedef struct 
{
	que_t que;	//任务队列
	pthread_cond_t cond;
	pthread_t *pthid;	//线程ID数组
	int flag;	//flag=1,表示线程池启动；flag=0,表示线程池停止
	int thread_num;
	//MYSQL *conn;	//数据库连接句柄
	//pthread_mutex_t db_mutex;
	pfunc pthread_func;	
}factory, *pfac;

void factory_init(pfac pfactory, int num_of_thread, int capacity, pfunc thread_func);	//线程池初始化
void factory_start(pfac pfactory);	//线程池启动
void factory_stop(pfac pfactory);	//停止线程程池
#endif
