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

void add(const char *command)
{
    if (count = HIST)
    {
        free(history[0]);

        for (int i = 1; i < HIST; i++)
        {
            history[i - 1] = history[i];
        }
        count--;
    }
    history[count] = strdup(command);

    if (!history[count])
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    count++;
}

void display()
{
    for (int i = 0; i < count; i++)
    {
        printf("%d : %s\n", i + 1, history[i]);
    }
}

void clear()
{
    if (count > 20)
    {
        count = 20;
    }
    for (int i = 0; i < count; i++)
    {
        free(history[i]);
    }
    count = 0;
}

void doCommand(int num)
{
    if (num < 1 || num > count)
    {
        printf("Invalid command.\n");
    }
    else
    {
        system(history[num - 1]);
    }
}

void parse_line(char *line, char *commands[], int *num_commands)
{
    char *command = strtok(line, ";");
    *num_commands = 0;
    while (command != NULL)
    {
        commands[(*num_commands)++] = command;
        command = strtok(NULL, ";");
    }
}

void parse_command(char *command, char *args[], int *num_args)
{
    char *arg = strtok(command, " \t\n");
    *num_args = 0;
    while (arg != NULL)
    {
        args[(*num_args)++] = arg;
        arg = strtok(NULL, " \t\n");
    }
    args[*num_args] = NULL;
}

int handle_cd_command(char *args[])
{
    if (args[1] == NULL)
    {
        char *home_dir = getenv("HOME");
        if (home_dir == NULL)
        {
            fprintf(stderr, "cd: HOME environment variable not set\n");
            return -1;
        }
        if (chdir(home_dir) != 0)
        {
            perror("cd");
            return -1;
        }
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("cd");
            return -1;
        }
    }
    return 0;
}

void handle_path(char **args)
{
    if (args[1] == NULL)
    {
        printf("%s\n", getenv("PATH"));
    }
    else if (strcmp(args[1], "+") == 0 && args[2] != NULL)
    {
        strncat(path_buffer, ":", BUFFER_SIZE - strlen(path_buffer) - 1);
        strncat(path_buffer, args[2], BUFFER_SIZE - strlen(path_buffer) - 1);
        setenv("PATH", path_buffer, 1);
    }
    else if (strcmp(args[1], "-") == 0 && args[2] != NULL)
    {
        char *position = strstr(path_buffer, args[2]);
        if (position == NULL)
        {
            memmove(position, position + strlenm(args[2]) + 1, strlen(position + strlen(args[2]) + 1) + 1);
            setenv("PATH", path_buffer, 1);
        }
    }
    else
    {
        fprintf(stderr, "Invalid path\n");
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