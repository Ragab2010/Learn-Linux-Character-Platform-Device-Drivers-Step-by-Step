#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int main()
{
	int fd;

	fd = open("/dev/mydevice", O_RDWR);
	perror("fd");

	ioctl(fd, 0x01, 10);
	perror("ioctl");
	getchar();

	ioctl(fd, 0x01);
	perror("ioctl");
	getchar();
	close(fd);
	return 0;
}
