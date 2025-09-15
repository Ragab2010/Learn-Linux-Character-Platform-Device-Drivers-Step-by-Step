#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "mystruct.h"

#define DEVICE_FILE	"/dev/mystruct"

int main()
{
	int fd;
	int retval;
	abc user_struct;

	user_struct.str = malloc(sizeof(char)*10);
	strcpy(user_struct.str, "hello");
	user_struct.i = 10;

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

	memset(&user_struct, 0, sizeof(user_struct));

	user_struct.str = malloc(sizeof(char)*10);
	retval = read(fd, &user_struct, sizeof(user_struct));
	printf("Write retval:%d\n", retval);
	printf("Int:%d\t Str:%s\n", user_struct.i, user_struct.str);
	getchar();
	
	printf("Closing File\n");
	close(fd);
	getchar();

	return 0;
}
