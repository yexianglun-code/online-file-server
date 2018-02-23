#include "./include/transfile.h"

int transfile(int sfd, int fd, int mode) //传输文件
{
	Data_pac data_pac;
	int ret, upload_file_len;
	struct stat filestat;
	
	upload_file_len = 0;
	bzero(&filestat, sizeof(filestat));
	fstat(fd, &filestat);
	if(1 == mode)
	{
		while(bzero(&data_pac, sizeof(data_pac)), (data_pac.len = read(fd, data_pac.buf, sizeof(data_pac.buf))) > 0)
		{
			upload_file_len += data_pac.len;
			ret = sendn(sfd, (char *)&data_pac, data_pac.len + 6);
			if(-1 == ret)
			{
				return -1; //表示出错
			}
			print_progress_bar(upload_file_len, filestat.st_size);
		}
	}
	else if(2 == mode)
	{
		while(1)
		{
			ret = sendfile(sfd, fd, NULL, FILE_LIMIT);	//零拷贝传送文件
			if(-1 == ret)
			{
				perror("sendfile");
				return -1;
			}
			upload_file_len += ret;
			if(upload_file_len <= filestat.st_size)
			{
				print_progress_bar(upload_file_len, filestat.st_size);
			}
			if(upload_file_len == filestat.st_size)
			{
				break;
			}
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
