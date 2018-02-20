#include "../include/factory.h"

void factory_init(pfac pfactory, int num_of_thread, int capacity, pfunc thread_func)	//线程池初始化
{
	pque_t q = &pfactory->que;

	q->que_capacity = capacity;
	pfactory->thread_num = num_of_thread;
	pfactory->pthread_func = thread_func;
	pfactory->pthid = (pthread_t *)calloc(num_of_thread, sizeof(pthread_t));
	pthread_cond_init(&pfactory->cond, NULL);
	pthread_mutex_init(&q->que_mutex, NULL);
}

void factory_start(pfac pfactory)
{
	int i;
	if(pfactory->flag == 0)
	{
		for(i=0;i<pfactory->thread_num;i++)
		{
			pthread_create(pfactory->pthid+i, NULL, pfactory->pthread_func, pfactory);
		}
		pfactory->flag = 1;	//表示启动线程池
	}
	else
	{
		printf("线程池已经启动\n");
	}
}

void factory_stop(pfac pfactory)
{
	if(pfactory->flag == 1)
	{
		pfactory->flag = 0;	//停止线程池
	}
}
