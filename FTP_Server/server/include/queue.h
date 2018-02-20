#ifndef __QUEUE_H__
#define __QUEUE_H__
#include "head.h"

typedef struct tag_node
{
	int nd_sockfd;
	struct tag_node *nd_next;
}node_t, *pnode_t;	//元素结构体，存储实际client fd(即存储任务)

typedef struct tag_que
{
	pnode_t que_head, que_tail;
	int que_capacity;
	int que_size;
	pthread_mutex_t que_mutex;
}que_t, *pque_t;

void que_init(pque_t pq, int capacity);	//初始化队列
void que_set(pque_t pq, pnode_t pnew);	//入队
void que_get(pque_t pq, pnode_t *p);	//出队
void que_destroy(pque_t pq);	//销毁队列
int que_full(pque_t pq);
int que_empty(pque_t pq);	//清空队列


//////////////////////////////////////////////////////////////////////
void insert_node_tail(pnode_t *que_head, pnode_t *que_tail, node_t *pnode);//尾插法插入结点pnode到队列中
void enqueue(pque_t pq, pnode_t pnew);	//入队操作
void dequeue(pque_t pq, pnode_t *p);	//出队操作
void print_queue(pque_t pq);	//打印队列pq

/////////////////////////////////////////////////////////////////////
//bool is_queue_empty(pque_t pq);	//判断队列是否为空
//void find_node_by_data(pque_t pq, DataType data, node_t **pret);	//根据数据元素查找目标结点,并用结点pret返回所求
//void find_node_by_pos(pque_t pq, int n, node_t **pret);	//寻找第n个结点,并用结点pret返回所求
//void locate_pos_by_data(pque_t pq, DataType data, int *pos);	//找出data在队列pq中首次出现的位置
//void delete_node_by_data(pque_t pq, DataType data); //删除结点数据为data的结点
//void delete_node_by_pos(pque_t pq, int pos, node_t *pret); //删除位置为pos的结点
//void reverse_queue(pque_t pq);	//将一个队列逆置

#endif
