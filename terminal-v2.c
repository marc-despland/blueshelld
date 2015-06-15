#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <errno.h>
#include <pty.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/wait.h>


// File Descriptor for the child
int fd;

// Flag use to signal that the child shell is terminated
int listen;


/*
Action on SIGWINCH to manage TTY resizing
*/
void resizeTTY(int sig) {
	struct winsize termsize;
    ioctl(STDIN_FILENO,TIOCGWINSZ,&termsize);
	ioctl(fd,TIOCSWINSZ,&termsize);	
}	

/*
Action on SIGCHLD to manage Child termination
*/
void receivedSigChld(int sig) {
	int status, pid;
	pid=wait(&status);
	listen=0;
}

int main()
{
    fd_set readset;
    int result;
	sigset_t mask;

	//Create the sigaction structure to handle SIGWINCH signal
	struct sigaction eventWindowResize;
	sigemptyset(&eventWindowResize.sa_mask);
	eventWindowResize.sa_flags=0;
	eventWindowResize.sa_handler= resizeTTY;

	//Create the sigaction structure to handle SIGCHLD signal
    struct sigaction eventSigChld;
    sigemptyset(&eventSigChld.sa_mask);
    eventSigChld.sa_flags=0;
    eventSigChld.sa_handler= receivedSigChld;

	//Define the sigmask  to catch SIGCHLD with pselect
	sigemptyset (&mask);
	sigaddset (&mask, SIGCHLD);

	//Create the fork PTY to manage our shell
	pid_t pid = forkpty(&fd, NULL, NULL, NULL);
    if (pid==-1) {
		printf("can't fork pty : %d %s\n", errno, strerror(errno));
	} else if (pid == 0) {
		//here we are the child process
		char *argv[]={ "/bin/bash","--login", 0};
		execv(argv[0], argv);
		return 1;
		//child process ends here
	}
    //here we are in the parent process

	//Start handling signal
	sigaction(SIGWINCH, &eventWindowResize, NULL);
	sigaction(SIGCHLD,&eventSigChld, NULL);

	
	char buffer[100];
    int count;
 	printf("We are the parent\n");


	// Backup intial TTY mode of STDIN
	struct termios orgttystate;
	tcgetattr(STDIN_FILENO, &orgttystate);

	// Update STDIN mode to remove ECHO and ICANON to allow transmission of character without buffering and echo
	struct termios ttystate;
	tcgetattr(STDIN_FILENO, &ttystate);
	ttystate.c_lflag &= ~ICANON;
	ttystate.c_lflag &= ~ECHO;
	ttystate.c_cc[VMIN] = 1;
	tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

	//Change TTY size to the actual size of the terminal
	struct winsize termsize;
	ioctl(STDOUT_FILENO,TIOCGWINSZ,&termsize);
	ioctl(fd,TIOCSWINSZ,&termsize);
	
	//Initialize the flag to signal child has end
	listen=1;
	do {
		//initialize file descriptor for select
		FD_ZERO(&readset);
		FD_SET(0, &readset);
		FD_SET(fd, &readset);
		
		//Block until something to read on child stdout, parent stdin or SIGCHLD
		result = pselect(fd+1, &readset, NULL, NULL, NULL, &mask);
		if (result > 0) {
			if (FD_ISSET(fd, &readset)) {
				//Child has write on its stdout
				count = read(fd, buffer, sizeof(buffer)-1);
				if (count >= 0) {
					buffer[count] = 0;
					printf("%s", buffer);
					//we need to flush to display line that not ends with \n
					fflush(stdout);
				}
			}
			if (FD_ISSET(STDIN_FILENO, &readset)) {
				//User has write some stuff
				count = read(STDIN_FILENO, buffer, sizeof(buffer)-1);
				if (count >= 0) {
					buffer[count] = 0;
					write(fd, buffer, count);
				}
			}
		}
	} while (listen);
	
	//reset STDIN to its original mode and quit
	tcsetattr(STDIN_FILENO, TCSANOW, &orgttystate);
    return 1;
}