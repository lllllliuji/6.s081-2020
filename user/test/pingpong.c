#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]) {
	int pid;
	int p[2];
	char buf[2];
	
	if (argc > 1) {
		fprintf(2, "error:usage pingpong\n");
		exit(1);
	}

	pipe(p);
	pid = fork();
	if (pid < 0) {
		fprintf(2, "fork error\n");
		exit(1);
	}
	else if (pid == 0) {
		pid = getpid();
		
		read(p[0], buf, 1);
		printf("%d: received ping\n", pid);
		write(p[1], buf, 1);
		
		close(p[0]);
		close(p[1]);
	}
	else {
		pid = getpid();
		write(p[1], "a", 1);
		wait(0);
		
		read(p[0], buf, 1);
		printf("%d: received pong\n", pid);

		close(p[0]);
		close(p[1]);
	}
	exit(0);
}
