#include <stdio.h>
#include <sys/fcntl.h>

int main()
{
	int fd;

	fd = open("/dev/mydevice", O_RDONLY);
	perror("fd");
	return 0;
}
