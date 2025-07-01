#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 128
#define BUFFER_SIZE 1024

char path_buffer[BUFFER_SIZE];

#define HIST 20
char *history[HIST];
int count = 0;

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handling);
    signal(SIGTSTP, signal_handling);
    pid_t shell_pgid = getpid();
    setpgid(shell_pgid, shell_pgid);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
    strncpy(path_buffer, getenv("PATH"), BUFFER_SIZE);
    if (argc > 2)
    {
        fprintf(stderr, "Usage: %s [batchfile]", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc == 2)
    {
        batch_mode(argv[1]);
    }
    else
    {
        interactive_mode();
    }

    return 0;
}