#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <errno.h>
#include <pty.h>
#include <fcntl.h>
 
int main()
{
    int outfd[2];
    int infd[2];
    fd_set readset;
    int result;
    int max;
    int fd;
	pid_t pid = forkpty(&fd, NULL, NULL, NULL);
    printf("we have fork %d\n", pid);
    if(pid==-1) {
		printf("can't fork pty : %d %s\n", errno, strerror(errno));
	} else if (pid == 0) {
		printf("We are the child\n");
		//successfully fork : we are the child
		char *argv[]={ "/bin/bash","--login", 0};
		execv(argv[0], argv);
		printf("end exec %s\n",argv[0]);
		return 1;
	}
    //here we are in the parent process

	char buffer[100];
    int count;
 	printf("We are the parent\n");



/*int flags;
  if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
    flags = 0;
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("fcntl");
    return 1;
  }*/

// Removing canonical mode from stdin
struct termios ttystate;
tcgetattr(STDIN_FILENO, &ttystate);

ttystate.c_lflag &= ~ICANON;
ttystate.c_lflag &= ~ECHO;

if (ttystate.c_lflag & ICANON) printf("ICANON is on\n");
if (ttystate.c_lflag & IGNCR) printf("IGNCR is on\n");
if (ttystate.c_lflag & ICRNL) printf("ICRNL is on\n");
if (ttystate.c_lflag & ECHO) printf("ECHO is on\n");

//ttystate.c_lflag &= ~(ICANON | IGNCR);
//ttystate.c_lflag &= ICRNL;
//ttystate.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
ttystate.c_cc[VMIN] = 1;
tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
	do {
		FD_ZERO(&readset);
		FD_SET(0, &readset);
		FD_SET(fd, &readset);
		max=1;
		if (fd + 1>max) max=fd + 1;
		//printf("We will select, fd is %d, max is %d\n",fd, max);
		result = select(max, &readset, NULL, NULL, NULL);
		//printf("\nEnd select %d %d\n",result,errno);
		if (result > 0) {
			if (FD_ISSET(fd, &readset)) {
				//printf("fd ready\n");
				count = read(fd, buffer, sizeof(buffer)-1);
				//printf("fd read : %d %s\n",count, buffer);
					if (count >= 0) {
						buffer[count] = 0;
						//printf("fd read : %d %s\n",count, buffer);
						printf("%s", buffer);
						fflush(stdout);
					}
			}
			if (FD_ISSET(0, &readset)) {
				//printf("STDIN ready\n");
				count = read(0, buffer, sizeof(buffer)-1);
				//fgets(buffer, sizeof(buffer)-1, stdin);
				//printf("Read from stdin : %d \n",count);
				if (count >= 0) {
					buffer[count] = 0;
					write(fd, buffer, count+1);
				}
			}
		}
	   //printf("end while\n");
	} while (1);
	printf("end child process");
    return 1;
}

