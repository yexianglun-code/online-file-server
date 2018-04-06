#include "../include/queue.h"

//void que_init(pque_t pq, int capacity)	//初始化队列
//{
//	node_t *pnode;
//	int sfd;
//	pq->que_size = 0;
//	pq->que_head = pq->que_tail = NULL;
//	while (scanf("%d", &sfd) != EOF)
//	{
//		pnode = (node_t *)calloc(1, sizeof(node_t));
//		pnode->nd_sockfd = sfd;
//		insert_node_tail(&pq->que_head, &pq->que_tail, pnode);
//		pq->que_size++;
//	}
//}

void que_set(pque_t pq, pnode_t pnew)	//入队
{
	if(pq->que_head == NULL)
	{
		pq->que_head = pnew;
		pq->que_tail = pnew;
		pq->que_size++;
	}
	else
	{
		pq->que_tail->nd_next = pnew;
		pq->que_tail = pnew;
		pq->que_size++;
	}
}

void que_get(pque_t pq, pnode_t *p)	//出队
{
	if(pq->que_head == NULL)
	{
		printf("队列为空\n");
		return;
	}
	*p = pq->que_head;	//获取要出队的队首结点
	pq->que_size--;
	if(pq->que_head == pq->que_tail)	//队列中仅剩一个元素
	{
		pq->que_head = pq->que_tail = NULL;
		return;
	}
	pq->que_head = pq->que_head->nd_next;	//让下一个结点成为队首
}


void que_destroy(pque_t pq)	//销毁队列
{
	if(pq == NULL)
	{
		printf("队列为空\n");
		return;
	}
	pnode_t pre, p;
	pre = pq->que_head;
	if(NULL != pre)
	{
		p = pre->nd_next;
	}
	while(pre != NULL)
	{
		free(pre);
		pre = p;
		if(p != NULL)
			p = p->nd_next;
	}
	free(pq);
}

int que_full(pque_t pq)
{
	return 0;
}

int que_empty(pque_t pq)	//清空队列
{
	if(pq->que_size == 0 || pq->que_head == NULL)
	{
		printf("队列为空\n");
		return -1;
	}
	pnode_t pre, p;
	pre = pq->que_head;
	p = pre->nd_next;
	while(pre != NULL)
	{
		free(pre);
		pre = p;
		if(p != NULL)
			p = p->nd_next;
	}
	pq->que_size = 0;
	pq->que_head = pq->que_tail = NULL;
	return 0;
}

///////////////////////////////////////////////////////////////////////
void insert_node_tail(pnode_t *que_head, pnode_t *que_tail, node_t *pnode)	//尾插法插入结点pnode到队列中
{
	if (*que_head == NULL)		//若链表为空
	{
		*que_head = *que_tail = pnode;
	}
	else
	{
		(*que_tail)->nd_next = pnode;
		*que_tail = pnode;
	}
}

void enqueue(pque_t pq, pnode_t pnew)	//入队操作
{
	if(pq->que_head == NULL)
	{
		pq->que_head = pnew;
		pq->que_tail = pnew;
		pq->que_size++;
	}
	else
	{
		pq->que_tail->nd_next = pnew;
		pq->que_tail = pnew;
		pq->que_size++;
	}
	return;
}

void dequeue(pque_t pq, pnode_t *p)	//出队操作
{
	if(pq->que_head == NULL)
	{
		printf("队列为空\n");
		return;
	}
	*p = pq->que_head;	//获取要出队的队首结点
	pq->que_size--;
	if(pq->que_head == pq->que_tail)	//队列中仅剩一个元素
	{
		pq->que_head = pq->que_tail = NULL;
		return;
	}
	pq->que_head = pq->que_head->nd_next;	//让下一个结点成为队首
}

void print_queue(pque_t pq)	//打印队列pq
{
	node_t *p;
	p = pq->que_head;
	if(p == NULL)
	{
		printf("队列为空\n");
		return;
	}
	printf("pq->que_capacity = %d\n", pq->que_capacity);
	printf("pq->que_size = %d\n", pq->que_size);
	printf("nd_sockfd: ");
	while (p != NULL)
	{
		if (p->nd_next != NULL)
		{
			printf("%d, ", p->nd_sockfd);
		}
		else
		{
			printf("%d", p->nd_sockfd);
		}
		p = p->nd_next;
	}
	printf("\n");
}

