#include "../include/transfile.h"

int transfile(int sfd, int fd, int mode) //传输文件
{
	Data_pac data_pac;
	int ret;
	
	if(1 == mode)
	{
		while(bzero(&data_pac, sizeof(data_pac)), (data_pac.len = read(fd, data_pac.buf, sizeof(data_pac.buf))) > 0)
		{
			ret = sendn(sfd, (char *)&data_pac, data_pac.len + 6);
			if(-1 == ret)
			{
				return -1; //表示出错
			}
		}
	}
	else if(2 == mode)
	{
		struct stat filestat;
		fstat(fd, &filestat);
		ret = sendfile(sfd, fd, NULL, filestat.st_size);	//零拷贝传送文件
		if(-1 == ret)
		{
			perror("sendfile");
			return -1;
		}
	}
	return 1; //表示发送完毕
}

int sendn(int sfd, char buf[], int len)
{
	int ret, total;
	total = 0;
	while(total < len)
	{
		ret = send(sfd, buf + total, len - total, 0);
		if(-1 == ret)
		{
			return -1;
		}
		total = total + ret;
	}
	return 0;
}

int recvn(int sfd, char buf[], int len)
{
	int ret, total;
	total = 0;
	while(total < len)
	{
		ret = recv(sfd, buf + total, len - total, 0);
		if(-1 == ret)
		{
			return -1;
		}
		total = total + ret;
	}
	return 0;
}
