#include <stdio.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <sys/ioctl.h>
#include "ioctl_cmd.h"

int main(int argc, char *argv[])
{
	char buffer[1024];	
	int fd;
	unsigned long length;
	unsigned char ch = 'A';
	int i = 0;

	fd = open("/dev/mydevice", O_RDWR);
	if (fd < 0) {
		perror("fd failed");
		exit(2);
	}


	ioctl(fd, MSG_GET_ADDRESS, NULL);
	perror("ioctl");
	getchar();

	//Get Length  - 0x01
	ioctl(fd, MSG_IOCTL_GET_LENGTH, &length);
	printf("Length:%lu\n", length);
	//Set Character - 0x03
	ioctl(fd, MSG_IOCTL_FILL_BUFFER, &ch);
	perror("ioctl");
	lseek(fd, 0, SEEK_SET);
	perror("lseek");
	length = read(fd, buffer, 1024);
	perror("Read");
	printf("length:%ld\n", length);
	buffer[1023] = '\0';
	printf("Buffer:%s\n", buffer);
	ioctl(fd, 4);
	perror("ioctl");
	close(fd);
}
