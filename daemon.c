#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char **environ;
int fdin[2], fdout[2], fderr[2];

void parent()
{
    fd_set fds;
    char buf[256];
    struct timeval tv;

    FD_ZERO(&fds);
    FD_SET(fdin[0], &fds);
    FD_SET(fd
/*
    while (1) {
        select(
        scanf("%s", buf);
        write(stdin, buf, strlen(buf));
    }
*/
    ssize_t ret = read(fdout[0], buf, sizeof(buf));
    buf[ret] = '\0';
    printf("ret: %lu, content: %s\n", ret, buf);
}

int main()
{

    pipe(fdin);
    pipe(fdout);
    pipe(fderr);

    pid_t pid = fork();
    if (pid == 0) { /* child */
        dup2(fdout[1], 1);
        write(1, "hi", 2);
//        dup2(stdinout[0], 0);
//        dup2(stdinout[1], 1);
//        dup2(stderr[0], 2);
//        execve("/bin/bash", NULL, environ);
    } else if (pid > 0) {    /* parent */
        parent();
        wait(NULL);
    } else {    /* ERROR */
    }

    printf("Program exit\n");
    return 0;
}
