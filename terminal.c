#include <unistd.h>
#include <stdio.h>
#include <sys/select.h>
#include <errno.h>
#include <pty.h>

/* since pipes are unidirectional, we need two pipes.
   one for data to flow from parent's stdout to child's
   stdin and the other for child's stdout to flow to
   parent's stdin */
 
#define NUM_PIPES          2
 
#define PARENT_WRITE_PIPE  0
#define PARENT_READ_PIPE   1
 
int pipes[NUM_PIPES][2];
 
/* always in a pipe[], pipe[0] is for read and 
   pipe[1] is for write */
#define READ_FD  0
#define WRITE_FD 1
 
#define PARENT_READ_FD  ( pipes[PARENT_READ_PIPE][READ_FD]   )
#define PARENT_WRITE_FD ( pipes[PARENT_WRITE_PIPE][WRITE_FD] )
 
#define CHILD_READ_FD   ( pipes[PARENT_WRITE_PIPE][READ_FD]  )
#define CHILD_WRITE_FD  ( pipes[PARENT_READ_PIPE][WRITE_FD]  )
 
void
main()
{
    int outfd[2];
    int infd[2];
    fd_set readset;
    int result;
    int max;
     
    // pipes for parent to write and read
    pipe(pipes[PARENT_READ_PIPE]);
    pipe(pipes[PARENT_WRITE_PIPE]);
     
    if(!fork()) {
        char *argv[]={ "/bin/bash","--login", 0};
 
        dup2(CHILD_READ_FD, STDIN_FILENO);
        dup2(CHILD_WRITE_FD, STDOUT_FILENO);
 
        /* Close fds not required by child. Also, we don't
           want the exec'ed program to know these existed */
//        close(CHILD_READ_FD);
//        close(CHILD_WRITE_FD);
        close(PARENT_READ_FD);
        close(PARENT_WRITE_FD);
          
        execv(argv[0], argv);
	printf("end exec\n");
    } else {
        char buffer[100];
        int count;
 
        /* close fds not required by parent */       
        //close(CHILD_READ_FD);
        close(CHILD_WRITE_FD);
/* 
        // Write to child’s stdin
        write(PARENT_WRITE_FD, "ls\n", 3);
  
        // Read from child’s stdout
        count = read(PARENT_READ_FD, buffer, sizeof(buffer)-1);
        if (count >= 0) {
            buffer[count] = 0;
            printf("%s", buffer);
        } else {
            printf("IO Error\n");
        }
	 // Write to child’s stdin
        write(PARENT_WRITE_FD, "ps -fe\n", 7);
        count = read(PARENT_READ_FD, buffer, sizeof(buffer)-1);
        if (count >= 0) {
            buffer[count] = 0;
            printf("%s", buffer);
        } else {
            printf("IO Error\n");
        }

	sleep(2);*/

do {
   FD_ZERO(&readset);
   FD_SET(0, &readset);
   FD_SET(PARENT_READ_FD, &readset);
   max=1;
   if (PARENT_READ_FD + 1>max) max=PARENT_READ_FD + 1;
   result = select(max, &readset, NULL, NULL, NULL);
   //printf("End select %d %d %s\n",result,errno, strerror(errno));
   if (result > 0) {
   	if (FD_ISSET(PARENT_READ_FD, &readset)) {
		//printf("PARENT_READ_FD ready\n");
		count = read(PARENT_READ_FD, buffer, sizeof(buffer)-1);
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

