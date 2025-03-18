#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
// #define MY_NAME "my_device_char"
#define MY_NAME "/dev/my_device_char"
int main()
{	char buffer[64];
	int fd;
	// todo: fd < 0 open device ???
	fd=open(MY_NAME,O_RDONLY);
	if(fd<0)
	{	
		// printf("open device %s\n",MY_NAME);
		perror("Failed to open device");
		return -1;
	}
	// read device
	printf("Ready to read device\n");
	read(fd,buffer,64);
	close(fd);
	printf("Read device success\n");
	return 0;
}
