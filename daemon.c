#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>

extern FILE *stderr;
extern char **environ;
int fdin[2], fdout[2], fderr[2];

void handle_child_exit(int signo)
{
    int status;
    wait(&status);
    if (WIFEXITED(status)) {
        printf("exit...\n");
        exit(1);
    }
}

void show_fd(int fd)
{
    char buf[256];
    ssize_t ret = read(fd, buf, sizeof(buf)-1);
    buf[ret] = '\0';
    printf("%s", buf);
    fflush(stdout);
}

void *monitor_parent_stdout(void *data)
{
    fd_set rfds;

    FD_ZERO(&rfds);
    FD_SET(fdout[0], &rfds);
    FD_SET(fderr[0], &rfds);

    while (1) {
        if (select(fderr[0]+1, &rfds, NULL, NULL, NULL) <= 0)
            continue;

        if (FD_ISSET(fdout[0], &rfds)) {
            show_fd(fdout[0]);
//            printf("from out\n");
        }
        if (FD_ISSET(fderr[0], &rfds)) {
            show_fd(fderr[0]);
//            printf("from err\n");
        }
    }
}

void parent(pid_t childpid)
{
    pthread_t tid;
    struct sigaction sig;
    char buf[256];

    sig.sa_handler = handle_child_exit;
    sigaction(SIGCHLD, &sig, NULL);

    pthread_create(&tid, NULL, monitor_parent_stdout, NULL);

    /* read is from file */

    while (1) {
//        scanf("%[^\n]", buf);
//        getchar();
//        write(fdin[1], buf, strlen(buf));
        sleep(1);
        write(fdin[1], "ls", 2);
        sleep(1);
        write(fdin[1], "sd", 2);
    }

    pthread_join(tid, NULL);
}

void child()
{
    char buf[256];

    dup2(fdin[0], 0);
    dup2(fdout[1], 1);
    dup2(fderr[1], 2);

    setvbuf(stdout, NULL, _IOLBF, 1024);
//    setvbuf(stderr, NULL, _IONBF, 1024);    /* it seems no necessary */

    while (1) {
//        fprintf(stderr, "shane@fucktu:~$\n");
//        printf("shane@fucktu:~$ \n");
//        fflush(stdout);

        ssize_t ret = read(0, buf, sizeof(buf)-1);
        if (ret == -1)
            perror("read: ");
        buf[ret] = '\0';

        if (!strncmp(buf, "ls", 2)) {
            printf("list files\n");
        } else if (!strncmp(buf, "pwd", 3)) {
            printf("/home/shane\n");
        } else if (!strncmp(buf, "exit", 4)) {
            break;
        } else {
//            write(2, "fuck\n", 5);
            fprintf(stderr, "command [%s] not found\n", buf);
//            printf("command [%s] not found\n", buf);
        }
    }
}

int main()
{

    pipe(fdin);
    pipe(fdout);
    pipe(fderr);

    pid_t pid = fork();
    if (pid == 0) { /* child */
        child();
//        write(1, "Hello\n", 6);
//        fprintf(stderr, "hel\n");
//        execve("/bin/bash", NULL, environ);
    } else if (pid > 0) {    /* parent */
        parent(pid);
        wait(NULL);
    } else {    /* ERROR */
    }

    printf("Program exit\n");
    return 0;
}
