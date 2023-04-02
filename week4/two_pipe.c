#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define	BUFFER_SIZE	25
#define	READ_END	0
#define	WRITE_END	1

int main(void) {
	//char write_msg[BUFFER_SIZE] = "Greetings";
	//char read_msg[BUFFER_SIZE];
	int fd[2];
	int fd2[2];

	//int status;
	//int retval;
	int count = 0;
	int count2 = 1000;

	pid_t pid;

	if (pipe(fd) == -1) {
		printf("PIPE ERROR\n");
		fprintf(stderr, "Pipe failed");
		return 1;
	}

	pid = fork();

	if (pid < 0) {
		printf("FORK ERROR\n");
		fprintf(stderr, "Fork failed");
		return 1;
	}

	if (pid > 0) {		// parent process
            close(fd[READ_END]);    // 0
            write(fd[WRITE_END], &count, 1);
	    close(fd[WRITE_END]);

            close(fd2[WRITE_END]);
            read(fd2[READ_END], &count2, 1);
            for(int j=0; j<5; j++) {
                printf("parent got Message: parent %d\n", count2);
                count2++;
                sleep(1);
            }
            close(fd2[READ_END]);
	} else {		// child process
		close(fd[WRITE_END]);
		read(fd[READ_END], &count, 1);
		for(int j=0; j<5; j++) {
		    printf("      Child got Message: parent %d\n", count);
		    count++;
		    sleep(1);
		}
		close(fd[READ_END]);

		close(fd2[READ_END]);    // 0
                write(fd2[WRITE_END], &count2, 1);   // 1
		close(fd[WRITE_END]);

	}
	return 0;
    }
