#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define DEV_NAME	"/dev/chrDevDemo"

int main(int argc, char *argv[])
{
	int ret, fd;
	char buf[256] = {0};

	fd = open(DEV_NAME, O_RDWR);
	if (0 > fd) {
		printf("open dev fail: %s\r\n", DEV_NAME);
		return -1;
	} else {
		printf("open dev success: %s\r\n", DEV_NAME);
	}

	//1 读一下当前数据	
	ret = read(fd, buf, sizeof(buf));
	printf("1 read ret:%d, buf:[%s]\r\n", ret, buf);

	//2 写入自己的数据
	snprintf(buf, sizeof(buf), "Hello Driver %s", __TIME__);
	ret = write(fd, buf, strlen(buf));
	printf("2 write ret:%d, len:%ld, data:%s\r\n", ret, strlen(buf), buf);	
	
	//3 再次读数据，应该是步骤2写入的数据
	memset(buf, 0x0, sizeof(buf));
	ret = read(fd, buf, sizeof(buf));
	printf("3 read ret:%d, buf:[%s]\r\n", ret, buf);

	printf("4 printf ioctl\n");
	ret = ioctl(fd, 0);
	if (ret != 0) {
		printf("ioctl error!\n");
	}

	close(fd);
	return 0;
}
