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

void handle_redirection(char *args[], int *input_fd, int *output_fd)
{
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "<") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "Error: No input file specified for redirection\n");
                return;
            }
            *input_fd = open(args[i + 1], O_RDONLY);
            if (*input_fd < 0)
            {
                perror("Error opening input file");
                return;
            }
            args[i] = NULL;
            break;
        }
        else if (strcmp(args[i], ">") == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "Error: No output file specified for redirection\n");
                return;
            }
            *output_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (*output_fd < 0)
            {
                perror("Error opening output file");
                return;
            }
            args[i] = NULL;
            break;
        }
    }
}

void handle_pipeline(char *commands[], int num)
{
    int pipe_fds[2];
    int input_fd = STDIN_FILENO;
    pid_t pid;

    for (int i = 0; i < num; i++)
    {
        if (i < num - 1 && pipe(pipe_fds) < 0)
        {
            perror("Error creating pipe");
            exit(EXIT_FAILURE);
        }

        pid = fork();
        if (pid == 0)
        {
            if (input_fd != STDIN_FILENO)
            {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }

            if (i < num - 1)
            {
                dup2(pipe_fds[1], STDOUT_FILENO);
                close(pipe_fds[0]);
                close(pipe_fds[1]);
            }
            char *args[MAX_ARGS];
            int num_args;
            parse_command(commands[i], args, &num_args);

            if (execvp(args[0], args) == -1)
            {
                perror("Error executing command in pipeline");
                exit(EXIT_FAILURE);
            }
        }
        else if (pid > 0)
        {
            waitpid(pid, NULL, 0);
            if (input_fd != STDIN_FILENO)
            {
                close(input_fd);
            }
            if (i < num - 1)
            {
                close(pipe_fds[1]);
                input_fd = pipe_fds[0];
            }
        }
        else
        {
            perror("Error forking process");
            exit(EXIT_FAILURE);
        }
    }
}

void execute_command(char *args[])
{
    if (args[0] == NULL)
    {
        return;
    }

    int num_pipes = 0;
    for (int i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            num_pipes++;
        }
    }

    if (num_pipes > 0)
    {
        char *pipeline_commands[MAX_ARGS];
        int num_pipeline_commands = 0;
        char *command = strtok(args[0], "|");
        while (command != NULL)
        {
            pipeline_commands[num_pipeline_commands++] = command;
            command = strtok(NULL, "|");
        }

        handle_pipeline(pipeline_commands, num_pipeline_commands);
        return;
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