#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int fork1(void);  
void panic(char*);
void dfs(int);
void fun(int);
int main(int argc, char* argv[]) {
    int p[2];
    
    if (argc > 1) {
        fprintf(2, "usage: prime\n");
        exit(1);
    }

    if (pipe(p) < 0) {
        panic("pipe error");
        exit(1);
    }

    for (int i = 2; i <= 35; i++) {
        write(p[1], &i, sizeof(int));
    }
    
    close(p[1]);
    fun(p[0]);
    close(p[0]);

    exit(0);
}

void panic(char *s) {
  fprintf(2, "%s\n", s);
  exit(1);
}

int fork1(void) {
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

void dfs(int fd) {
    int prime, num;
    int p[2];
    if (read(fd, &prime, sizeof(int)) == 0) {
        return;
    }

    if (pipe(p) < 0) {
        panic("pipe error");
        exit(1);
    }
    
    printf("prime %d\n", prime);
    while (read(fd, &num, sizeof(int))) {
        if (num % prime) {
            write(p[1], &num, sizeof(int));        
        }
    }
    close(p[1]);
    dfs(p[0]);
    close(p[0]);
}

void fun(int fd) {
    int prime, num;
    int p[2];

    if (read(fd, &prime, sizeof(int)) == 0) {
        return;
    }

    if (pipe(p) < 0) {
        panic("pipe error");
        exit(1);
    }

    printf("prime %d\n", prime);

    if (fork1() == 0) {
        close(p[1]);
        fun(p[0]);
        close(p[0]);
        exit(0);
    }
    else {
        while (read(fd, &num, sizeof(int))) {
            if (num % prime) {
                write(p[1], &num, sizeof(int));
            }
        }
        close(p[0]);
        close(p[1]);
        wait(0);
    }
}