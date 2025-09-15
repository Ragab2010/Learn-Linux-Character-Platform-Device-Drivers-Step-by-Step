#include <stdio.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char buffer[50];	
	int fd1, fd2;
	int length;
	int i = 0;

	fd1 = open("/dev/mydevice1", O_RDWR);
	if (fd1 < 0) {
		perror("fd1 failed");
		exit(2);
	}

	fd2 = open("/dev/mydevice1", O_RDWR);
	if (fd2 < 0) {
		perror("fd2 failed");
		exit(2);
	}

	printf("write : %ld\n", write(fd1, "hello world", sizeof("hello world")));
	lseek(fd2, 0, SEEK_SET);
	perror("lseek");
	memset(buffer, 0, sizeof(buffer));
	length = read(fd2, buffer, sizeof(buffer));
	buffer[length] = '\0';
	printf("Read:%s\t length:%d\n", buffer, length);
	close(fd1);
	close(fd2);
}
