#include "./include/transfile.h"

int transfile(int sfd, char *file_path) //传输文件
{
	//printf("file_path=%s\n", file_path);
	Data_pac data_pac;
	int ret, fd;
	struct stat filestat;
	
	fd = open(file_path, O_RDONLY);
	if(-1 == fd)
	{
		perror("open");
		return -2;
	}
	//fstat(fd, &filestat);
	//sendfile(sfd, fd, NULL, filestat.st_size);	//零拷贝传送文件
	while(bzero(&data_pac, sizeof(data_pac)), (data_pac.len = read(fd, data_pac.buf, sizeof(data_pac.buf))) > 0)
	{
		ret = sendn(sfd, (char *)&data_pac, data_pac.len + 6);
		if(-1 == ret)
		{
			close(fd);
			return -1; //表示出错
		}
	}
	close(fd);
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
