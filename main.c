#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 128
#define BUFFER_SIZE 1024

char path_buffer[BUFFER_SIZE];

#define HIST 20
char *history[HIST];
int count = 0;

void signal_handling(int sig)
{
    if (sig == SIGINT || sig == SIGTSTP)
    {
        printf("\nnewshell> ");
        fflush(stdout);
    }
}

void interactive_mode()
{
    char input[MAX_INPUT_SIZE];
    char *commands[MAX_ARGS];
    char *args[MAX_ARGS];
    int num_commands, num_args;

    while (1)
    {
        printf("newshell> ");
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL)
        {
            break;
        }

        input[strcspn(input, "\n")] = '\0';

        parse_line(input, commands, &num_commands);

        for (int i = 0; i < num_commands; i++)
        {
            parse_command(commands[i], args, &num_args);
            execute_command(args);
        }

        if (strncmp(input, "myhistory", 9) == 0)
        {
            if (strcmp(input, "myhistory") == 0)
            {
                display();
            }
            else if (strcmp(input, "myhistory-c") == 0)
            {
                clear();
            }
            else if (strncmp(input, "myhistory-e", 11) == 0)
            {
                int num = atoi(input + 11);
                doCommand(num);
            }
        }
        else
        {
            if (strcmp(input, "exit") == 0)
            {
                break;
            }
            else
            {
                add(input);
            }
        }
    }
    clear();
}

void batch_mode(const char *batch_file)
{
    FILE *file = fopen(batch_file, "r");
    if (!file)
    {
        perror("Error opening the batch file");
        exit(EXIT_FAILURE);
    }

    char input[MAX_INPUT_SIZE];
    char *commands[MAX_ARGS];
    char *args[MAX_ARGS];
    int num_commands, num_args;

    while (fgets(input, MAX_INPUT_SIZE, file))
    {
        printf("%s", input);
        parse_line(input, commands, &num_commands);

        for (int i = 0; i < num_commands; i++)
        {
            parse_command(commands[i], args, &num_args);
            execute_command(args);
        }
    }

    fclose(file);
}

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