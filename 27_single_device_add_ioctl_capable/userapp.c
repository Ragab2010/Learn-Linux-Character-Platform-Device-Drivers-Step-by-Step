#include <stdio.h>
#include <fcntl.h> 
#include <stdlib.h>
#include <sys/ioctl.h>
#include "ioctl_cmd.h"

int main(int argc, char *argv[])
{
	int fd;

	fd = open("/dev/mydevice", O_RDWR);
	if (fd < 0) {
		perror("fd failed");
		exit(2);
	}

	ioctl(fd, SIG_IOCTL_SEND_SIGNAL);
	perror("ioctl");
	getchar();

	close(fd);
}
