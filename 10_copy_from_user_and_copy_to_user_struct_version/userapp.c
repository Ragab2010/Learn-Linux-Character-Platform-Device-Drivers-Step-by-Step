#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "mystruct.h"

#define DEVICE_FILE	"/dev/mystruct"

int main()
{
	int fd;
	int retval;
	abc user_struct = {5, 'a'};

	printf("Opening File:%s\n", DEVICE_FILE);
	fd = open(DEVICE_FILE, O_RDWR);

	if (fd < 0) {
		perror("Open Failed");
		exit(1);
	}

	getchar();


	retval = write(fd, &user_struct, sizeof(user_struct));
	printf("Write retval:%d\n", retval);
	getchar();

	user_struct.i = 4;
	user_struct.c = 'b';

	retval = read(fd, &user_struct, sizeof(user_struct));
	printf("Read retval:%d\t int:%d\tchar:%c\n", retval, user_struct.i, user_struct.c);
	getchar();
	
	printf("Closing File\n");
	close(fd);
	getchar();

	return 0;
}