/////////////////////////////////////////////////////////////////////////
//bool is_queue_empty(pque_t pq)
//{
//	if(pq->head == NULL)
//	{
//		printf("队列为空!\n");
//		return true;
//	}
//	else
//	{
//		return false;
//	}
//}
//
//void find_node_by_data(pque_t pq, DataType data, node_t **pret)	//根据数据元素查找目标结点,并用结点pret返回所求
//{
//	if(is_queue_empty(pq))
//	{
//		return;
//	}
//	if (data == pq->tail->data)	//所要找的结点为尾指针
//	{
//		*pret = pq->tail;
//		return;
//	}
//	node_t *p;
//	p = pq->head;
//	while (p != NULL)
//	{
//		if (p->data == data)
//		{
//			*pret = p;
//			break;
//		}
//		p = p->next;
//	}
//	if (p == NULL)
//	{
//		printf("没有找到数据为%d的结点\n", data);
//		*pret = NULL;
//	}
//}
//
//void find_node_by_pos(pque_t pq, int n, node_t **pret)	//寻找第n个结点,并用结点pret返回所求
//{
//	if(is_queue_empty(pq))
//	{
//		return;
//	}
//	*pret = pq->head;
//	if (n <= pq->length)
//	{
//		for (int i = 1; i < n; i++)
//		{
//			*pret = (*pret)->next;
//		}
//	}
//	else
//	{
//		printf("超过寻找范围\n");
//	}
//}
//
//void locate_pos_by_data(pque_t pq, DataType data, int *pos)	//找出data在队列pq中首次出现的位置
//{
//	if(is_queue_empty(pq))
//	{
//		return;
//	}
//	node_t *p = pq->head;
//	for (*pos = 1; *pos <= pq->length;(*pos)++)
//	{
//		if (p->data == data)
//		{
//			return;
//		}
//		p = p->next;
//	}
//	*pos = -1;	//队列中不存在数据为data的结点
//}
//
//void delete_node_by_data(pque_t pq, DataType data) //删除结点数据为data的结点
//{
//	if(is_queue_empty(pq))
//	{	
//		return;
//	}
//	node_t *p, *tmp, *pre;
//	while(data == pq->head->data)	//若要删除队首
//	{
//		tmp = pq->head;
//		pq->head = pq->head->next;
//		pq->length--;
//		if(pq->length == 0)
//		{
//			pq->head = NULL;
//			pq->tail = NULL;
//			free(tmp);
//			return;
//		}
//		free(tmp);
//	}
//	pre = pq->head;
//	p = p->next;
//	while(p != NULL)
//	{
//		if(p->data == data)
//		{
//			pre->next = p->next;
//			pq->length--;
//			if(p == pq->tail)	//若删除的是队尾
//			{	
//				pq->tail = pre;
//			}
//			if(pq->length == 0)
//			{
//				pq->head = NULL;
//				pq->tail = NULL;
//				free(p);
//				return;
//			}
//			free(p);
//			p = pre->next;
//		}
//		else
//		{
//			pre = p;
//			p = p->next;
//		}
//	}
//}
//
//void delete_node_by_pos(pque_t pq, int pos, node_t *pret) //删除位置为pos的结点
//{
//	if(is_queue_empty(pq))
//	{
//		return;
//	}
//	node_t *p, *pre;
//	p = pq->head;
//	pre = p;
//	if(pos <= pq->length)
//	{
//		for(int i=1;i<pos;i++)
//		{
//			pre = p;
//			p = p->next;
//		}
//		pre->next = p->next;
//		pq->length--;
//		if(p == pq->tail)	//若删除的是队尾
//		{	
//			pq->tail = pre;
//		}
//		pret->data = p->data;
//		if(pq->length == 0)
//		{
//			pq->head = NULL;
//			pq->tail = NULL;
//			free(p);
//			return;
//		}
//		free(p);
//	}
//	else
//	{
//		printf("超出范围\n");
//	}
//}
//
//void reverse_queue(pque_t pq)	//将一个队列逆置
//{
//	if(is_queue_empty(pq))
//	{
//		return;
//	}
//	node_t *p, *q, *temp;
//	temp = p = pq->tail = pq->head;
//	q = NULL;
//	while (temp != NULL)
//	{
//		temp = temp->next;
//		p->next = q;
//		q = p;
//		p = temp;
//	}
//	pq->head = q;
//}
