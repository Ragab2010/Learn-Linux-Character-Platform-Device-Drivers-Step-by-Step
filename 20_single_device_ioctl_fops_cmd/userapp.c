#include <stdio.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[])
{
    char buffer[1024];	
	int fd;
	unsigned int length;
	unsigned char ch = 'A';
	int i = 0;

	fd = open("/dev/mydevice", O_RDWR);
	if (fd < 0) {
		perror("fd failed");
		exit(2);
	}
	//Get Length  - 0x01
	ioctl(fd, 0x01, &length);
	printf("Length:%u\n", length);
	//Set Character - 0x03
	ioctl(fd, 0x03, &ch);
	perror("ioctl");
	lseek(fd, 0, SEEK_SET);
	perror("lseek");
	length = read(fd, buffer, 1024);
	perror("Read");
	printf("length:%d\n", length);
	buffer[1023] = '\0';
	printf("Buffer:%s\n", buffer);
	close(fd);
}
