#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

int value = 5;

int main(){
	int fd[2];
	pid_t pid;

	if (pipe(fd) == -1) {
            printf("PIPE ERROR\n");
            fprintf(stderr, "Pipe failed");
	    return 1;
	}

	pid = fork();

	if (pid == 0) {
		close(fd[0]);
		value += 15;
		write(fd[1], &value, 1);
		close(fd[0]);
		return 0;
	} else if (pid > 0) {
		wait(NULL);
		close(fd[1]);
		read(fd[0], &value, 1);
		printf("PARENT: value = %d\n", value); /* LINE A */
		close(fd[0]);
		return 0;
	}
}
