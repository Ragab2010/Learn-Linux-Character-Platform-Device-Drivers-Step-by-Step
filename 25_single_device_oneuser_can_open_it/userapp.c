#include <stdio.h>
#include <fcntl.h> 
#include <stdlib.h>

int main(int argc, char *argv[])
{
    char buffer[50];	
	int fd;
	int length;
	int i = 0;

	fd = open("/dev/mydevice", O_RDWR);
	if (fd < 0) {
		perror("fd failed");
		exit(2);
	}
	printf("Open Successful\n");
	write(fd, "hello world", sizeof("hello world"));;	
	getchar();
	close(fd);
}
