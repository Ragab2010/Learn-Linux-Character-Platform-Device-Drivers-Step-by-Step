#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>

void *func(void *arg)
{
	int fd;
	
	fd = open("/dev/mydevice", O_RDWR);
	if (fd < 0) {
		printf("Thread id:%d failed to opened file\n",
				pthread_self());
		return NULL;
	} else {
		printf("Thread id:%d successfully opened file\n",
				pthread_self());
	}	
	sleep(5);
	
	write(fd, "hello world", sizeof("hello world"));

	close(fd);
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t thread1, thread2;

	pthread_create(&thread1,NULL, func, NULL);
	pthread_create(&thread2,NULL, func, NULL);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	return 0;
}
