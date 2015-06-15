#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <errno.h>
#include <pty.h>

 
void
main()
{
    int outfd[2];
    int infd[2];
    fd_set readset;
    int result;
    int max;
    int fd;
	pid_t pid = forkpty(&fd, NULL, NULL, NULL);
    if(pid==-1) {
		printf("can't fork pty : %d %s\n", errno, strerror(errno));
	} else if (pid == 0) {
		//successfully fork : we are the child
		char *argv[]={ "/bin/bash","--login", 0};
		execv(argv[0], argv);
		printf("end exec %s\n",argv[0]);
		return 1;
	}
    //here we are in the parent process

	char buffer[100];
    int count;
 
	do {
		FD_ZERO(&readset);
		FD_SET(0, &readset);
		FD_SET(fd, &readset);
		max=1;
		if (fd + 1>max) max=fd + 1;
		result = select(max, &readset, NULL, NULL, NULL);
		printf("End select %d %d %s\n",result,errno, strerror(errno));
		if (result > 0) {
			if (FD_ISSET(fd, &readset)) {
				//printf("PARENT_READ_FD ready\n");
				count = read(fd, buffer, sizeof(buffer)-1);
					if (count >= 0) {
						buffer[count] = 0;
						printf("%s", buffer);
					}
			}
			if (FD_ISSET(0, &readset)) {
				//printf("STDIN ready %d\n",sizeof(buffer)-1);
				count = read(0, buffer, sizeof(buffer)-1);
				//fgets(buffer, sizeof(buffer)-1, stdin);
				//printf("Read from stdin : %d \n",count);
				if (count >= 0) {
					buffer[count] = 0;
					write(PARENT_WRITE_FD, buffer, count+1);
				}
			}
		}
	   //printf("end while\n");
	} while (1);
	printf("end child process");
    }
}

