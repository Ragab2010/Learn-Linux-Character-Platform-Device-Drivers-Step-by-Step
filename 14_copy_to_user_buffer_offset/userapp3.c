#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define DEVICE_FILE	"/dev/msg"

int main()
{
	int fd;
	int retval;
	char buffer[10];
	int i;

	printf("Opening File:%s\n", DEVICE_FILE);
	fd = open(DEVICE_FILE, O_RDWR);

	if (fd < 0) {
		perror("Open Failed");
		exit(1);
	}

	getchar();

	retval = read(fd, buffer, 5);
	buffer[retval] = '\0';
	printf("read retval:%d\n", retval);
	printf("read buffer:%s\n", buffer);
	getchar();

	retval = read(fd, buffer, 5);
	buffer[retval] = '\0';
	printf("read retval:%d\n", retval);
	printf("read buffer:%s\n", buffer);
	getchar();
	
	printf("Closing File\n");
	close(fd);
	getchar();

	return 0;
}
